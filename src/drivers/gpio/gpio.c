//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio.c
 *    GPIO implementation.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>    // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <DfwkHost.h>      // for DfwkDeviceInitialize
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <arm_intrinsic.h> // for __DSB
#include <bug_check.h>     // for BUG_ASSERT
#include <fpfw_init.h>     // for fpfw_init_get_handle
#include <gpio.h>          // for gpio_device_t, pgpio_device_t, gpio_interface_t, ...
#include <gpio_lib.h>      // for GPIO_CTRL_PIN_MSK_FLAG
#include <kng_error.h>     // for KNG_E_INVALIDARG, KNG_E_NOTIMPL
#include <nvic.h>          // for nvic_irq_clear_pending, nvic_irq_enable, nvic...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

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
    uint32_t gpio_level = 0;

    nvic_status_t nvic_status = nvic_get_current_irq(&irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);

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

    // Get GPIO pin mask.
    int silib_status = gpio_get_interrupt_status(GPIO_CTRL_PIN_MSK(gpio_ctrl_id, 0xFF), &gpio_pin_mask);
    BUG_ASSERT_PARAM(silib_status == SILIBS_SUCCESS, silib_status, gpio_pin_mask);

    // Get GPIO level.
    silib_status = gpio_get_input(GPIO_CTRL_PIN_MSK(gpio_ctrl_id, gpio_pin_mask), &gpio_level);
    BUG_ASSERT_PARAM(silib_status == SILIBS_SUCCESS, silib_status, gpio_level);

    bool barrierAdded = false;
    PDFWK_ASYNC_REQUEST_HEADER pending_request = NULL;

    // Process all pending requests in the deferred manual queue.
    while (DfwkQueueDequeueRequest(&dev->IsrReqQueue, &pending_request))
    {
        if (pending_request->RequestType == GPIO_REQUEST_NULL)
        {
            // End of the queue.
            break;
        }

        BUG_ASSERT_PARAM(pending_request->RequestType == GPIO_REQUEST_ISR_ASYNC, pending_request->RequestType, 0);
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

        if (gpio_ctrl_id == request_ctrl_id && (gpio_pin_mask & request_pin_mask))
        {
            // Matched GPIO controller and pin mask.
            // Set GPIO level and status to the request and complete it.
            gpio_request->level = (gpio_request->gpio_pin_id & GPIO_CTRL_PIN_MSK_FLAG)
                                      ? (gpio_level & request_pin_mask)
                                      : ((gpio_level & request_pin_mask) ? 1 : 0);
            gpio_request->status = KNG_SUCCESS;

            DfwkAsyncRequestComplete(&gpio_request->Header);
        }
        else
        {
            // This pending request is not for this interrupt. Put the request back to the queue.
            if (!barrierAdded)
            {
                static gpio_request_t isr_null_request = {.Header = {.RequestType = GPIO_REQUEST_NULL}, .gpio_pin_id = 0};

                // Add a barrier marker request to indicate end of pending requests before re-enqueueing.
                DfwkQueueEnqueueRequest(&dev->IsrReqQueue, &isr_null_request.Header);
                barrierAdded = true;
            }

            DfwkQueueEnqueueRequest(&dev->IsrReqQueue, pending_request);
        }
    }

    // Clear interrupt status.
    silib_status = gpio_clear_interrupt_status(GPIO_CTRL_PIN_MSK(gpio_ctrl_id, gpio_pin_mask));
    BUG_ASSERT_PARAM(silib_status == SILIBS_SUCCESS, silib_status, gpio_ctrl_id);

    __DSB();
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
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    DfwkDeviceInitialize(&dev->Header, schedule);

    // Initialize GPIO driver request queue.
    DfwkQueueInitialize(&dev->Queue, &dev->Header, gpio_dispatch, dev, DfwkQueueType_SerializedDispatch);

    // Initialize GPIO interrupt callback manual queue.
    DfwkQueueInitialize(&dev->IsrReqQueue, &dev->Header, gpio_dispatch, dev, DfwkQueueType_ManualDispatch);

    for (uint32_t i = 0; i < dev->IrqConfigCount; i++)
    {
        // Register ISR for each GPIO controller.
        nvic_status = nvic_irq_set_isr_with_param(dev->IrqConfig[i].nvic_irq, gpio_isr, dev);
        BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, dev->IrqConfig[i].nvic_irq);

        // Clear any pending interrupt.
        nvic_status = nvic_irq_clear_pending(dev->IrqConfig[i].nvic_irq);
        BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, dev->IrqConfig[i].nvic_irq);

        // Enable the interrupt.
        nvic_status = nvic_irq_enable(dev->IrqConfig[i].nvic_irq);
        BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, dev->IrqConfig[i].nvic_irq);
    }
}

/**
 * @brief Initialize GPIO interface.
 *
 * @param iface [out] GPIO interface.
 */
void gpio_interface_init(pgpio_interface_t iface)
{
    // Initialize interface with device.
    iface->Device = (pgpio_device_t)fpfw_init_get_handle("gpio_dev");
    DfwkInterfaceInitialize(&iface->Header, &iface->Device->Header, &iface->Device->Queue, NULL); // Synchonous request is not supported.
}

/**
 * @brief Register deferred GPIO ISR as completion callback.
 *
 * @param iface [in] GPIO interface.
 * @param request [in] GPIO request.
 * @param gpio_ctrl_pin_id [in] GPIO controller pin ID.
 * @param callback [in] Completion callback.
 */
uint32_t gpio_register_deferred_isr(pgpio_interface_t iface,
                                    pgpio_request_t request,
                                    uint32_t gpio_ctrl_pin_id,
                                    DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                    void* context)
{
    // Validate input arguments.
    if (iface == NULL || request == NULL || callback == NULL)
    {
        return KNG_E_INVALIDARG;
    }

    if (iface->Device == NULL)
    {
        // GPIO interface is not initialized properly.
        return KNG_E_HANDLE;
    }

    if (request->Header.AllocatedSize != sizeof(gpio_request_t))
    {
        // Request is not properly initialized. Initialize the request.
        DfwkAsyncRequestInitialize(&request->Header, sizeof(gpio_request_t));
    }

    // Configure the request.
    request->Header.RequestType = GPIO_REQUEST_ISR_ASYNC;
    request->gpio_pin_id = gpio_ctrl_pin_id;
    DfwkAsyncRequestSetCompletionRoutine(&request->Header, callback, context);

    // Send the request to GPIO driver.
    DfwkInterfaceSendAsync(&iface->Header, &request->Header);

    return KNG_SUCCESS;
}
