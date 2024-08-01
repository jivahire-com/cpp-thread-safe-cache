//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio.c
 *    GPIO implementation.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkClientInterfaceOpen
#include <DfwkDriver.h> // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <DfwkHost.h>   // for DfwkDeviceInitialize
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_init.h>  // for fpfw_init_get_handle
#include <gpio.h>       // for gpio_device_t, pgpio_device_t, gpio_interface_t, ...
#include <gpio_lib.h>   // for GPIO_CTRL_PIN_MSK_FLAG
#include <kng_error.h>  // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <nvic.h>       // for nvic_irq_clear_pending, nvic_irq_enable, nvic...
#include <tx_api.h>     // for TX_NO_TIME_SLICE, TX_SUCCESS, TX_WAIT_FOREVER, ...

/*-- Symbolic Constant Macros (defines) --*/
#define GPIO_ISR_THREAD_STACK_SIZE   (TX_MINIMUM_STACK + 1024)
#define GPIO_MESSAGE_QUEUE_POOL_SIZE 10

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pgpio_interface_t gpio_interface = NULL;
static uint8_t gpio_isr_thread_stack[GPIO_ISR_THREAD_STACK_SIZE];
static uint32_t gpio_message_queue_pool[GPIO_MESSAGE_QUEUE_POOL_SIZE];

/*------------- Functions ----------------*/
/**
 * @brief GPIO Interrupt Service Routine
 *
 * @param context [in] GPIO device as context.
 */
void gpio_isr(void* context)
{
    pgpio_device_t dev = (pgpio_device_t)context;

    uint32_t irq_num = 0;
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_mask = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t gpio_level = 0;
    uint32_t message = 0;

    nvic_status_t nvic_status = nvic_get_current_irq(&irq_num);
    FPFW_RUNTIME_ASSERT(nvic_status == NVIC_STATUS_SUCCESS);

    // Find matching GPIO controller ID from irq number.
    for (uint32_t i = 0; i < dev->IrqConfigCount; i++)
    {
        if (dev->IrqConfig[i].nvic_irq == irq_num)
        {
            gpio_ctrl_id = dev->IrqConfig[i].gpio_ctrl_id;
            break;
        }
    }

    if (gpio_ctrl_id == 0)
    {
        // No mapped GPIO controller.
        return;
    }

    int silib_status = gpio_get_interrupt_status(GPIO_CTRL_PIN_MSK(gpio_ctrl_id, 0xFF), &gpio_pin_mask);
    FPFW_RUNTIME_ASSERT(silib_status == SILIBS_SUCCESS);

    gpio_pin_id = GPIO_CTRL_PIN_MSK(gpio_ctrl_id, gpio_pin_mask);

    silib_status = gpio_get_input(gpio_pin_id, &gpio_level);
    FPFW_RUNTIME_ASSERT(silib_status == SILIBS_SUCCESS);

    // Upper 16 bits is GPIO level mask, Lower 16 bits is GPIO pin mask.
    message = ((gpio_level & 0xFFFF) << 16) | (gpio_pin_id & 0xFFFF);

    // Send message to the deferred ISR thread.
    UINT tx_status = tx_queue_send(&dev->TxGpioIsrMessageQueue, &message, TX_NO_WAIT);
    FPFW_RUNTIME_ASSERT(tx_status == TX_SUCCESS);

    // Clear interrupt status.
    silib_status = gpio_clear_interrupt_status(gpio_pin_id);
    FPFW_RUNTIME_ASSERT(silib_status == SILIBS_SUCCESS);
}

/**
 * @brief Mockable thread loop function, which can be overridden by tests.
 *
 * @return Always return true for the thread loop.
 */
bool thread_loop()
{
    return true;
}

/**
 * @brief GPIO ISR callback thread main.
 *
 * @param thread_input [in] Thread input which is device pointer.
 */
