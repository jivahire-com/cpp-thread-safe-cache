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

extern tlm_operating_mode_t pending_mode_change;
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

TEST_FUNCTION(test_exec_tlm_cmpnt_change_telemetry_mode_disable, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_PUBLISHING;

    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    expect_function_call(in_band_tlm_cmpnt_tlm_mode_exit_actions);
    expect_function_call(in_band_tlm_cmpnt_tlm_mode_enter_actions);

    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_DISABLED);

    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_DISABLED);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_change_telemetry_mode_enable_with_inst, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;
    tlm_executive_status.inst_pkg_sample_period_ms = 40;

    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    expect_function_call(in_band_tlm_cmpnt_tlm_mode_exit_actions);
    expect_function_call(in_band_tlm_cmpnt_tlm_mode_enter_actions);

    will_return(in_band_tlm_cmpnt_is_any_instantaneous_enabled, true);

    will_return(__wrap__txe_timer_activate, TX_SUCCESS);
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_PUBLISHING);

    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_PUBLISHING);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_change_telemetry_mode_enable_without_inst, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;
    tlm_executive_status.inst_pkg_sample_period_ms = 40;

    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    expect_function_call(in_band_tlm_cmpnt_tlm_mode_exit_actions);
    expect_function_call(in_band_tlm_cmpnt_tlm_mode_enter_actions);

    will_return(in_band_tlm_cmpnt_is_any_instantaneous_enabled, false);

    will_return(__wrap__txe_timer_activate, TX_SUCCESS);
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);

    exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_PUBLISHING);

    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_PUBLISHING);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_change_telemetry_mode_fail, test_setup, test_teardown)
{
    // no processing of mode change if already in the same mode
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;
    exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_DISABLED);
    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_DISABLED);

    tlm_executive_status.op_mode = TLM_OP_MODE_SENSOR_FIFO_RAW_DATA;
    exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_PUBLISHING);
    assert_int_equal(tlm_executive_status.op_mode, TLM_OP_MODE_SENSOR_FIFO_RAW_DATA);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_get_status, test_setup, test_teardown)
{

    tlm_executive_status.op_mode = TLM_OP_MODE_PUBLISHING;
    tlm_executive_status.pwr_pkg_period_ms = 10;
    tlm_executive_status.inst_pkg_sample_period_ms = 20;
    tlm_executive_status.data_aggr_period_ms = 30;
    tlm_executive_status.pwr_pkg_timer_active = true;
    tlm_executive_status.inst_sample_timer_active = true;
    tlm_executive_status.data_aggr_timer_active = true;
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

    assert_int_equal(status_out.op_mode, TLM_OP_MODE_PUBLISHING);
    assert_int_equal(status_out.pwr_pkg_period_ms, 10);
    assert_int_equal(status_out.inst_pkg_sample_period_ms, 20);
    assert_int_equal(status_out.data_aggr_period_ms, 30);
    assert_false(status_out.pwr_pkg_timer_active);
    assert_false(status_out.inst_sample_timer_active);
    assert_false(status_out.data_aggr_timer_active);
    assert_false(status_out.twenty_four_hr_pkg_timer_active);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_get_status_2, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;
    tlm_executive_status.pwr_pkg_period_ms = 100;
    tlm_executive_status.inst_pkg_sample_period_ms = 200;
    tlm_executive_status.data_aggr_period_ms = 300;
    tlm_executive_status.pwr_pkg_timer_active = false;
    tlm_executive_status.inst_sample_timer_active = false;
    tlm_executive_status.data_aggr_timer_active = false;
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
    assert_int_equal(status_out.data_aggr_period_ms, 300);
    assert_true(status_out.pwr_pkg_timer_active);
    assert_true(status_out.inst_sample_timer_active);
    assert_true(status_out.data_aggr_timer_active);
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

    exec_tlm_cmpnt_init(1, 1000, 100, 86000);

    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 1000);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 100);
}

TEST_FUNCTION(test_data_aggr_timer_cb, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, DATA_AGGR_TMR_EXPIRED);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    data_aggr_timer_cb(0);
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

TEST_FUNCTION(test_exec_tlm_cmpnt_notify_new_in_band_mts_message, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_INBAND_MTS_MESSAGE);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    exec_tlm_cmpnt_notify_new_in_band_mts_message();
}

// test exec_tlm_cmpnt_notify_new_in_band_mts_message
TEST_FUNCTION(test_exec_tlm_cmpnt_notify_new_out_of_band_pldm_request, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUT_OF_BAND_PLDM_REQ);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    exec_tlm_cmpnt_notify_new_out_of_band_pldm_request();
}

TEST_FUNCTION(test_exec_tlm_cmpnt_notify_new_mode_change, test_setup, test_teardown)
{
    pending_mode_change = TLM_OP_MODE_PUBLISHING;
    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, MODE_CHANGE_PENDING);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);

    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    exec_tlm_cmpnt_set_mode_change_pending(TLM_OP_MODE_SENSOR_FIFO_RAW_DATA);
    assert_int_equal(pending_mode_change, TLM_OP_MODE_SENSOR_FIFO_RAW_DATA);
}

