//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_exception_handler_init.cpp
 * Tests the init of the exception handler and exception handler itself
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>              // for FPFW_UNUSED
#include <fpfw_init.h>              // for fpfw_init_component_t
#include <nvic.h>                   // for nvic_status_t, nvic_set_isr_fault
#include <tx_api.h>                 // for TX_SUCCESS, TX_THREAD

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_excp_hnd;

/*------------- Functions ----------------*/
//
// Mocks
//
nvic_status_t __wrap_nvic_set_isr_fault(isr_callback_fn_sans_params_t isr)
{
    check_expected_ptr(isr);
    function_called();

    return mock_type(nvic_status_t);
}

UINT __wrap__tx_thread_stack_error_notify(VOID (*stack_error_handler)(TX_THREAD *thread_ptr))
{
    check_expected_ptr(stack_error_handler);
    function_called();

    return mock_type(UINT);
}

//
// Tests
//

TEST_FUNCTION(test_exception_handler_init, NULL, NULL)
{
    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_excp_hnd.children[0]);
    assert_string_equal("nvic", _fpfw_component_excp_hnd.children[1]);

    // Set up expectations
    expect_any(__wrap_nvic_set_isr_fault, isr);
    expect_function_call(__wrap_nvic_set_isr_fault);    // expect general exception handler to be set
    will_return(__wrap_nvic_set_isr_fault, NVIC_STATUS_SUCCESS);

    expect_any(NVIC_SetVector, isr);
    expect_function_call(NVIC_SetVector);               // expect DebugMonitor_IRQn handler to be set

    expect_any(__wrap__tx_thread_stack_error_notify, stack_error_handler);
    expect_function_call(__wrap__tx_thread_stack_error_notify);    // expect stack error handler to be set
    will_return(__wrap__tx_thread_stack_error_notify, TX_SUCCESS);

    // Call API under test
    _fpfw_component_excp_hnd.init_fn();
}
}