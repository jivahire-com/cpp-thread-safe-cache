//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_exec_cmpnt.cpp
 * Test entry into in band component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <exec_tlm_cmpnt.h>
#include <exec_tlm_cmpnt_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <tx_api.h>
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

TEST_FUNCTION(test_exec_tlm_cmpnt_disable_data_collection, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_NOMINAL;

    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    exec_tlm_cmpnt_disable_data_collection();

    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_DISABLED);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_get_status, test_setup, test_teardown)
{

    tlm_executive_status.op_mode = TLM_OP_MODE_NOMINAL;
    tlm_executive_status.pwr_pkg_period_ms = 10;
    tlm_executive_status.inst_pkg_sample_period_ms = 20;
    tlm_executive_status.pwr_aggr_period_ms = 30;
    tlm_executive_status.pwr_pkg_timer_active = true;
    tlm_executive_status.inst_sample_timer_active = true;
    tlm_executive_status.pwr_aggr_timer_active = true;
    tlm_executive_status.twenty_four_hr_pkg_timer_active = true;

    will_return(__wrap__txe_timer_info_get, TX_FALSE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_FALSE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_FALSE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_FALSE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    telemetry_executive_status_t status_out;
    exec_tlm_cmpnt_get_status(&status_out);

    assert_int_equal(status_out.op_mode, TLM_OP_MODE_NOMINAL);
    assert_int_equal(status_out.pwr_pkg_period_ms, 10);
    assert_int_equal(status_out.inst_pkg_sample_period_ms, 20);
    assert_int_equal(status_out.pwr_aggr_period_ms, 30);
    assert_false(status_out.pwr_pkg_timer_active);
    assert_false(status_out.inst_sample_timer_active);
    assert_false(status_out.pwr_aggr_timer_active);
    assert_false(status_out.twenty_four_hr_pkg_timer_active);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_get_status_2, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;
    tlm_executive_status.pwr_pkg_period_ms = 100;
    tlm_executive_status.inst_pkg_sample_period_ms = 200;
    tlm_executive_status.pwr_aggr_period_ms = 300;
    tlm_executive_status.pwr_pkg_timer_active = false;
    tlm_executive_status.inst_sample_timer_active = false;
    tlm_executive_status.pwr_aggr_timer_active = false;
    tlm_executive_status.twenty_four_hr_pkg_timer_active = false;

    will_return(__wrap__txe_timer_info_get, TX_TRUE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_TRUE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_TRUE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    will_return(__wrap__txe_timer_info_get, TX_TRUE);
    will_return(__wrap__txe_timer_info_get, TX_SUCCESS);

    telemetry_executive_status_t status_out;
    exec_tlm_cmpnt_get_status(&status_out);

    assert_int_equal(status_out.op_mode, TLM_OP_MODE_DISABLED);
    assert_int_equal(status_out.pwr_pkg_period_ms, 100);
    assert_int_equal(status_out.inst_pkg_sample_period_ms, 200);
    assert_int_equal(status_out.pwr_aggr_period_ms, 300);
    assert_true(status_out.pwr_pkg_timer_active);
    assert_true(status_out.inst_sample_timer_active);
    assert_true(status_out.pwr_aggr_timer_active);
    assert_true(status_out.twenty_four_hr_pkg_timer_active);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_init, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);

    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 6);

    exec_tlm_cmpnt_init(1000, 100);

    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 1000);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 100);
}

TEST_FUNCTION(test_pwr_aggr_timer_cb, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, PWR_AGGR_TMR_EXPIRED);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    pwr_aggr_timer_cb(0);
}

TEST_FUNCTION(test_inst_sample_timer_cb, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, INST_SAMPLE_TMR_EXPIRED);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    inst_sample_timer_cb(0);
}

TEST_FUNCTION(test_pwr_pkg_timer_cb, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, PWR_PKG_TMR_EXPIRED);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    pwr_pkg_timer_cb(0);
}

TEST_FUNCTION(test_every_24hr_pkg_timer_cb, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, EVERY_24HR_PKG_TMR_EXPIRED);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    every_24hr_pkg_timer_cb(0);
}