void gpio_isr_callback_thread_main(ULONG thread_input)
{
    pgpio_device_t dev = (pgpio_device_t)thread_input;
    UINT txStatus = TX_SUCCESS;
    ULONG message = 0;

    while (thread_loop())
    {
        bool barrierAdded = false;
        txStatus = tx_queue_receive(&dev->TxGpioIsrMessageQueue, &message, TX_WAIT_FOREVER);

        if (txStatus == TX_SUCCESS)
        {
            // Decode message: Upper 16 bits is GPIO level mask, Lower 16 bits is GPIO pin mask.
            const uint32_t isr_level = (message >> 16) & 0xFFFF;
            const uint32_t isr_gpio_pin_mask = (message & 0xFFFF) | GPIO_CTRL_PIN_MSK_FLAG;

            PDFWK_ASYNC_REQUEST_HEADER pending_request = NULL;
            while (DfwkQueueDequeueRequest(&dev->IsrReqQueue, &pending_request))
            {
                if (pending_request->RequestType == GPIO_REQUEST_NULL)
                {
                    // End of the queue.
                    break;
                }

                FPFW_RUNTIME_ASSERT(pending_request->RequestType == GPIO_REQUEST_ISR_ASYNC);
                pgpio_request_t gpio_request = (pgpio_request_t)pending_request;
                uint8_t request_ctrl_id = GET_GPIO_CTRL_ID(gpio_request->gpio_pin_id);
                uint8_t request_pin_mask = 0;

                if (gpio_request->gpio_pin_id & GPIO_CTRL_PIN_MSK_FLAG)
                {
                    // Request ISR with GPIO pin mask.
                    request_pin_mask = GET_GPIO_PIN_ID(gpio_request->gpio_pin_id);
                }
                else
                {
                    // Request ISR with single GPIO pin ID.
                    request_pin_mask = 1 << GET_GPIO_PIN_ID(gpio_request->gpio_pin_id);
                }

                if (GET_GPIO_CTRL_ID(isr_gpio_pin_mask) == request_ctrl_id &&
                    (GET_GPIO_PIN_ID(isr_gpio_pin_mask) & request_pin_mask))
                {
                    gpio_request->level = (gpio_request->gpio_pin_id & GPIO_CTRL_PIN_MSK_FLAG)
                                              ? (isr_level & request_pin_mask)
                                              : ((isr_level & request_pin_mask) ? 1 : 0);
                    gpio_request->status = KNG_SUCCESS;

                    DfwkAsyncRequestComplete(&gpio_request->Header);
                }
                else
                {
                    // Put the request back to the queue.

                    if (!barrierAdded)
                    {
                        static gpio_request_t isr_null_request = {.Header = {.RequestType = GPIO_REQUEST_NULL},
                                                                  .gpio_pin_id = 0};

                        // Add a barrier to prevent the queue from being empty.
                        DfwkQueueEnqueueRequest(&dev->IsrReqQueue, &isr_null_request.Header);
                        barrierAdded = true;
                    }

                    DfwkQueueEnqueueRequest(&dev->IsrReqQueue, pending_request);
                }
            }
        }
    }
}

/**
 * @brief GPIO driver request default dispatcher.
 *
 * @param request [in] GPIO request.
 * @param context [in] GPIO device as context.
 */
static void gpio_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    pgpio_device_t dev = (pgpio_device_t)context;
    pgpio_request_t gpio_request = (pgpio_request_t)request;

    switch (request->RequestType)
    {
    case GPIO_REQUEST_ISR_ASYNC:
        // Move the request to the deferred manual queue.
        DfwkQueueEnqueueRequest(&dev->IsrReqQueue, request);
        break;
    default:
        // Unsupported request type.
        gpio_request->status = KNG_E_INVALIDARG;
        DfwkAsyncRequestComplete(request);
        break;
    }
}

/**
 * @brief Initialize GPIO device.
 *
 * @param dev [out] GPIO device.
 * @param schedule [in] Pointer to the schedule.
 */