TEST_FUNCTION(test_exec_tlm_cmpnt_is_telemetry_publishing_enabled, test_setup, test_teardown)
{
    tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;

    bool enabled = exec_tlm_cmpnt_is_telemetry_publishing_enabled();

    assert_false(enabled);

    tlm_executive_status.op_mode = TLM_OP_MODE_PUBLISHING;
    enabled = exec_tlm_cmpnt_is_telemetry_publishing_enabled();
    assert_true(enabled);
}

TEST_FUNCTION(test_exec_tlm_get_timestamp_microseconds, test_setup, test_teardown)
{

    uint64_t timestamp = 0;
    // Set up mock return values for gtimer_prodfw_get_counter and gtimer_prodfw_get_frequency
    will_return(__wrap_gtimer_prodfw_get_counter, 5000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000000);
    // Call the function to be tested
    timestamp = exec_tlm_cmpnt_get_timestamp_microseconds();
    // Add assertions to verify the expected behavior
    assert_int_equal(timestamp, 5);
}

TEST_FUNCTION(test_exec_tlm_run_timer_enter_actions, test_setup, test_teardown)
{
    will_return(__wrap__txe_timer_activate, TX_SUCCESS);
    run_timer_enter_actions(TLM_OP_MODE_COLLECTING_DATA);

    // no entry actions
    run_timer_enter_actions(TLM_OP_MODE_SENSOR_FIFO_RAW_DATA);

    // test invalid mode
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    run_timer_enter_actions((tlm_operating_mode_t)(TLM_OP_MODE_SENSOR_FIFO_RAW_DATA + 10));
}

TEST_FUNCTION(test_exec_tlm_run_timer_exit_actions, test_setup, test_teardown)
{
    // no exit actions
    run_timer_exit_actions(TLM_OP_MODE_COLLECTING_DATA);
    run_timer_exit_actions(TLM_OP_MODE_SENSOR_FIFO_RAW_DATA);

    // test invalid mode
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    run_timer_exit_actions((tlm_operating_mode_t)(TLM_OP_MODE_SENSOR_FIFO_RAW_DATA + 10));
}

TEST_FUNCTION(test_exec_tlm_update_timers_fail, test_setup, test_teardown)
{
    tlm_executive_status.data_aggr_period_ms = 4;
    tlm_executive_status.inst_pkg_sample_period_ms = 5;
    tlm_executive_status.pwr_pkg_period_ms = 6;
    tlm_executive_status.twenty_four_hr_pkg_period_ms = 7;

    exec_tlm_cmpnt_udpdate_timer_periods(0, 200, 200, 200);

    assert_int_equal(tlm_executive_status.data_aggr_period_ms, 4);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 5);
    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 6);
    assert_int_equal(tlm_executive_status.twenty_four_hr_pkg_period_ms, 7);

    exec_tlm_cmpnt_udpdate_timer_periods(300, 0, 300, 300);

    assert_int_equal(tlm_executive_status.data_aggr_period_ms, 4);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 5);
    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 6);
    assert_int_equal(tlm_executive_status.twenty_four_hr_pkg_period_ms, 7);

    exec_tlm_cmpnt_udpdate_timer_periods(400, 400, 0, 400);

    assert_int_equal(tlm_executive_status.data_aggr_period_ms, 4);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 5);
    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 6);
    assert_int_equal(tlm_executive_status.twenty_four_hr_pkg_period_ms, 7);

    exec_tlm_cmpnt_udpdate_timer_periods(500, 500, 500, 0);

    assert_int_equal(tlm_executive_status.data_aggr_period_ms, 4);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 5);
    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 6);
    assert_int_equal(tlm_executive_status.twenty_four_hr_pkg_period_ms, 7);
}

TEST_FUNCTION(test_exec_tlm_update_timers_success, test_setup, test_teardown)
{
    tlm_executive_status.data_aggr_period_ms = 4;
    tlm_executive_status.inst_pkg_sample_period_ms = 5;
    tlm_executive_status.pwr_pkg_period_ms = 6;
    tlm_executive_status.twenty_four_hr_pkg_period_ms = 7;

    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);

    exec_tlm_cmpnt_udpdate_timer_periods(200, 201, 202, 203);

    assert_int_equal(tlm_executive_status.data_aggr_period_ms, 200);
    assert_int_equal(tlm_executive_status.inst_pkg_sample_period_ms, 201);
    assert_int_equal(tlm_executive_status.pwr_pkg_period_ms, 202);
    assert_int_equal(tlm_executive_status.twenty_four_hr_pkg_period_ms, 203);
}

TEST_FUNCTION(test_exec_tlm_update_timers_tx_fail, test_setup, test_teardown)
{
    tlm_executive_status.data_aggr_period_ms = 4;
    tlm_executive_status.inst_pkg_sample_period_ms = 5;
    tlm_executive_status.pwr_pkg_period_ms = 6;
    tlm_executive_status.twenty_four_hr_pkg_period_ms = 7;

    will_return(__wrap__txe_timer_change, TX_TIMER_ERROR);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);

    exec_tlm_cmpnt_udpdate_timer_periods(200, 201, 202, 203);

    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_TIMER_ERROR);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);

    exec_tlm_cmpnt_udpdate_timer_periods(200, 201, 202, 203);

    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_TIMER_ERROR);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);

    exec_tlm_cmpnt_udpdate_timer_periods(200, 201, 202, 203);

    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_SUCCESS);
    will_return(__wrap__txe_timer_change, TX_TIMER_ERROR);

    exec_tlm_cmpnt_udpdate_timer_periods(200, 201, 202, 203);
}