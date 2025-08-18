//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_data_proc_cmpnt.cpp
 * This test file is used to test the data processing telemetry component.
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <tlm_fuses.h>

extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
bool data_proc_snsr_fifo_is_empty[SENSOR_FIFO_MAX_ID] = {0};
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
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

TEST_FUNCTION(test_data_proc_tlm_cmpnt_init, test_setup, test_teardown)
{
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, count, sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_tlm_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_core_info_get_enable_cores_result, 0x00);

    data_proc_tlm_cmpnt_init(1);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg, test_setup, test_teardown)
{

    die_2_die_exch_init(0);
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    die_2_die_exch_init(1);
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg, test_setup, test_teardown)
{

    static uint64_t expected_droop_counts[NUMBER_OF_CORES_PER_DIE];
    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        expected_droop_counts[i] = i * 10;
    }

    for (uint8_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        data_proc_snsr_fifo_is_empty[i] = true;
    }
    will_return(__wrap_sensor_fifo_svc_is_empty, data_proc_snsr_fifo_is_empty);
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);
    data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_tlm_mode_enter_actions, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
    data_proc_tlm_cmpnt_tlm_mode_enter_actions(TLM_OP_MODE_PUBLISHING);

    data_proc_tlm_cmpnt_tlm_mode_enter_actions(TLM_OP_MODE_DISABLED);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_prepare_data_for_inst_sample, test_setup, test_teardown)
{
    // expand unit test once implementation is available
    data_proc_tlm_cmpnt_prepare_data_for_inst_sample();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg, test_setup, test_teardown)
{
    // expand unit test once implementation is available
    data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg();
}

TEST_FUNCTION(test_data_smpl_reset_residency_timestamps, test_setup, test_teardown)
{
    // Initialize all cores with non-zero timestamps
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].cstate_res_timestamp_uS = 2000;
        core_rt[core_id].pstate_res_timestamp_uS = 3000;

        // Initialize throttle timestamps
        for (uint8_t throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
        {
            core_rt[core_id].throttle_res_timestamp_uS[throttle_source] = 4000;
        }

        // Initialize rack priority timestamps
        for (uint8_t priority = 0; priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; priority++)
        {
            core_rt[core_id].rack_pri_res_timestamp_uS[priority] = 5000;
        }
    }

    // Test Case 1: CState and PState validity (original test case)
    core_rt[3].status_flags.pkt_cstate_is_valid = true;
    core_rt[4].status_flags.pkt_pstate_is_valid = true;

    // Test Case 2: Throttle active with specific throttle sources
    core_rt[1].status_flags.throttle_is_active = true;
    core_rt[1].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;
    core_rt[1].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    // Leave other throttle sources as false to test the inner loop condition

    // Test Case 3: Rack throttle active
    core_rt[2].status_flags.rack_throttle_is_active = true;
    core_rt[2].latest_rack_throttle_priority = 3;

    // Test Case 4: Core with multiple flags active
    core_rt[5].status_flags.pkt_cstate_is_valid = true;
    core_rt[5].status_flags.pkt_pstate_is_valid = true;
    core_rt[5].status_flags.throttle_is_active = true;
    core_rt[5].status_flags.rack_throttle_is_active = true;
    core_rt[5].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] = true;
    core_rt[5].latest_rack_throttle_priority = 1;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10000);
    data_smpl_reset_residency_timestamps();

    // Verify results for all cores
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        // Test CState timestamp updates
        if (core_id == 3 || core_id == 5) // Cores with pkt_cstate_is_valid = true
        {
            assert_int_equal(core_rt[core_id].cstate_res_timestamp_uS, 10000);
        }
        else
        {
            assert_int_equal(core_rt[core_id].cstate_res_timestamp_uS, 2000);
        }

        // Test PState timestamp updates
        if (core_id == 4 || core_id == 5) // Cores with pkt_pstate_is_valid = true
        {
            assert_int_equal(core_rt[core_id].pstate_res_timestamp_uS, 10000);
        }
        else
        {
            assert_int_equal(core_rt[core_id].pstate_res_timestamp_uS, 3000);
        }
    }

    // Test throttle timestamp updates for core 1
    assert_int_equal(core_rt[1].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE], 10000);
    assert_int_equal(core_rt[1].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT], 10000);
    // Other throttle sources should remain unchanged
    assert_int_equal(core_rt[1].throttle_res_timestamp_uS[THROTTLE_SOURCE_RACK_LIMIT], 4000);
    assert_int_equal(core_rt[1].throttle_res_timestamp_uS[THROTTLE_SOURCE_VR_HOT], 4000);

    // Test rack throttle timestamp updates for core 2
    assert_int_equal(core_rt[2].rack_pri_res_timestamp_uS[3], 10000); // Priority 3 was set as latest
    // Other priorities should remain unchanged
    assert_int_equal(core_rt[2].rack_pri_res_timestamp_uS[0], 5000);
    assert_int_equal(core_rt[2].rack_pri_res_timestamp_uS[1], 5000);
    assert_int_equal(core_rt[2].rack_pri_res_timestamp_uS[2], 5000);

    // Test core 5 with multiple flags (all should be updated)
    assert_int_equal(core_rt[5].cstate_res_timestamp_uS, 10000);
    assert_int_equal(core_rt[5].pstate_res_timestamp_uS, 10000);
    assert_int_equal(core_rt[5].throttle_res_timestamp_uS[THROTTLE_SOURCE_ADAPTIVE_CLK], 10000);
    assert_int_equal(core_rt[5].rack_pri_res_timestamp_uS[1], 10000);
    // Other throttle sources for core 5 should remain unchanged
    assert_int_equal(core_rt[5].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE], 4000);

    // Test a core with no flags set (core 0) - nothing should change
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 2000);
    assert_int_equal(core_rt[0].pstate_res_timestamp_uS, 3000);
    assert_int_equal(core_rt[0].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT], 4000);
    assert_int_equal(core_rt[0].rack_pri_res_timestamp_uS[0], 5000);
}