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
#include <die_2_die_exchange_i.h>
#include <power_tlm_fuse.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
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
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);

    data_proc_tlm_cmpnt_init(1);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg, test_setup, test_teardown)
{

    die_2_die_exch_init(0);
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    die_2_die_exch_init(1);
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_enable_disable_transition, test_setup, test_teardown)
{
    data_proc_tlm_cmpnt_enable_disable_transition(true);
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