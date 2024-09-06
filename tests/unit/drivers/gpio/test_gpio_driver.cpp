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
#include "real_proto.h" // for __real_DfwkQueueEnqueueRequest

#include <DfwkCommon.h>     // for DfwkAsyncRequestInitialize, DfwkAsyncRe...
#include <DfwkSchedule.h>   // for DFWK_SCHEDULE
#include <FpFwLinkedList.h> // for FpFwListEntryInitialize, FpFwListInitialize
#include <FpFwLock.h>       // for FpFwLockInitialize
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <gpio.h>           // for gpio_request_type_idx, gpio_request_t
#include <gpio_lib.h>       // for MSCP_EXP_GPIO_4, MSCP_EXP_GPIO_6, GPIO_C...
#include <interrupts.h>     // for _SCP_IRQn_t
#include <kng_error.h>      // for KNG_E_INVALIDARG, KNG_STATUS, KNG_SUCCESS
#include <stdint.h>         // for uint32_t
#include <string.h>         // for NULL, memset, strcmp
#include <tx_api.h>         // for ULONG, TX_SUCCESS, TX_WAIT_FOREVER

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static gpio_irq_config_t gpio_irq_config[] = {
    {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
    {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
};
static gpio_device_t gpio_device;
static gpio_interface_t gpio_interface;
static DFWK_SCHEDULE schedule;

/*------------- Functions ----------------*/
//
// Mocks
//
bool __wrap_thread_loop()
{
    // return thread_loop_count-- > 0;
    return mock_type(bool); // return value is set by the test via will_return_count()
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    gpio_device.IrqConfig = gpio_irq_config;
    gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Set up device and interface
    FpFwLockInitialize(&gpio_device.Header.ReentrancyProtection);
    gpio_device.Header.Schedule = &schedule;
    FpFwListInitialize(&gpio_device.Queue.RequestQueue);
    FpFwListEntryInitialize(&gpio_device.Queue.ListEntry);
    FpFwLockInitialize(&gpio_device.Queue.Lock);
    FpFwListEntryInitialize(&gpio_device.Queue.ListEntry);
    gpio_device.Queue.QueueType = DfwkQueueType_SerializedDispatch;
    gpio_device.Queue.PauseCount = 0;
    gpio_device.Queue.Idle = true;
    gpio_device.Queue.OwningDevice = &gpio_device.Header;
    FpFwListInitialize(&gpio_device.IsrReqQueue.RequestQueue);
    FpFwListEntryInitialize(&gpio_device.IsrReqQueue.ListEntry);
    FpFwLockInitialize(&gpio_device.IsrReqQueue.Lock);
    FpFwListEntryInitialize(&gpio_device.IsrReqQueue.ListEntry);
    gpio_device.IsrReqQueue.QueueType = DfwkQueueType_ManualDispatch;
    gpio_device.IsrReqQueue.PauseCount = 0;
    gpio_device.IsrReqQueue.Idle = true;
    gpio_device.IsrReqQueue.OwningDevice = &gpio_device.Header;

    gpio_interface.Header.OwningDevice = &gpio_device.Header;
    gpio_interface.Header.DispatchQueue = &gpio_device.Queue;
    gpio_interface.Header.Opened = false;

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    memset(&gpio_device, 0, sizeof(gpio_device));
    memset(&gpio_interface, 0, sizeof(gpio_interface));

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_gpio_device_init, nullptr, nullptr)
{
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
    expect_function_call(__wrap_DfwkQueueInitialize); // expect default queue initialization

    expect_value(__wrap_DfwkQueueInitialize, Queue, &test_gpio_device.IsrReqQueue);
    expect_value(__wrap_DfwkQueueInitialize, Device, &test_gpio_device);
    expect_function_call(__wrap_DfwkQueueInitialize); // expect Isr request manual queue initialization

    expect_function_call(__wrap__txe_queue_create);  // expect queue creation for GPIO ISR
    expect_function_call(__wrap__txe_thread_create); // expect thread creation for GPIO ISR

    // Call API under test
    gpio_device_init(&test_gpio_device, &test_schedule);

    // Check expectations
    assert_int_equal(test_gpio_device.Header.ReentrancyProtection.Signature, 0xfeedf00d);
    assert_int_equal(test_gpio_device.Header.Schedule, &test_schedule);
}

TEST_FUNCTION(test_gpio_interface_init, nullptr, nullptr)
{
    gpio_device_t test_gpio_device;
    gpio_interface_t test_gpio_interface;
    test_gpio_device.IrqConfig = gpio_irq_config;
    test_gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    // Set up expectations
    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &test_gpio_interface);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &test_gpio_device.Header);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &test_gpio_device.Queue);
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchSync, NULL);
    expect_function_call(__wrap_DfwkInterfaceInitialize); // expect interface initialization

    for (unsigned int i = 0; i < sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]); i++)
    {
        expect_function_call(__wrap_nvic_irq_set_isr_with_param); // expect IRQ set
        expect_function_call(__wrap_nvic_irq_clear_pending);      // expect IRQ clear pending
        expect_function_call(__wrap_nvic_irq_enable);             // expect IRQ enable
    }

    // Call API under test
    gpio_interface_init(&test_gpio_interface, &test_gpio_device);

    // Check expectations
    assert_int_equal(test_gpio_interface.Device, &test_gpio_device);
    assert_int_equal(test_gpio_interface.Header.OwningDevice, &test_gpio_device);
}

