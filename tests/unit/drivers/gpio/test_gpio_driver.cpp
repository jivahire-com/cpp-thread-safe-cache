//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_gpio_driver.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <DfwkCommon.h>     // for DfwkAsyncRequestInitialize, DfwkAsyncRe...
#include <DfwkSchedule.h>   // for DFWK_SCHEDULE
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <gpio.h>           // for gpio_request_type_idx, gpio_request_t
#include <gpio_lib.h>       // for MSCP_EXP_GPIO_4, MSCP_EXP_GPIO_6, GPIO_C...
#include <interrupts.h>     // for _SCP_IRQn_t
#include <kng_error.h>      // for KNG_E_INVALIDARG, KNG_STATUS, KNG_SUCCESS
#include <stdint.h>         // for uint32_t
#include <string.h>         // for NULL, memset, strcmp

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_gpio_device_init, nullptr, nullptr)
{
    gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    gpio_device_t test_gpio_device;
    DFWK_SCHEDULE test_schedule;

    test_gpio_device.IrqConfig = gpio_irq_config;
    test_gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Set up expectations
    expect_value(__wrap_DfwkDeviceInitialize, Device, &test_gpio_device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &test_schedule);
    expect_function_call(__wrap_DfwkDeviceInitialize); // expect device initialization

    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_gpio_device.Queue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_gpio_device);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_gpio_device);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);
    expect_function_call(__wrap_DfwkQueueInitialize); // expect default queue initialization

    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_gpio_device.IsrReqQueue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_gpio_device);
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &test_gpio_device);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_ManualDispatch);
    expect_function_call(__wrap_DfwkQueueInitialize); // expect Isr request manual queue initialization

    for (unsigned int i = 0; i < sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]); i++)
    {
        expect_function_call(__wrap_nvic_irq_set_isr_with_param); // expect IRQ set
        expect_function_call(__wrap_nvic_irq_clear_pending);      // expect IRQ clear pending
        expect_function_call(__wrap_nvic_irq_enable);             // expect IRQ enable
    }

    // Call API under test
    gpio_device_init(&test_gpio_device, &test_schedule);
}

TEST_FUNCTION(test_gpio_interface_init, nullptr, nullptr)
{
    gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    gpio_device_t test_gpio_device;
    gpio_interface_t test_gpio_interface;
    test_gpio_device.IrqConfig = gpio_irq_config;
    test_gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Set up expectations
    expect_string(__wrap_fpfw_init_get_handle, id, "gpio_dev");
    will_return(__wrap_fpfw_init_get_handle, &test_gpio_device);
    expect_function_call(__wrap_fpfw_init_get_handle); // expect get handle

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_gpio_interface);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_gpio_device.Header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_gpio_device.Queue);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchSync, NULL);
    expect_function_call(__wrap_DfwkInterfaceInitialize); // expect interface initialization

    // Call API under test
    gpio_interface_init(&test_gpio_interface);

    // Check expectations
    assert_int_equal(test_gpio_interface.Device, &test_gpio_device);
}

extern void gpio_isr(void* context);

TEST_FUNCTION(test_gpio_isr, nullptr, nullptr)
{
    gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    gpio_device_t test_gpio_device;
    uint32_t interrupt_status = 0x00000002;

    test_gpio_device.IrqConfig = gpio_irq_config;
    test_gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Test GPIO ISR
    // Mock nvic_get_current_irq, gpio_get_input and gpio_get_interrupt_status and check if message generated as expected.
    will_return(__wrap_nvic_get_current_irq, HW_INT_GPIO_CTRL_4_INT);
    expect_function_call(__wrap_nvic_get_current_irq);

    expect_value(__wrap_gpio_get_interrupt_status, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF));
    will_return(__wrap_gpio_get_interrupt_status, interrupt_status);
    expect_function_call(__wrap_gpio_get_interrupt_status);

    expect_value(__wrap_gpio_get_input, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status));
    will_return(__wrap_gpio_get_input, interrupt_status);
    expect_function_call(__wrap_gpio_get_input);

    gpio_request_t request_4_2 = {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 2)};
    gpio_request_t request_4_1 = {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 1)};
    gpio_request_t request_null = {.Header = {.RequestType = GPIO_REQUEST_NULL}, .gpio_pin_id = 0};

    // Get GPIO4:2 request
    will_return(__wrap_DfwkQueueDequeueRequest, &request_4_2);
    will_return(__wrap_DfwkQueueDequeueRequest, true);
    expect_function_call(__wrap_DfwkQueueDequeueRequest);

    // Enqueue barrier request into IsrReqQueue
    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &(test_gpio_device.IsrReqQueue));
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request->RequestType, request_null.Header.RequestType);
    expect_value(__wrap_DfwkQueueEnqueueRequest, gpio_request->gpio_pin_id, request_null.gpio_pin_id);
    expect_function_call(__wrap_DfwkQueueEnqueueRequest);

    // Re-Enqueue GPIO4:2 request
    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &(test_gpio_device.IsrReqQueue));
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request->RequestType, request_4_2.Header.RequestType);
    expect_value(__wrap_DfwkQueueEnqueueRequest, gpio_request->gpio_pin_id, request_4_2.gpio_pin_id);
    expect_function_call(__wrap_DfwkQueueEnqueueRequest);

    // Get GPIO4:1 request matching with interrupt status
    will_return(__wrap_DfwkQueueDequeueRequest, &request_4_1);
    will_return(__wrap_DfwkQueueDequeueRequest, true);
    expect_function_call(__wrap_DfwkQueueDequeueRequest);

    // Expect completion of GPIO4:1
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Get barrier request which will break loop
    will_return(__wrap_DfwkQueueDequeueRequest, &request_null);
    will_return(__wrap_DfwkQueueDequeueRequest, true);
    expect_function_call(__wrap_DfwkQueueDequeueRequest);

    // Make sure interrupts are cleared
    expect_value(__wrap_gpio_clear_interrupt_status, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status));
    expect_function_call(__wrap_gpio_clear_interrupt_status);

    gpio_isr(&test_gpio_device);
}

