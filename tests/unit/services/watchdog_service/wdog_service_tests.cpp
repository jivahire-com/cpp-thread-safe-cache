//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file wdog_service_tests.cpp
 * Watchdog Service Tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <stddef.h>    // for NULL
#include <tx_api.h>    // for TX_AUTO_ACTIVATE, TX_SUCCESS
#include <wdog.h>      // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void wdog_heartbeat(uint32_t wdog_init_counter);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
// mocks for ThreadX (tx) APIs
UINT __wrap__tx_thread_sleep(ULONG timer_ticks)
{
    check_expected(timer_ticks);
    function_called();
    return TX_SUCCESS;
}

void __wrap_wdog_cmsdk_apb_reload()
{
    function_called();
}

int32_t __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                                  CHAR* name_ptr,
                                  VOID (*entry_function)(ULONG entry_input),
                                  ULONG entry_input,
                                  VOID* stack_start,
                                  ULONG stack_size,
                                  UINT priority,
                                  UINT preempt_threshold,
                                  ULONG time_slice,
                                  UINT auto_start)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(entry_function);
    check_expected(entry_input);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    check_expected(preempt_threshold);
    check_expected(time_slice);
    check_expected(auto_start);

    return mock_type(int32_t);
}

void __wrap_wdog_cmsdk_apb_init(uint32_t reload_timeout, bool reset_enable)
{
    check_expected(reload_timeout);
    check_expected(reset_enable);
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_wdog_service_init, nullptr, nullptr)
{
#define LOWEST_PRIORITY (TX_MAX_PRIORITIES - 1)

    expect_not_value(__wrap__txe_thread_create, thread_ptr, NULL);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_not_value(__wrap__txe_thread_create, entry_function, NULL);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_not_value(__wrap__txe_thread_create, stack_start, NULL);
    expect_not_value(__wrap__txe_thread_create, stack_size, 0);
    expect_value(__wrap__txe_thread_create, priority, LOWEST_PRIORITY);
    expect_value(__wrap__txe_thread_create, preempt_threshold, LOWEST_PRIORITY);
    expect_value(__wrap__txe_thread_create, time_slice, TX_NO_TIME_SLICE);
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    wdog_service_init(1, 1);
}

TEST_FUNCTION(test_wdog_heartbeat, nullptr, nullptr)
{
    expect_value(__wrap_wdog_cmsdk_apb_init, reset_enable, 1);
    expect_not_value(__wrap_wdog_cmsdk_apb_init, reload_timeout, 0);
    expect_function_call(__wrap_wdog_cmsdk_apb_init);

    expect_not_value(__wrap__tx_thread_sleep, timer_ticks, 0);
    expect_function_call(__wrap__tx_thread_sleep);

    wdog_heartbeat(1);
}

TEST_FUNCTION(test_wdog_heartbeat_reload, nullptr, nullptr)
{
    expect_function_call(__wrap_wdog_cmsdk_apb_reload);
    expect_not_value(__wrap__tx_thread_sleep, timer_ticks, 0);
    expect_function_call(__wrap__tx_thread_sleep);

    wdog_heartbeat(1);
}
}
