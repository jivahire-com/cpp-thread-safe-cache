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

TEST_FUNCTION(test_data_util_convert_systick_to_microseconds, test_setup, test_teardown)
{
    will_return(__wrap_gtimer_prodfw_get_frequency, 0);
    uint64_t micro_seconds = data_util_convert_systick_to_microseconds(0x1FFFFFFFF);
    assert_int_equal(micro_seconds, 0);

    will_return(__wrap_gtimer_prodfw_get_frequency, 0x10000000);
    micro_seconds = data_util_convert_systick_to_microseconds(0x1FFFFFFFF);
    assert_int_equal(micro_seconds, (0x1FFFFFFFF * 1000000) / 0x10000000);
}

TEST_FUNCTION(test_data_util_running_avg_update, test_setup, test_teardown)
{
    running_avg_t running_avg;
    running_avg.average = 1000;
    running_avg.num_samples = 10;
    uint16_t new_value = 1200;

    data_util_running_avg_update(&running_avg, new_value);
    assert_int_equal(running_avg.average, 1018);
    assert_int_equal(running_avg.num_samples, 11);
}

TEST_FUNCTION(test_data_util_running_avg_update_corner, test_setup, test_teardown)
{
    running_avg_t running_avg;
    running_avg.average = 0;
    running_avg.num_samples = 0;
    uint16_t new_value = 1500;

    data_util_running_avg_update(&running_avg, new_value);
    assert_int_equal(running_avg.average, 1500);
    assert_int_equal(running_avg.num_samples, 1);

    data_util_running_avg_update(nullptr, new_value);

    running_avg.average = UINT16_MAX;
    running_avg.num_samples = UINT16_MAX;
    new_value = UINT16_MAX;

    data_util_running_avg_update(&running_avg, new_value);
    assert_int_equal(running_avg.average, UINT16_MAX);
    assert_int_equal(running_avg.num_samples, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_running_avg_reset, test_setup, test_teardown)
{
    running_avg_t running_avg;
    running_avg.average = 1000;
    running_avg.num_samples = 5;

    data_util_running_avg_reset(&running_avg);

    assert_int_equal(running_avg.average, 0);
    assert_int_equal(running_avg.num_samples, 0);

    data_util_running_avg_reset(nullptr);
}

TEST_FUNCTION(test_data_util_mean_of_means, test_setup, test_teardown)
{
    uint16_t mean = data_util_mean_of_means(1000, 10, 2000, 5);
    assert_int_equal(mean, 1333);
}

TEST_FUNCTION(test_data_util_mean_of_means_corner, test_setup, test_teardown)
{
    uint16_t mean = data_util_mean_of_means(1000, 0, 2000, 0);
    assert_int_equal(mean, 0);

    mean = data_util_mean_of_means(UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX);
    assert_int_equal(mean, UINT16_MAX);
}

// Unit test for data_utils_update_residency
TEST_FUNCTION(test_data_utils_update_residency, test_setup, test_teardown)
{
    uint64_t time_stamp_uS = 2000;
    soc_dimm.previous_soc_dimm_timestamp_uS = 1000;
    soc_dimm.residency_uS = 800;
    uint64_t time_diff_uS = 0;

    time_diff_uS =
        data_utils_update_residency(time_stamp_uS, &soc_dimm.previous_soc_dimm_timestamp_uS, &soc_dimm.residency_uS);

    assert_int_equal(soc_dimm.residency_uS, 1800);
    assert_int_equal(soc_dimm.previous_soc_dimm_timestamp_uS, 2000);
    assert_int_equal(time_diff_uS, 1000);

    // Invalid case
    soc_dimm.previous_soc_dimm_timestamp_uS = 0;
    time_diff_uS = 0;
    time_diff_uS =
        data_utils_update_residency(time_stamp_uS, &soc_dimm.previous_soc_dimm_timestamp_uS, &soc_dimm.residency_uS);

    assert_int_equal(soc_dimm.residency_uS, 1800); // will not update
    assert_int_equal(soc_dimm.previous_soc_dimm_timestamp_uS, 2000);
    assert_int_equal(time_diff_uS, 0);
}

TEST_FUNCTION(test_data_util_calc_mma_u16, test_setup, test_teardown)
{
    mma_u16_t mma = {0};

    // Feed a few test values and validate the min and max, as the average is
    // already tested.

    data_util_calc_mma_u16(&mma, 100);
    assert_int_equal(mma.min, 100);
    assert_int_equal(mma.max, 100);

    data_util_calc_mma_u16(&mma, 50);
    assert_int_equal(mma.min, 50);
    assert_int_equal(mma.max, 100);

    data_util_calc_mma_u16(&mma, 150);
    assert_int_equal(mma.min, 50);
    assert_int_equal(mma.max, 150);
}