TEST_FUNCTION(test_gpio_isr_mask, nullptr, nullptr)
{
    gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    gpio_device_t test_gpio_device;
    uint32_t interrupt_status = 0x00000002;

    test_gpio_device.IrqConfig = gpio_irq_config;
    test_gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Test GPIO ISR
    // Mock nvic_get_current_irq, gpio_get_input and gpio_get_interrupt_status and check if message generated as expected.
    will_return(__wrap_nvic_get_current_irq, HW_INT_GPIO_CTRL_4_INT);
    expect_function_call(__wrap_nvic_get_current_irq);

    expect_value(__wrap_gpio_get_interrupt_status, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF));
    will_return(__wrap_gpio_get_interrupt_status, interrupt_status);
    expect_function_call(__wrap_gpio_get_interrupt_status);

    expect_value(__wrap_gpio_get_input, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status));
    will_return(__wrap_gpio_get_input, interrupt_status);
    expect_function_call(__wrap_gpio_get_input);

    gpio_request_t request_4_all = {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF)};

    // Get GPIO4:1 request matching with interrupt status
    will_return(__wrap_DfwkQueueDequeueRequest, &request_4_all);
    will_return(__wrap_DfwkQueueDequeueRequest, true);
    expect_function_call(__wrap_DfwkQueueDequeueRequest);

    // Expect completion of GPIO4:1
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    will_return(__wrap_DfwkQueueDequeueRequest, NULL);
    will_return(__wrap_DfwkQueueDequeueRequest, false);
    expect_function_call(__wrap_DfwkQueueDequeueRequest);

    // Make sure interrupts are cleared
    expect_value(__wrap_gpio_clear_interrupt_status, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status));
    expect_function_call(__wrap_gpio_clear_interrupt_status);

    gpio_isr(&test_gpio_device);
}

static void gpio_isr_callback(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    FPFW_UNUSED(CompletionContext);
}

TEST_FUNCTION(test_gpio_register_deferred_isr, nullptr, nullptr)
{
    gpio_device_t gpio_device = {};
    gpio_interface_t gpio_interface = {.Device = &gpio_device};
    gpio_request_t gpio_isr_request = {};
    uint32_t context = 0;

    // Set up expectations
    expect_value(__wrap_DfwkAsyncRequestInitialize, Request, &gpio_isr_request.Header);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(gpio_request_t));
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, &gpio_isr_request.Header);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, gpio_isr_callback);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, &context);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &gpio_interface.Header);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, &gpio_isr_request.Header);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    uint32_t result = gpio_register_deferred_isr(&gpio_interface, &gpio_isr_request, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), gpio_isr_callback, &context);
    assert_int_equal(result, (uint32_t)KNG_SUCCESS);

    // Invalid parameter tests
    gpio_interface.Device = NULL;
    result = gpio_register_deferred_isr(&gpio_interface, &gpio_isr_request, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), gpio_isr_callback, &context);
    assert_int_equal(result, (uint32_t)KNG_E_HANDLE);

    result = gpio_register_deferred_isr(NULL, &gpio_isr_request, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), gpio_isr_callback, &context);
    assert_int_equal(result, (uint32_t)KNG_E_INVALIDARG);

    result = gpio_register_deferred_isr(&gpio_interface, NULL, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), gpio_isr_callback, &context);
    assert_int_equal(result, (uint32_t)KNG_E_INVALIDARG);

    result = gpio_register_deferred_isr(&gpio_interface, &gpio_isr_request, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), NULL, &context);
    assert_int_equal(result, (uint32_t)KNG_E_INVALIDARG);

    gpio_interface.Device = &gpio_device;
    expect_value(__wrap_DfwkAsyncRequestInitialize, Request, &gpio_isr_request.Header);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(gpio_request_t));
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, &gpio_isr_request.Header);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, gpio_isr_callback);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, NULL);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &gpio_interface.Header);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, &gpio_isr_request.Header);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    result = gpio_register_deferred_isr(&gpio_interface, &gpio_isr_request, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 1), gpio_isr_callback, NULL);
    assert_int_equal(result, KNG_SUCCESS);
}
} // extern "C"