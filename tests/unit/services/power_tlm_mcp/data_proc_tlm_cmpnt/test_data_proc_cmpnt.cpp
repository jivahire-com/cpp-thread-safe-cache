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
    for (uint8_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        data_proc_snsr_fifo_is_empty[i] = true;
    }
    will_return(__wrap_sensor_fifo_svc_is_empty, data_proc_snsr_fifo_is_empty);
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
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
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].cstate_res_timestamp_uS = 2000;
        core_rt[core_id].pstate_res_timestamp_uS = 3000;
    }

    core_rt[3].status_flags.pkt_cstate_is_valid = true;
    core_rt[4].status_flags.pkt_pstate_is_valid = true;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10000);
    data_smpl_reset_residency_timestamps();

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        if (core_id == 3)
        {
            assert_int_equal(core_rt[core_id].cstate_res_timestamp_uS, 10000);
        }
        else
        {
            assert_int_equal(core_rt[core_id].cstate_res_timestamp_uS, 2000);
        }

        if (core_id == 4)
        {
            assert_int_equal(core_rt[core_id].pstate_res_timestamp_uS, 10000);
        }
        else
        {
            assert_int_equal(core_rt[core_id].pstate_res_timestamp_uS, 3000);
        }
    }
}