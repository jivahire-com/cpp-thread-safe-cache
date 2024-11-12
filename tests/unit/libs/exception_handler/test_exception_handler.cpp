//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_exception_handler.cpp
 * exception handler tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <crash_dump.h>
#include <exception_handler.h>   // for exception_handler, threadx_stack_error_handler
#include <exception_handler_i.h> // for exception_stack_frame_t
#include <kng_error.h>           // for KNG_CD_HARDFAULT_EXCEPTION
#include <stdint.h>              // for uint32_t, uintptr_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void crash_dump_wait_forever()
{
    function_called();
}

void save_crash_context(exception_stack_frame_t* stack_frame)
{
    check_expected_ptr(stack_frame);
    function_called();

    // Store registers from exception stack frame into crash context
    g_core_crash_context.r0 = stack_frame->R0;
    g_core_crash_context.r1 = stack_frame->R1;
    g_core_crash_context.r2 = stack_frame->R2;
    g_core_crash_context.r3 = stack_frame->R3;
    g_core_crash_context.r12 = stack_frame->R12;
    g_core_crash_context.lr = stack_frame->LR;
    g_core_crash_context.pc = stack_frame->PC;
}

int get_active_exception(void)
{
    function_called();
    return mock_type(int);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    check_expected(p1);
    check_expected(p2);
    check_expected(p3);
    check_expected(p4);
    function_called();
}

void __wrap_crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    check_expected(p1);
    check_expected(p2);
    check_expected(p3);
    check_expected(p4);
    function_called();
}

bool __wrap_crash_dump_bug_check_initiated_dump()
{
    function_called();
    return mock_type(bool);
}

//
// Tests
//
void test_exception_handler_params(int exception, uint32_t error_code)
{
    exception_stack_frame_t stack_frame;

    // Set up expectations
    expect_value(save_crash_context, stack_frame, &stack_frame);
    expect_function_call(save_crash_context);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, exception);

    expect_value(__wrap_crash_dump_handler, errorCode, error_code);
    expect_any(__wrap_crash_dump_handler, p1); // __FILE__
    expect_any(__wrap_crash_dump_handler, p2); // __LINE__
    expect_value(__wrap_crash_dump_handler, p3, 0);
    expect_value(__wrap_crash_dump_handler, p4, 0);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame);
}

TEST_FUNCTION(test_exception_handler, nullptr, nullptr)
{
    test_exception_handler_params(-13, KNG_CD_HARDFAULT_EXCEPTION);        // HardFault_IRQn
    test_exception_handler_params(-12, KNG_CD_MEMORYMANAGEMENT_EXCEPTION); // MemoryManagement_IRQn
    test_exception_handler_params(-11, KNG_CD_BUSFAULT_EXCEPTION);         // BusFault_IRQn
    test_exception_handler_params(-10, KNG_CD_USAGEFAULT_EXCEPTION);       // UsageFault_IRQn
    test_exception_handler_params(-14, KNG_CD_WDT_TIMEOUT);                // NonMaskableInt_IRQn
    test_exception_handler_params(0, KNG_CD_DEFAULT_EXCEPTION);            // Default
}

TEST_FUNCTION(test_exception_handler_bug_check, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_value(save_crash_context, stack_frame, &stack_frame);
    expect_function_call(save_crash_context);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -4); // DebugMonitor_IRQn

    expect_function_call(__wrap_crash_dump_bug_check_initiated_dump);
    will_return(__wrap_crash_dump_bug_check_initiated_dump, true);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_E_NOTIMPL);
    expect_value(__wrap_crash_dump_handler, p1, 1);
    expect_value(__wrap_crash_dump_handler, p2, 2);
    expect_value(__wrap_crash_dump_handler, p3, 3);
    expect_value(__wrap_crash_dump_handler, p4, 4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame);

    // Set up expectations
    expect_value(save_crash_context, stack_frame, &stack_frame);
    expect_function_call(save_crash_context);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -4); // DebugMonitor_IRQn

    expect_function_call(__wrap_crash_dump_bug_check_initiated_dump);
    will_return(__wrap_crash_dump_bug_check_initiated_dump, false);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_CD_EXTERNAL_REQUEST);
    expect_value(__wrap_crash_dump_handler, p1, 0);
    expect_value(__wrap_crash_dump_handler, p2, 0);
    expect_value(__wrap_crash_dump_handler, p3, 0);
    expect_value(__wrap_crash_dump_handler, p4, 0);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame);
}

TEST_FUNCTION(test_threadx_stack_error_handler, nullptr, nullptr)
{
    TX_THREAD tx_thread;

    // Set up expectations
    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_CD_THREAD_STACK_ERROR);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_value(__wrap_crash_dump_bug_check, p3, &tx_thread);
    expect_value(__wrap_crash_dump_bug_check, p4, 0);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    threadx_stack_error_handler(&tx_thread);
}
}