void gpio_device_init(pgpio_device_t dev, PDFWK_SCHEDULE schedule)
{
    UINT txStatus = TX_SUCCESS;

    DfwkDeviceInitialize(&dev->Header, schedule);

    // Initialize GPIO driver request queue.
    DfwkQueueInitialize(&dev->Queue, &dev->Header, gpio_dispatch, dev, DfwkQueueType_SerializedDispatch);

    // Initialize GPIO interrupt callback manual queue.
    DfwkQueueInitialize(&dev->IsrReqQueue, &dev->Header, gpio_dispatch, dev, DfwkQueueType_ManualDispatch);

    // Create a message queue to handle GPIO ISR in a deferred way.
    txStatus = tx_queue_create(&dev->TxGpioIsrMessageQueue,
                               (char*)"gpio_isr_queue",                               // Message queue name
                               sizeof(gpio_message_queue_pool[0]) / sizeof(uint32_t), // Message_size in 32-bit word
                               gpio_message_queue_pool,          // Message queue memory pool
                               sizeof(gpio_message_queue_pool)); // Message queue size in bytes
    FPFW_RUNTIME_ASSERT(txStatus == TX_SUCCESS);

    // Create a thread to handle GPIO ISR in a deferred way.
    txStatus = tx_thread_create(&dev->GpioIsrThread,
                                "gpio_deferred_isr",           // Thread name
                                gpio_isr_callback_thread_main, // Thread entry
                                (ULONG)dev,                    // Entry input
                                gpio_isr_thread_stack,         // Thread stack
                                GPIO_ISR_THREAD_STACK_SIZE,    // Thread stack size
                                1,                             // Priority
                                1,                             // Preempt Threshold
                                TX_NO_TIME_SLICE,
                                TX_AUTO_START);
    FPFW_RUNTIME_ASSERT(txStatus == TX_SUCCESS);
}

/**
 * @brief Initialize GPIO driver interface.
 *
 * @param iface [out] GPIO interface.
 * @param dev [in] GPIO device.
 */
void gpio_interface_init(pgpio_interface_t iface, pgpio_device_t dev)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    // Initialize GPIO driver interface.
    iface->Device = dev;
    DfwkInterfaceInitialize(&iface->Header, &dev->Header, &dev->Queue, NULL); // Synchonous request is not supported.

    for (uint32_t i = 0; i < dev->IrqConfigCount; i++)
    {
        // Register ISR for each GPIO controller.
        nvic_status = nvic_irq_set_isr_with_param(dev->IrqConfig[i].nvic_irq, gpio_isr, dev);
        FPFW_RUNTIME_ASSERT(nvic_status == NVIC_STATUS_SUCCESS);

        // Clear any pending interrupt.
        nvic_status = nvic_irq_clear_pending(dev->IrqConfig[i].nvic_irq);
        FPFW_RUNTIME_ASSERT(nvic_status == NVIC_STATUS_SUCCESS);

        // Enable the interrupt.
        nvic_status = nvic_irq_enable(dev->IrqConfig[i].nvic_irq);
        FPFW_RUNTIME_ASSERT(nvic_status == NVIC_STATUS_SUCCESS);
    }
}

/**
 * @brief Validate async request and send it to GPIO driver if valid.
 *
 * @param request [in] GPIO request.
 * @return KNG_SUCCESS if succeeded, otherwise error code.
 */
uint32_t gpio_cmd_async(pgpio_request_t request)
{
    // Check the interface is initialized.
    if (gpio_interface == NULL)
    {
        gpio_interface = (pgpio_interface_t)fpfw_init_get_handle("gpio_int"); // Use the interface initialized at init state.

        if (gpio_interface == NULL)
        {
            return KNG_E_NOINTERFACE;
        }
    }

    // Check the interface is opened.
    if (!gpio_interface->Header.Opened)
    {
        int32_t status = DfwkClientInterfaceOpen(&gpio_interface->Header);
        if (DFWK_FAILED(status))
        {
            return KNG_E_FAIL;
        }
    }

    // Check request completion callback is set.
    if (request->Header.CompletionRoutine == NULL)
    {
        return KNG_E_INVALIDARG;
    }

    // Check request type.
    switch (request->Header.RequestType)
    {
    case GPIO_REQUEST_ISR_ASYNC:
        break;
    default:
        // Unsupported request type.
        return KNG_E_INVALIDARG;
    }

    DfwkInterfaceSendAsync(&gpio_interface->Header, &request->Header);

    return KNG_SUCCESS;
}
