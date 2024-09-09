//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_remote_die_test.cpp
 * Power service remote die tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {
#include "corebits.h"   // for corebits_t
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h>     // for expect_any_always, CmockaWrapperTest
#include <assert.h>            // for assert
#include <cstddef>             // for NULL, size_t
#include <fpfw_icc_base.h>
#include <fpfw_status.h>       // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <mock_bug_check.h>    // for __wrap_crash_dump_bug_check
#include <power_i.h>           // for power_fuses_get_dts_coeff, power_fuse...
#include <power_remote_die_i.h>
#include <power_runconfig.h>   // for power_fuse_data_t, dts_coeff_t, power...
#include <power_runconfig_i.h> // for power_fuses_read, power_fuses_get_cur...
#include <stdint.h>            // for int8_t, uint64_t, uint8_t, uintptr_t
#include <string.h>            // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void __real_power_remote_die_idle_reset();
void __real_power_remote_die_init(power_runconfig_t* p_runconfig);
void __real_power_remote_die_exchange_inputs(power_runconfig_t* p_runconfig);
void __real_power_remote_die_exchange_complete(power_runconfig_t* p_runconfig);

/*-- Declarations (Statics and globals) --*/

static power_runconfig_t s_test_power_runconfig = {};
static power_service_config_t s_test_power_service_config = {};
static icc_base_recv_complete_notify s_stored_recv_cb = NULL;                   
static icc_base_send_complete_notify s_stored_send_cb = NULL;
static void* s_stored_send_context = NULL;
static void* s_stored_recv_context = NULL;
static uint32_t* s_stored_send_payload_buffer = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t *icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected_ptr(icc_ctx);
    check_expected_ptr(params);

    // save the passed in callback
    s_stored_recv_cb = params->cb;
    // only want to store the first one
    if (s_stored_recv_context == NULL) {
        s_stored_recv_context = params->cb_ctx;
    }

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t *icc_ctx, fpfw_icc_base_send_req_t* params)
{
    check_expected_ptr(icc_ctx);
    check_expected_ptr(params);

    // save the passed in callback
    s_stored_send_cb = params->cb;
    s_stored_send_payload_buffer = (uint32_t*)params->payload_buffer;
    s_stored_send_context = params->cb_ctx;

    return mock_type(fpfw_status_t);
}

uint32_t __wrap_cortex_m7_atomic_or(volatile uint32_t* address, uint32_t value)
{
    function_called();
    *address |= value;
    return *address;
}

// end mocks
} // extern "C"

static int setup(void** state)
{
    UNUSED(state);
    /* some tests will modify the module context */

    s_test_power_runconfig.p_sconfig = &s_test_power_service_config;
    s_test_power_service_config.platform_is_multi_die = false;
    s_test_power_service_config.icc_d2d_ctx = NULL;

    // use power idle reset to clear internal state
    __real_power_remote_die_idle_reset();

    return 0;
}

static int teardown(void** state)
{
    UNUSED(state);

    return 0;
}

void set_expectations_for_recv_request(void *icc_ctx)
{
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, icc_ctx);
    expect_not_value(__wrap_fpfw_icc_base_recv, params, NULL);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
}

void set_expectations_for_send_request(void *icc_ctx)
{
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, icc_ctx);
    expect_not_value(__wrap_fpfw_icc_base_send, params, NULL);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
}

void set_expectations_for_send_complete_signal(int signal)
{
    expect_any(__wrap_power_loops_handle_event, context);
    expect_value(__wrap_power_loops_handle_event, event, signal);
    expect_any(__wrap_power_loops_handle_event, event_data);
}

POWER_TEST(remote_die_init__single_die, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    // nothing should happen because platform is single die
    __real_power_remote_die_init(&s_test_power_runconfig);
}

// test value to use for ICC ctx
#define TEST_ICC_D2D_CTX 0xFFFF1224
void setup_multi_die()
{
    // indicate platform is multi die
    s_test_power_service_config.platform_is_multi_die = true;
    s_test_power_service_config.icc_d2d_ctx = (void*)TEST_ICC_D2D_CTX;
}