extern void gpio_isr(void* context);

TEST_FUNCTION(test_gpio_isr, test_setup, test_teardown)
{
    uint32_t interrupt_status = 0x00000002;

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

    const uint32_t expected_message = ((interrupt_status << 16) & 0xFFFF0000) |
                                      (GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status) & 0x0000FFFF);

    // Check the message is properly constructed and sent to the queue
    expect_value(__wrap__txe_queue_send, *((uint32_t*)source_ptr), expected_message);
    expect_function_call(__wrap__txe_queue_send);

    // Make sure interrupts are cleared
    expect_value(__wrap_gpio_clear_interrupt_status, gpio_ctrl_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, interrupt_status));
    expect_function_call(__wrap_gpio_clear_interrupt_status);

    gpio_isr(&gpio_device);
}

static void gpio_isr_callback(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);
    pgpio_request_t gpio_request = (pgpio_request_t)Request;

    check_expected_ptr(Request);
    check_expected(gpio_request->gpio_pin_id);
    check_expected(gpio_request->level);
    function_called();
}

extern void gpio_isr_callback_thread_main(ULONG thread_input);

TEST_FUNCTION(test_gpio_isr_callback_thread_main_id, test_setup, test_teardown)
{
    // Set up requests
    gpio_request_t requests[] = {
        {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 1), .level = 0},
        {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0x05), .level = 0}};

    for (unsigned int i = 0; i < sizeof(requests) / sizeof(requests[0]); i++)
    {
        DfwkAsyncRequestInitialize(&requests[i].Header, sizeof(gpio_request_t));
        DfwkAsyncRequestSetCompletionRoutine(&requests[i].Header, gpio_isr_callback, NULL);
    }

    __real_DfwkQueueEnqueueRequest(&gpio_device.IsrReqQueue, &requests[0].Header);
    __real_DfwkQueueEnqueueRequest(&gpio_device.IsrReqQueue, &requests[1].Header);

    will_return(__wrap_thread_loop, true); // loop once

    // Set up expectations
    ULONG level = 0x00000002;
    ULONG gpio_ctrl_pin_id = GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0x02);
    ULONG message = ((level << 16) & 0xFFFF0000) | (gpio_ctrl_pin_id & 0x0000FFFF);
    will_return(generate_message, message);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);
    expect_function_call(__wrap__txe_queue_receive);

    expect_function_call(__wrap_DfwkQueueDequeueRequest);          // Get GPIO4:1 request
    expect_function_call(__wrap_DfwkAsyncRequestComplete);         // Expect completion of GPIO4:1
    expect_value(gpio_isr_callback, Request, &requests[0].Header); // Expect completion of GPIO4:1
    expect_value(gpio_isr_callback, gpio_request->gpio_pin_id, GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 1));
    expect_value(gpio_isr_callback, gpio_request->level, 1);
    expect_function_call(gpio_isr_callback);
    expect_function_call(__wrap_DfwkQueueDequeueRequest); // Get GPIO6:0,2 request
    expect_function_call(__wrap_DfwkQueueEnqueueRequest); // Enqueue barrier request
    expect_function_call(__wrap_DfwkQueueEnqueueRequest); // Re-enqueue GPIO6:0,2 request
    expect_function_call(__wrap_DfwkQueueDequeueRequest); // Expect dequeue of barrier request

    will_return(__wrap_thread_loop, false); // break loop after test

    gpio_isr_callback_thread_main((ULONG)&gpio_device);
}

