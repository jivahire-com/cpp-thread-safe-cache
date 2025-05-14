//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_data_sampling.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <data_utilities_i.h>
#include <dvfs.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_info;
extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
extern uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES]; // /* used only for MPAM*/
}

/*------------- Typedefs -----------------*/

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

// Unit test for data_util_calc_mma_res
TEST_FUNCTION(test_data_util_calc_mma_res, test_setup, test_teardown)
{

    uint16_t mma_min = 0;
    uint16_t mma_max = 0;
    uint16_t mma_average = 0;
    uint16_t mma_latest_value = 50;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Valid parameters
    data_util_calc_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 50);
    assert_int_equal(mma_max, 50);
    assert_int_equal(mma_average, 50);

    // Test case: Update with new latest value
    mma_latest_value = 100;
    data_util_calc_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 50);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 75); // Weighted average calculation

    // Test case: Update with lower latest value
    mma_latest_value = 30;
    data_util_calc_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52); // Weighted average calculation

    // Test case: Zero latest value
    mma_latest_value = 0;
    data_util_calc_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52); // No change in average

    // Test case: Invalid parameters (time_diff_uS = 0)
    time_diff_uS = 0;
    data_util_calc_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    // No change in values as the function should handle invalid parameters gracefully
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52);
}