POWER_TEST(remote_die_init__dual_die, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    
    setup_multi_die();
    
    // expect two calls to setup_recv_request
    set_expectations_for_recv_request((void*)TEST_ICC_D2D_CTX);
    set_expectations_for_recv_request((void*)TEST_ICC_D2D_CTX);

    __real_power_remote_die_init(&s_test_power_runconfig);
}

POWER_TEST(remote_die_idle_reset, setup, teardown)
{
    // expect no observable change
    __real_power_remote_die_idle_reset();
}

POWER_TEST(remote_die_exchange_inputs__single_die, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    // signal should be sent immediately
    set_expectations_for_send_complete_signal(POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS);
    __real_power_remote_die_exchange_inputs(&s_test_power_runconfig);
}

POWER_TEST(remote_die_exchange_inputs__multi_die, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    
    setup_multi_die();

    // expect a call to icc send
    set_expectations_for_send_request((void*)TEST_ICC_D2D_CTX);
    // no immediate signal when we call the exchange function
    expect_function_call(__wrap_cortex_m7_atomic_or);
    __real_power_remote_die_exchange_inputs(&s_test_power_runconfig);
    
    // expect we need a send callback
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_function_call(__wrap_cortex_m7_atomic_or);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);

    // signal will be sent after we also receive a recv callback
    set_expectations_for_send_complete_signal(POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS);
    // also expect recv will be called again
    expect_function_call(__wrap_cortex_m7_atomic_or);
    set_expectations_for_recv_request((void*)TEST_ICC_D2D_CTX);

    expect_value(__wrap_FpFwAssert, expression, true);
    s_stored_recv_cb(s_stored_recv_context, 0, FPFW_STATUS_SUCCESS);
}

POWER_TEST(remote_die_exchange_inputs__multi_die__send_started, setup, teardown)
{
    setup_multi_die();

    // expect a call to icc send
    set_expectations_for_send_request((void*)TEST_ICC_D2D_CTX);
    // no immediate signal when we call the exchange function
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_function_call(__wrap_cortex_m7_atomic_or);
    __real_power_remote_die_exchange_inputs(&s_test_power_runconfig);
    
    // if no send callback, then a later call to exchange inputs should not send again
    // no send_request, no atomic_or
    expect_value(__wrap_FpFwAssert, expression, true);
    __real_power_remote_die_exchange_inputs(&s_test_power_runconfig);
    
    // send a callback to clear this condition (no other way to clear it)
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_function_call(__wrap_cortex_m7_atomic_or);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);

}

POWER_TEST(remote_die_exchange_inputs__multi_die__recv_first, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    
    setup_multi_die();

    // recv callback called first will be called again
    expect_function_call(__wrap_cortex_m7_atomic_or);
    set_expectations_for_recv_request((void*)TEST_ICC_D2D_CTX);

    expect_value(__wrap_FpFwAssert, expression, true);
    s_stored_recv_cb(s_stored_recv_context, 0, FPFW_STATUS_SUCCESS);
    
    // then exchange inputs call
    set_expectations_for_send_request((void*)TEST_ICC_D2D_CTX);
    // still no immediate signal when we call the exchange function
    expect_function_call(__wrap_cortex_m7_atomic_or);
    __real_power_remote_die_exchange_inputs(&s_test_power_runconfig);
    
    // expect we need a send callback
    // signal will be sent after we set send callback
    set_expectations_for_send_complete_signal(POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS);

    expect_value(__wrap_FpFwAssert, expression, true);
    expect_function_call(__wrap_cortex_m7_atomic_or);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
    
}


POWER_TEST(remote_die_exchange_complete__single_die, setup, teardown)
{
    expect_value(__wrap_FpFwAssert, expression, true);
    // signal should be sent immediately
    set_expectations_for_send_complete_signal(POWER_CTRL_LOOP_SIGNAL_EXCHANGE_COMPLETE);
    __real_power_remote_die_exchange_complete(&s_test_power_runconfig);
}