TEST_FUNCTION(test_gpio_isr_callback_thread_main_mask, test_setup, test_teardown)
{
    // Set up requests
    gpio_request_t requests[] = {
        {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 1), .level = 0},
        {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC}, .gpio_pin_id = GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0x05), .level = 0}};

    for (unsigned int i = 0; i < sizeof(requests) / sizeof(requests[0]); i++)
    {
        DfwkAsyncRequestInitialize(&requests[i].Header, sizeof(gpio_request_t));
        DfwkAsyncRequestSetCompletionRoutine(&requests[i].Header, gpio_isr_callback, NULL);
    }

    __real_DfwkQueueEnqueueRequest(&gpio_device.IsrReqQueue, &requests[0].Header);
    __real_DfwkQueueEnqueueRequest(&gpio_device.IsrReqQueue, &requests[1].Header);

    will_return(__wrap_thread_loop, true); // loop once

    // Set up expectations
    ULONG level = 0x00000005;
    ULONG gpio_ctrl_pin_id = GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0x01);
    ULONG message = ((level << 16) & 0xFFFF0000) | (gpio_ctrl_pin_id & 0x0000FFFF);
    will_return(generate_message, message);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);
    expect_function_call(__wrap__txe_queue_receive);

    expect_function_call(__wrap_DfwkQueueDequeueRequest); // Get GPIO4:1 request
    expect_function_call(__wrap_DfwkQueueEnqueueRequest); // Enqueue barrier request
    expect_function_call(__wrap_DfwkQueueEnqueueRequest); // Re-enqueue GPIO4:1 request

    expect_function_call(__wrap_DfwkQueueDequeueRequest);          // Get GPIO6:0,2 request
    expect_function_call(__wrap_DfwkAsyncRequestComplete);         // Expect completion of GPIO6:0
    expect_value(gpio_isr_callback, Request, &requests[1].Header); // Expect completion of GPIO6:0
    expect_value(gpio_isr_callback, gpio_request->gpio_pin_id, GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0x05));
    expect_value(gpio_isr_callback, gpio_request->level, 0x05);
    expect_function_call(gpio_isr_callback);

    expect_function_call(__wrap_DfwkQueueDequeueRequest); // Expect dequeue of barrier request

    will_return(__wrap_thread_loop, false); // break loop after test

    gpio_isr_callback_thread_main((ULONG)&gpio_device);
}

TEST_FUNCTION(test_gpio_cmd_async, test_setup, test_teardown)
{
    gpio_request_t gpio_request = {.Header = {.RequestType = GPIO_REQUEST_ISR_ASYNC},
                                   .gpio_pin_id = GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_4, 1),
                                   .level = 0};

    // Interface is not ready yet.
    expect_string(__wrap_fpfw_init_get_handle, id, "gpio_int");
    expect_function_call(__wrap_fpfw_init_get_handle);
    will_return(__wrap_fpfw_init_get_handle, nullptr);
    KNG_STATUS ret = gpio_cmd_async(&gpio_request);
    assert_int_equal(ret, KNG_E_NOINTERFACE);

    // Test without completion routine
    expect_string(__wrap_fpfw_init_get_handle, id, "gpio_int");
    expect_function_call(__wrap_fpfw_init_get_handle);
    will_return(__wrap_fpfw_init_get_handle, &gpio_interface);
    ret = gpio_cmd_async(&gpio_request);
    assert_int_equal(ret, KNG_E_INVALIDARG);

    // Configure request properly.
    DfwkAsyncRequestInitialize(&gpio_request.Header, sizeof(gpio_request_t));
    DfwkAsyncRequestSetCompletionRoutine(&gpio_request.Header, gpio_isr_callback, NULL);

    expect_function_call(__wrap_DfwkQueueEnqueueRequest);

    ret = gpio_cmd_async(&gpio_request);
    assert_int_equal(ret, KNG_SUCCESS);

    // Test with invalid request
    gpio_request.Header.RequestType = GPIO_REQUEST_COUNT + 1;
    ret = gpio_cmd_async(&gpio_request);
    assert_int_equal(ret, KNG_E_INVALIDARG);
}

} // extern "C"