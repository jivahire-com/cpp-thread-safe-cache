//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_data_utilities.cpp
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
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;
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

TEST_FUNCTION(test_data_util_calc_time_diff_and_update, test_setup, test_teardown)
{
    uint64_t last_timestamp_uS, current_timestamp_uS, difference_uS;

    // Test case: Normal time progression
    last_timestamp_uS = 1000;
    current_timestamp_uS = 2500;
    difference_uS = 0;

    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 1500);
    assert_int_equal(last_timestamp_uS, 2500); // Should be updated to current

    // Test case: Continuous time progression
    current_timestamp_uS = 3000;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 500);
    assert_int_equal(last_timestamp_uS, 3000);

    // Test case: Same timestamp (no time progression)
    current_timestamp_uS = 3000;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 0);
    assert_int_equal(last_timestamp_uS, 3000);

    // Test case: Time going backwards (wraparound or reset)
    current_timestamp_uS = 2000;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 0);        // Should be 0 when time goes backwards
    assert_int_equal(last_timestamp_uS, 2000); // Should still update last timestamp

    // Test case: Maximum values
    last_timestamp_uS = UINT64_MAX - 1000;
    current_timestamp_uS = UINT64_MAX;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 1000);
    assert_int_equal(last_timestamp_uS, UINT64_MAX);

    // Test case: Zero timestamps
    last_timestamp_uS = 0;
    current_timestamp_uS = 0;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 0);
    assert_int_equal(last_timestamp_uS, 0);

    // Test case: Starting from zero
    last_timestamp_uS = 0;
    current_timestamp_uS = 5000;
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, &difference_uS);

    assert_int_equal(difference_uS, 5000);
    assert_int_equal(last_timestamp_uS, 5000);
}

TEST_FUNCTION(test_data_util_calc_time_diff_and_update_null, test_setup, test_teardown)
{
    uint64_t last_timestamp_uS, current_timestamp_uS, difference_uS;

    // Test case: Normal time progression
    last_timestamp_uS = 1000;
    current_timestamp_uS = 2500;
    difference_uS = 0;

    data_util_calc_time_diff_and_update(nullptr, &current_timestamp_uS, &difference_uS);
    data_util_calc_time_diff_and_update(&last_timestamp_uS, nullptr, &difference_uS);
    data_util_calc_time_diff_and_update(&last_timestamp_uS, &current_timestamp_uS, nullptr);
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

TEST_FUNCTION(test_data_util_running_avg_dur_reset, test_setup, test_teardown)
{
    running_avg_dur_t ra;
    ra.sum_weighted = 12345;
    ra.total_time_uS = 67890;

    data_util_running_avg_dur_reset(&ra);

    assert_int_equal(ra.sum_weighted, 0);
    assert_int_equal(ra.total_time_uS, 0);

    data_util_running_avg_dur_reset(nullptr);
}

TEST_FUNCTION(test_data_util_running_avg_dur_update, test_setup, test_teardown)
{
    running_avg_dur_t ra;
    data_util_running_avg_dur_reset(&ra);

    // Test normal case
    data_util_running_avg_dur_update(&ra, 100, 1000);
    assert_int_equal(ra.sum_weighted, 100000);
    assert_int_equal(ra.total_time_uS, 1000);

    // Test accumulation
    data_util_running_avg_dur_update(&ra, 200, 2000);
    assert_int_equal(ra.sum_weighted, 500000);
    assert_int_equal(ra.total_time_uS, 3000);

    // Test zero duration (should not update)
    data_util_running_avg_dur_update(&ra, 300, 0);
    assert_int_equal(ra.sum_weighted, 500000);
    assert_int_equal(ra.total_time_uS, 3000);

    // Test maximum uint16_t value
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, UINT16_MAX, 1000);
    assert_int_equal(ra.sum_weighted, (uint64_t)UINT16_MAX * 1000);
    assert_int_equal(ra.total_time_uS, 1000);

    // Test maximum uint32_t duration
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, 1000, UINT32_MAX);
    assert_int_equal(ra.sum_weighted, (uint64_t)1000 * UINT32_MAX);
    assert_int_equal(ra.total_time_uS, UINT32_MAX);

    // Test maximum values for both parameters
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, UINT16_MAX, UINT32_MAX);
    assert_int_equal(ra.sum_weighted, (uint64_t)UINT16_MAX * UINT32_MAX);
    assert_int_equal(ra.total_time_uS, UINT32_MAX);

    data_util_running_avg_dur_update(nullptr, UINT16_MAX, UINT32_MAX);
}

TEST_FUNCTION(test_data_util_running_avg_dur_get, test_setup, test_teardown)
{
    running_avg_dur_t ra;

    // Test zero time case (should return 0)
    data_util_running_avg_dur_reset(&ra);
    assert_int_equal(data_util_running_avg_dur_get(&ra), 0);

    // Test simple case: 100 value for 1000 microseconds = average of 100
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, 100, 1000);
    assert_int_equal(data_util_running_avg_dur_get(&ra), 100);

    // Test rounding: 100 value for 3 microseconds = 33333/3 = 11111 (rounded)
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, 100, 3);
    uint16_t result = data_util_running_avg_dur_get(&ra);
    // Expected: (100*3 + 3/2) / 3 = (300 + 1) / 3 = 301/3 = 100 (rounded)
    assert_int_equal(result, 100);

    // Test rounding down: 100 value for 7 microseconds
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, 100, 7);
    result = data_util_running_avg_dur_get(&ra);
    // Expected: (100*7 + 7/2) / 7 = (700 + 3) / 7 = 703/7 = 100 (rounded)
    assert_int_equal(result, 100);

    // Test accumulated values: weighted average
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, 200, 1000); // 200 for 1000us
    data_util_running_avg_dur_update(&ra, 100, 2000); // 100 for 2000us
    result = data_util_running_avg_dur_get(&ra);
    // Expected: ((200*1000) + (100*2000) + 3000/2) / 3000 = (200000 + 200000 + 1500) / 3000 = 401500/3000 = 133 (rounded)
    assert_int_equal(result, 133);

    // Test maximum value input
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, UINT16_MAX, 1);
    result = data_util_running_avg_dur_get(&ra);
    assert_int_equal(result, UINT16_MAX);

    // Test large values that should clamp to UINT16_MAX
    data_util_running_avg_dur_reset(&ra);
    data_util_running_avg_dur_update(&ra, UINT16_MAX, UINT16_MAX);
    result = data_util_running_avg_dur_get(&ra);
    // This should result in a very large value that gets truncated to uint16_t
    // (UINT16_MAX * UINT16_MAX + UINT16_MAX/2) / UINT16_MAX = UINT16_MAX + 0.5 = UINT16_MAX (truncated)
    assert_int_equal(result, UINT16_MAX);

    result = data_util_running_avg_dur_get(nullptr);
}

TEST_FUNCTION(test_data_util_mean_of_means, test_setup, test_teardown)
{
    uint16_t mean = data_util_mean_of_means(1000, 10, 2000, 5);
    assert_int_equal(mean, 1333);

    mean = data_util_mean_of_means(0, 0, 0, 0);
    assert_int_equal(mean, 0);
}

TEST_FUNCTION(test_data_util_mean_of_summations, test_setup, test_teardown)
{
    uint16_t mean = data_util_mean_of_summations(1000, 10, 2000, 5);
    assert_int_equal(mean, 200);

    mean = data_util_mean_of_summations(0, 0, 0, 0);
    assert_int_equal(mean, 0);

    mean = data_util_mean_of_summations(UINT32_MAX, UINT16_MAX, UINT32_MAX, UINT16_MAX);
    assert_int_equal(mean, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_mean_of_means_corner, test_setup, test_teardown)
{
    uint16_t mean = data_util_mean_of_means(1000, 0, 2000, 0);
    assert_int_equal(mean, 0);

    mean = data_util_mean_of_means(UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX);
    assert_int_equal(mean, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_max_avg_of_summations, test_setup, test_teardown)
{
    uint16_t max = data_util_max_avg_of_summations(1000, 10, 2000, 5);
    assert_int_equal(max, 400);

    max = data_util_max_avg_of_summations(0, 0, 0, 0);
    assert_int_equal(max, 0);

    max = data_util_max_avg_of_summations(2000, 4, 0, 0);
    assert_int_equal(max, 500);

    max = data_util_max_avg_of_summations(UINT32_MAX, UINT16_MAX, UINT32_MAX, UINT16_MAX);
    assert_int_equal(max, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_calc_mma_u16, test_setup, test_teardown)
{
    mma_u16_t mma = {{0}};

    // Test initial state
    assert_int_equal(mma.min, 0);
    assert_int_equal(mma.max, 0);
    assert_int_equal(mma.running_avg.summation, 0);
    assert_int_equal(mma.running_avg.num_samples, 0);

    // Test first value - should set both min and max
    data_util_calc_mma_u16(&mma, 100);
    assert_int_equal(mma.min, 100);
    assert_int_equal(mma.max, 100);
    assert_int_equal(data_util_running_avg_u16_get(&mma.running_avg), 100);
    assert_int_equal(mma.running_avg.summation, 100);
    assert_int_equal(mma.running_avg.num_samples, 1);

    // Test lower value (new minimum)
    data_util_calc_mma_u16(&mma, 50);
    assert_int_equal(mma.min, 50);
    assert_int_equal(mma.max, 100);
    assert_int_equal(data_util_running_avg_u16_get(&mma.running_avg), 75); // (100 + 50) / 2 = 75
    assert_int_equal(mma.running_avg.summation, 150);
    assert_int_equal(mma.running_avg.num_samples, 2);

    // Test higher value (new maximum)
    data_util_calc_mma_u16(&mma, 150);
    assert_int_equal(mma.min, 50);
    assert_int_equal(mma.max, 150);
    // Expected running average: (150 + 150) / 3 = 100
    assert_int_equal(data_util_running_avg_u16_get(&mma.running_avg), 100);
    assert_int_equal(mma.running_avg.summation, 300);
    assert_int_equal(mma.running_avg.num_samples, 3);

    // Test zero value - should become new minimum only if actually smaller
    data_util_calc_mma_u16(&mma, 0);
    assert_int_equal(mma.min, 0);   // Should update to 0 since 0 < 50
    assert_int_equal(mma.max, 150); // Should remain unchanged
    // Expected running average: (300 + 0) / 4 = 75
    assert_int_equal(data_util_running_avg_u16_get(&mma.running_avg), 75);
    assert_int_equal(mma.running_avg.summation, 300);
    assert_int_equal(mma.running_avg.num_samples, 4);

    // Test another zero value - should NOT update min since min is already 0
    data_util_calc_mma_u16(&mma, 0);
    assert_int_equal(mma.min, 0);   // Should remain 0
    assert_int_equal(mma.max, 150); // Should remain unchanged
    // Expected running average: (300 + 0) / 5 = 60
    assert_int_equal(data_util_running_avg_u16_get(&mma.running_avg), 60);
    assert_int_equal(mma.running_avg.summation, 300);
    assert_int_equal(mma.running_avg.num_samples, 5);

    // Test value larger than zero but smaller than current min - should update min
    data_util_calc_mma_u16(&mma, 5);
    assert_int_equal(mma.min, 0);   // Should remain 0 since 5 > 0
    assert_int_equal(mma.max, 150); // Should remain unchanged
    assert_int_equal(mma.running_avg.summation, 305);
    assert_int_equal(mma.running_avg.num_samples, 6);

    // Test scenario where zero is the first value
    mma_u16_t mma_zero = {{0}};
    data_util_calc_mma_u16(&mma_zero, 0);
    assert_int_equal(mma_zero.min, 0);
    assert_int_equal(mma_zero.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(&mma_zero.running_avg), 0);
    assert_int_equal(mma_zero.running_avg.summation, 0);
    assert_int_equal(mma_zero.running_avg.num_samples, 1);

    // Test that subsequent non-zero values don't incorrectly update min
    data_util_calc_mma_u16(&mma_zero, 100);
    assert_int_equal(mma_zero.min, 0);   // Should remain 0 since 100 > 0
    assert_int_equal(mma_zero.max, 100); // Should update to 100
    assert_int_equal(data_util_running_avg_u16_get(&mma_zero.running_avg), 50); // (0 + 100) / 2 = 50
    assert_int_equal(mma_zero.running_avg.summation, 100);
    assert_int_equal(mma_zero.running_avg.num_samples, 2);

    // Test negative scenario - add another zero, should not change min
    data_util_calc_mma_u16(&mma_zero, 0);
    assert_int_equal(mma_zero.min, 0);   // Should remain 0
    assert_int_equal(mma_zero.max, 100); // Should remain 100
    // Expected: (100 + 0) / 3 = 33
    assert_int_equal(data_util_running_avg_u16_get(&mma_zero.running_avg), 33);
    assert_int_equal(mma_zero.running_avg.summation, 100);
    assert_int_equal(mma_zero.running_avg.num_samples, 3);

    // Test the original bug scenario: samples in sequence that exposed the bug
    mma_u16_t mma_bug = {{0}};
    data_util_calc_mma_u16(&mma_bug, 874); // First sample
    assert_int_equal(mma_bug.min, 874);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u16(&mma_bug, 26); // Second sample - new min
    assert_int_equal(mma_bug.min, 26);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u16(&mma_bug, 0); // Third sample - new min (zero)
    assert_int_equal(mma_bug.min, 0);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u16(&mma_bug, 50); // Fourth sample - should NOT change min
    assert_int_equal(mma_bug.min, 0);     // Should stay 0, not become 50
    assert_int_equal(mma_bug.max, 874);

    // Test NULL pointer (should not crash)
    data_util_calc_mma_u16(nullptr, 150);

    // Test maximum uint16_t values
    mma_u16_t mma_max = {{0}};
    data_util_calc_mma_u16(&mma_max, UINT16_MAX);
    assert_int_equal(mma_max.min, UINT16_MAX);
    assert_int_equal(mma_max.max, UINT16_MAX);
    assert_int_equal(data_util_running_avg_u16_get(&mma_max.running_avg), UINT16_MAX);
    assert_int_equal(mma_max.running_avg.summation, UINT16_MAX);
    assert_int_equal(mma_max.running_avg.num_samples, 1);

    data_util_calc_mma_u16(&mma_max, 0);
    assert_int_equal(mma_max.min, 0);
    assert_int_equal(mma_max.max, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_calc_mma_u32, test_setup, test_teardown)
{
    mma_u32_t mma = {{0}};

    // Test initial state
    assert_int_equal(mma.min, 0);
    assert_int_equal(mma.max, 0);
    assert_int_equal(mma.running_avg.summation, 0);
    assert_int_equal(mma.running_avg.num_samples, 0);

    // Test first value
    data_util_calc_mma_u32(&mma, 1000);
    assert_int_equal(mma.min, 1000);
    assert_int_equal(mma.max, 1000);
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 1000);
    assert_int_equal(mma.running_avg.summation, 1000);
    assert_int_equal(mma.running_avg.num_samples, 1);

    // Test lower value (new minimum)
    data_util_calc_mma_u32(&mma, 500);
    assert_int_equal(mma.min, 500);
    assert_int_equal(mma.max, 1000);
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 750); // (1000 + 500) / 2 = 750
    assert_int_equal(mma.running_avg.summation, 1500);
    assert_int_equal(mma.running_avg.num_samples, 2);

    // Test higher value (new maximum)
    data_util_calc_mma_u32(&mma, 2000);
    assert_int_equal(mma.min, 500);
    assert_int_equal(mma.max, 2000);
    // Expected running average: (1500 + 2000) / 3 = 1167 (rounded)
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 1167);
    assert_int_equal(mma.running_avg.summation, 3500);
    assert_int_equal(mma.running_avg.num_samples, 3);

    // Test value between min and max
    data_util_calc_mma_u32(&mma, 1200);
    assert_int_equal(mma.min, 500);  // Should remain unchanged
    assert_int_equal(mma.max, 2000); // Should remain unchanged
    // Expected running average: (3500 + 1200) / 4 = 1175
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 1175);
    assert_int_equal(mma.running_avg.summation, 4700);
    assert_int_equal(mma.running_avg.num_samples, 4);

    // Test zero value - should become new minimum
    data_util_calc_mma_u32(&mma, 0);
    assert_int_equal(mma.min, 0);    // Should update to 0
    assert_int_equal(mma.max, 2000); // Should remain unchanged
    // Expected running average: (4700 + 0) / 5 = 940
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 940);
    assert_int_equal(mma.running_avg.summation, 4700);
    assert_int_equal(mma.running_avg.num_samples, 5);

    // Test another zero value - should NOT update min since min is already 0
    data_util_calc_mma_u32(&mma, 0);
    assert_int_equal(mma.min, 0);    // Should remain 0
    assert_int_equal(mma.max, 2000); // Should remain unchanged
    // Expected running average: (4700 + 0) / 6 = 783
    assert_int_equal(data_util_running_avg_u32_get(&mma.running_avg), 783);
    assert_int_equal(mma.running_avg.summation, 4700);
    assert_int_equal(mma.running_avg.num_samples, 6);

    // Test scenario where zero is the first value
    mma_u32_t mma_zero = {{0}};
    data_util_calc_mma_u32(&mma_zero, 0);
    assert_int_equal(mma_zero.min, 0);
    assert_int_equal(mma_zero.max, 0);
    assert_int_equal(data_util_running_avg_u32_get(&mma_zero.running_avg), 0);
    assert_int_equal(mma_zero.running_avg.summation, 0);
    assert_int_equal(mma_zero.running_avg.num_samples, 1);

    // Test that subsequent non-zero values don't incorrectly update min
    data_util_calc_mma_u32(&mma_zero, 100);
    assert_int_equal(mma_zero.min, 0);   // Should remain 0 since 100 > 0
    assert_int_equal(mma_zero.max, 100); // Should update to 100
    assert_int_equal(data_util_running_avg_u32_get(&mma_zero.running_avg), 50); // (0 + 100) / 2 = 50
    assert_int_equal(mma_zero.running_avg.summation, 100);
    assert_int_equal(mma_zero.running_avg.num_samples, 2);

    // Test the original bug scenario: samples that would expose the old bug
    mma_u32_t mma_bug = {{0}};
    data_util_calc_mma_u32(&mma_bug, 874); // First sample
    assert_int_equal(mma_bug.min, 874);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u32(&mma_bug, 26); // Second sample - new min
    assert_int_equal(mma_bug.min, 26);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u32(&mma_bug, 0); // Third sample - new min (zero)
    assert_int_equal(mma_bug.min, 0);
    assert_int_equal(mma_bug.max, 874);

    data_util_calc_mma_u32(&mma_bug, 50); // Fourth sample - should NOT change min
    assert_int_equal(mma_bug.min, 0);     // Should stay 0, not become 50
    assert_int_equal(mma_bug.max, 874);

    // Test maximum uint32_t value
    mma_u32_t mma_max = {{0}};
    data_util_calc_mma_u32(&mma_max, UINT32_MAX);
    assert_int_equal(mma_max.min, UINT32_MAX);
    assert_int_equal(mma_max.max, UINT32_MAX);
    assert_int_equal(data_util_running_avg_u32_get(&mma_max.running_avg), UINT32_MAX);
    assert_int_equal(mma_max.running_avg.summation, UINT32_MAX);
    assert_int_equal(mma_max.running_avg.num_samples, 1);

    // Test second value with max - should reduce average
    data_util_calc_mma_u32(&mma_max, 0);
    assert_int_equal(mma_max.min, 0);
    assert_int_equal(mma_max.max, UINT32_MAX);
    // Expected: (UINT32_MAX + 0 + 1) / 2 using 64-bit arithmetic
    uint32_t expected_avg = (uint32_t)(((uint64_t)UINT32_MAX + 1) / 2);
    assert_int_equal(data_util_running_avg_u32_get(&mma_max.running_avg), expected_avg);
    assert_int_equal(mma_max.running_avg.summation, UINT32_MAX);
    assert_int_equal(mma_max.running_avg.num_samples, 2);

    // Test large values to ensure no overflow
    mma_u32_t mma_large = {{0}};
    data_util_calc_mma_u32(&mma_large, 0xFFFF0000);
    data_util_calc_mma_u32(&mma_large, 0xFFFF0001);
    assert_int_equal(mma_large.min, 0xFFFF0000);
    assert_int_equal(mma_large.max, 0xFFFF0001);
    // Average should be calculated correctly without overflow
    uint32_t expected_large_avg = (uint32_t)(((uint64_t)0xFFFF0000 + 0xFFFF0001 + 1) / 2);
    assert_int_equal(data_util_running_avg_u32_get(&mma_large.running_avg), expected_large_avg);
    assert_int_equal(mma_large.running_avg.summation, (uint64_t)0xFFFF0000 + 0xFFFF0001);
    assert_int_equal(mma_large.running_avg.num_samples, 2);

    // Test saturation behavior - add many samples to reach UINT32_MAX sample count
    mma_u32_t mma_sat = {{0}};
    mma_sat.running_avg.num_samples = UINT32_MAX - 1;
    mma_sat.running_avg.summation = 1000ULL * (UINT32_MAX - 1);
    mma_sat.min = 500;
    mma_sat.max = 1500;

    data_util_calc_mma_u32(&mma_sat, 1200);
    assert_int_equal(mma_sat.min, 500);
    assert_int_equal(mma_sat.max, 1500);
    assert_int_equal(mma_sat.running_avg.num_samples, UINT32_MAX); // Should be saturated
    // Average should still be calculated correctly
    uint64_t total = 1000ULL * (UINT32_MAX - 1) + 1200;
    uint64_t updated_avg = (total + UINT32_MAX / 2) / UINT32_MAX;
    assert_int_equal(data_util_running_avg_u32_get(&mma_sat.running_avg), (uint32_t)updated_avg);

    // Test NULL pointer (should not crash and log error)
    data_util_calc_mma_u32(nullptr, 100);

    // Test sequence with duplicate values
    mma_u32_t mma_dup = {{0}};
    data_util_calc_mma_u32(&mma_dup, 1000);
    data_util_calc_mma_u32(&mma_dup, 1000);
    data_util_calc_mma_u32(&mma_dup, 1000);
    assert_int_equal(mma_dup.min, 1000);
    assert_int_equal(mma_dup.max, 1000);
    assert_int_equal(data_util_running_avg_u32_get(&mma_dup.running_avg), 1000);
    assert_int_equal(mma_dup.running_avg.summation, 3000);
    assert_int_equal(mma_dup.running_avg.num_samples, 3);
}

TEST_FUNCTION(test_data_util_calc_mma_dur_u16, test_setup, test_teardown)
{
    mma_u16_dur_t mma_dur = {{0}};

    // Test initial state
    assert_int_equal(mma_dur.min, 0);
    assert_int_equal(mma_dur.max, 0);
    assert_int_equal(mma_dur.running_avg_dur.sum_weighted, 0);
    assert_int_equal(mma_dur.running_avg_dur.total_time_uS, 0);

    // Test first value
    data_util_calc_mma_dur_u16(&mma_dur, 100, 1000);
    assert_int_equal(mma_dur.min, 100);
    assert_int_equal(mma_dur.max, 100);
    assert_int_equal(mma_dur.running_avg_dur.sum_weighted, 100000);
    assert_int_equal(mma_dur.running_avg_dur.total_time_uS, 1000);

    // Test lower value (new minimum)
    data_util_calc_mma_dur_u16(&mma_dur, 50, 2000);
    assert_int_equal(mma_dur.min, 50);
    assert_int_equal(mma_dur.max, 100);
    assert_int_equal(mma_dur.running_avg_dur.sum_weighted, 200000);
    assert_int_equal(mma_dur.running_avg_dur.total_time_uS, 3000);

    // Test higher value (new maximum)
    data_util_calc_mma_dur_u16(&mma_dur, 150, 1500);
    assert_int_equal(mma_dur.min, 50);
    assert_int_equal(mma_dur.max, 150);
    assert_int_equal(mma_dur.running_avg_dur.sum_weighted, 425000);
    assert_int_equal(mma_dur.running_avg_dur.total_time_uS, 4500);

    // Test zero duration (should not update running average)
    uint64_t prev_sum_weighted = mma_dur.running_avg_dur.sum_weighted;
    uint32_t prev_total_time = mma_dur.running_avg_dur.total_time_uS;
    data_util_calc_mma_dur_u16(&mma_dur, 200, 0);
    assert_int_equal(mma_dur.min, 50);
    assert_int_equal(mma_dur.max, 200);                                        // Min/max should still update
    assert_int_equal(mma_dur.running_avg_dur.sum_weighted, prev_sum_weighted); // But duration average should not
    assert_int_equal(mma_dur.running_avg_dur.total_time_uS, prev_total_time);

    // Test maximum values
    mma_u16_dur_t mma_dur_max = {{0}};
    data_util_calc_mma_dur_u16(&mma_dur_max, UINT16_MAX, UINT32_MAX);
    assert_int_equal(mma_dur_max.min, UINT16_MAX);
    assert_int_equal(mma_dur_max.max, UINT16_MAX);
    assert_int_equal(mma_dur_max.running_avg_dur.sum_weighted, (uint64_t)UINT16_MAX * UINT32_MAX);
    assert_int_equal(mma_dur_max.running_avg_dur.total_time_uS, UINT32_MAX);

    // Test null pointer (should not crash)
    data_util_calc_mma_dur_u16(nullptr, 100, 1000);

    // Test min update when min is initially 0
    mma_u16_dur_t mma_dur_zero = {{0}};
    data_util_calc_mma_dur_u16(&mma_dur_zero, 0, 1000);
    assert_int_equal(mma_dur_zero.min, 0);
    assert_int_equal(mma_dur_zero.max, 0);

    // Test that non-zero value updates min from 0
    data_util_calc_mma_dur_u16(&mma_dur_zero, 25, 1000);
    assert_int_equal(mma_dur_zero.min, 25);
    assert_int_equal(mma_dur_zero.max, 25);
}

TEST_FUNCTION(test_data_util_mov_avg_u16_init, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    data_util_mov_avg_u16_init(&ma, samples, 5);
    assert_int_equal(ma.sample_capacity, 5);
    assert_int_equal(ma.sample_index, 0);
    assert_int_equal(ma.sample_count, 0);
    assert_int_equal(ma.total_sum, 0);
    assert_ptr_equal(ma.samples, samples);

    // Test with invalid parameters
    data_util_mov_avg_u16_init(nullptr, samples, 5);
    data_util_mov_avg_u16_init(&ma, nullptr, 5);
    data_util_mov_avg_u16_init(&ma, samples, 0);
}

TEST_FUNCTION(test_data_util_mov_avg_u16_add_sample, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u16_init(&ma, samples, 5);

    // Add a sample and check the state
    data_util_mov_avg_u16_add_sample(&ma, 100);
    assert_int_equal(ma.sample_count, 1);
    assert_int_equal(ma.total_sum, 100);
    assert_int_equal(ma.samples[0], 100);
    assert_int_equal(data_util_mov_avg_u16_get(&ma), 100);

    // Add another sample
    data_util_mov_avg_u16_add_sample(&ma, 200);
    assert_int_equal(ma.sample_count, 2);
    assert_int_equal(ma.total_sum, 300);
    assert_int_equal(ma.samples[1], 200);
    assert_int_equal(data_util_mov_avg_u16_get(&ma), 150); // Average of 100 and 200

    // Add more samples to fill the buffer
    data_util_mov_avg_u16_add_sample(&ma, 300);
    data_util_mov_avg_u16_add_sample(&ma, 400);
    data_util_mov_avg_u16_add_sample(&ma, 500);
    assert_int_equal(ma.sample_count, 5); // Should not exceed capacity
    assert_int_equal(ma.total_sum, 1500);
    assert_int_equal(ma.samples[2], 300);
    assert_int_equal(data_util_mov_avg_u16_get(&ma), 300); // Average of 100, 200, 300, 400, 500

    // Check circular behavior
    data_util_mov_avg_u16_add_sample(&ma, 600);
    assert_int_equal(ma.sample_index, 1);                  // Should overwrite the first sample
    assert_int_equal(ma.total_sum, 2000);                  // New total sum after overwrite
    assert_int_equal(data_util_mov_avg_u16_get(&ma), 400); // Average of 200, 300, 400, 500, 600
}

TEST_FUNCTION(test_data_util_mov_avg_u16_add_sample_corner, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u16_init(&ma, samples, 5);

    // Add sample with NULL pointer
    data_util_mov_avg_u16_add_sample(nullptr, 100);

    // Add sample with zero value
    data_util_mov_avg_u16_add_sample(&ma, 0);
    assert_int_equal(ma.sample_count, 1);
    assert_int_equal(ma.total_sum, 0);
    assert_int_equal(ma.samples[0], 0);

    ma.total_sum = UINT32_MAX - 10;
    data_util_mov_avg_u16_add_sample(&ma, 20);
    assert_int_equal(ma.total_sum, UINT32_MAX);
}

TEST_FUNCTION(test_data_util_mov_avg_u16_get_fail, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u16_init(&ma, samples, 5);

    // Test with NULL pointer
    assert_int_equal(data_util_mov_avg_u16_get(nullptr), 0);

    // Test with no samples added
    assert_int_equal(data_util_mov_avg_u16_get(&ma), 0);
}

// add unit tests for data_util_mov_avg_u16_clear
TEST_FUNCTION(test_data_util_mov_avg_u16_clear, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u16_init(&ma, samples, 5);

    // Add some samples
    data_util_mov_avg_u16_add_sample(&ma, 100);
    data_util_mov_avg_u16_add_sample(&ma, 200);
    assert_int_equal(ma.sample_count, 2);
    assert_int_equal(ma.total_sum, 300);

    // Clear the moving average
    data_util_mov_avg_u16_clear(&ma);
    assert_int_equal(ma.sample_count, 0);
    assert_int_equal(ma.total_sum, 0);
    assert_int_equal(ma.sample_index, 0);
}

// add unit tests for data_util_mov_avg_u16_clear corner cases
TEST_FUNCTION(test_data_util_mov_avg_u16_clear_corner, test_setup, test_teardown)
{
    moving_avg_u16_t ma;
    uint16_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u16_init(&ma, samples, 5);

    // Clear with NULL pointer
    data_util_mov_avg_u16_clear(nullptr);
}

TEST_FUNCTION(test_data_util_mov_avg_u32_init, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

    data_util_mov_avg_u32_init(&ma, samples, 5);
    assert_int_equal(ma.sample_capacity, 5);
    assert_int_equal(ma.sample_index, 0);
    assert_int_equal(ma.sample_count, 0);
    assert_int_equal(ma.total_sum, 0);
    assert_ptr_equal(ma.samples, samples);

    // Test with invalid parameters
    data_util_mov_avg_u32_init(nullptr, samples, 5);
    data_util_mov_avg_u32_init(&ma, nullptr, 5);
    data_util_mov_avg_u32_init(&ma, samples, 0);
}

TEST_FUNCTION(test_data_util_mov_avg_u32_add_sample, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u32_init(&ma, samples, 5);

    // Add a sample and check the state
    data_util_mov_avg_u32_add_sample(&ma, 100);
    assert_int_equal(ma.sample_count, 1);
    assert_int_equal(ma.total_sum, 100);
    assert_int_equal(ma.samples[0], 100);
    assert_int_equal(data_util_mov_avg_u32_get(&ma), 100);

    // Add another sample
    data_util_mov_avg_u32_add_sample(&ma, 200);
    assert_int_equal(ma.sample_count, 2);
    assert_int_equal(ma.total_sum, 300);
    assert_int_equal(ma.samples[1], 200);
    assert_int_equal(data_util_mov_avg_u32_get(&ma), 150); // Average of 100 and 200

    // Add more samples to fill the buffer
    data_util_mov_avg_u32_add_sample(&ma, 300);
    data_util_mov_avg_u32_add_sample(&ma, 400);
    data_util_mov_avg_u32_add_sample(&ma, 500);
    assert_int_equal(ma.sample_count, 5); // Should not exceed capacity
    assert_int_equal(ma.total_sum, 1500);
    assert_int_equal(ma.samples[2], 300);
    assert_int_equal(data_util_mov_avg_u32_get(&ma), 300); // Average of 100, 200, 300, 400, 500

    // Check circular behavior
    data_util_mov_avg_u32_add_sample(&ma, 600);
    assert_int_equal(ma.sample_index, 1);                  // Should overwrite the first sample
    assert_int_equal(ma.total_sum, 2000);                  // New total sum after overwrite
    assert_int_equal(data_util_mov_avg_u32_get(&ma), 400); // Average of 200, 300, 400, 500, 600
}

TEST_FUNCTION(test_data_util_mov_avg_u32_add_sample_corner, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u32_init(&ma, samples, 5);

    // Add sample with NULL pointer
    data_util_mov_avg_u32_add_sample(nullptr, 100);

    // Add sample with zero value
    data_util_mov_avg_u32_add_sample(&ma, 0);
    assert_int_equal(ma.sample_count, 1);
    assert_int_equal(ma.total_sum, 0);
    assert_int_equal(ma.samples[0], 0);

    ma.total_sum = UINT32_MAX - 10;
    data_util_mov_avg_u32_add_sample(&ma, 20);
    assert_int_equal(ma.total_sum, UINT32_MAX);

    ma.total_sum = UINT32_MAX;
    data_util_mov_avg_u32_add_sample(&ma, 20);
    assert_int_equal(ma.total_sum, UINT32_MAX);
}

TEST_FUNCTION(test_data_util_mov_avg_u32_get_fail, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u32_init(&ma, samples, 5);

    // Test with NULL pointer
    assert_int_equal(data_util_mov_avg_u32_get(nullptr), 0);

    // Test with no samples added
    assert_int_equal(data_util_mov_avg_u32_get(&ma), 0);
}

// add unit tests for data_util_mov_avg_u32_clear
TEST_FUNCTION(test_data_util_mov_avg_u32_clear, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u32_init(&ma, samples, 5);

    // Add some samples
    data_util_mov_avg_u32_add_sample(&ma, 100);
    data_util_mov_avg_u32_add_sample(&ma, 200);
    assert_int_equal(ma.sample_count, 2);
    assert_int_equal(ma.total_sum, 300);

    // Clear the moving average
    data_util_mov_avg_u32_clear(&ma);
    assert_int_equal(ma.sample_count, 0);
    assert_int_equal(ma.total_sum, 0);
    assert_int_equal(ma.sample_index, 0);
}

// add unit tests for data_util_mov_avg_u32_clear corner cases
TEST_FUNCTION(test_data_util_mov_avg_u32_clear_corner, test_setup, test_teardown)
{
    moving_avg_u32_t ma;
    uint32_t samples[5] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    data_util_mov_avg_u32_init(&ma, samples, 5);

    // Clear with NULL pointer
    data_util_mov_avg_u32_clear(nullptr);
}

TEST_FUNCTION(test_data_util_running_avg_u32_add_sample, test_setup, test_teardown)
{
    running_u32_avg_t avg = {0};

    // Test case: NULL pointer (should not crash and log error)
    data_util_running_avg_u32_add_sample(nullptr, 100);

    // Test case: First sample - should set summation to sample value
    data_util_running_avg_u32_add_sample(&avg, 1000);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 1000);
    assert_int_equal(avg.summation, 1000);
    assert_int_equal(avg.num_samples, 1);

    // Test case: Second sample - should add to summation
    data_util_running_avg_u32_add_sample(&avg, 2000);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 1500); // (1000 + 2000) / 2 = 1500
    assert_int_equal(avg.summation, 3000);
    assert_int_equal(avg.num_samples, 2);

    // Test case: Third sample - verify summation continues correctly
    data_util_running_avg_u32_add_sample(&avg, 3000);
    // Expected: (1000 + 2000 + 3000) / 3 = 6000/3 = 2000
    assert_int_equal(data_util_running_avg_u32_get(&avg), 2000);
    assert_int_equal(avg.summation, 6000);
    assert_int_equal(avg.num_samples, 3);

    // Test case: Add zero value
    data_util_running_avg_u32_add_sample(&avg, 0);
    // Expected: (6000 + 0) / 4 = 1500
    assert_int_equal(data_util_running_avg_u32_get(&avg), 1500);
    assert_int_equal(avg.summation, 6000);
    assert_int_equal(avg.num_samples, 4);

    // Test case: Add maximum uint32_t value
    running_u32_avg_t avg_max = {0};
    data_util_running_avg_u32_add_sample(&avg_max, UINT32_MAX);
    assert_int_equal(data_util_running_avg_u32_get(&avg_max), UINT32_MAX);
    assert_int_equal(avg_max.summation, UINT32_MAX);
    assert_int_equal(avg_max.num_samples, 1);

    // Test case: Add second value to max - should reduce average
    data_util_running_avg_u32_add_sample(&avg_max, 0);
    // Expected: (UINT32_MAX + 0) / 2 = UINT32_MAX / 2 = 2147483647.5, rounded to 2147483648
    uint32_t expected_avg = (uint32_t)(((uint64_t)UINT32_MAX + 1) / 2);
    assert_int_equal(data_util_running_avg_u32_get(&avg_max), expected_avg);
    assert_int_equal(avg_max.summation, UINT32_MAX);
    assert_int_equal(avg_max.num_samples, 2);

    // Test case: Sample count saturation at UINT32_MAX
    running_u32_avg_t avg_sat = {0};
    avg_sat.summation = 5000ULL * UINT32_MAX;
    avg_sat.num_samples = UINT32_MAX;

    data_util_running_avg_u32_add_sample(&avg_sat, 6000);
    // num_samples should remain at UINT32_MAX (saturated)
    assert_int_equal(avg_sat.num_samples, UINT32_MAX);
    // summation should remain unchanged since sample was rejected
    assert_int_equal(avg_sat.summation, 5000ULL * UINT32_MAX);

    // Test case: Many small samples to verify precision
    running_u32_avg_t avg_small = {0};
    for (int i = 1; i <= 100; i++)
    {
        data_util_running_avg_u32_add_sample(&avg_small, i);
    }
    // Sum of 1 to 100 = 5050, average = 5050/100 = 50.5, rounded to 51
    assert_int_equal(data_util_running_avg_u32_get(&avg_small), 51);
    assert_int_equal(avg_small.summation, 5050);
    assert_int_equal(avg_small.num_samples, 100);

    // Test case: Alternating high/low values
    running_u32_avg_t avg_alt = {0};
    data_util_running_avg_u32_add_sample(&avg_alt, 1000000);
    data_util_running_avg_u32_add_sample(&avg_alt, 0);
    data_util_running_avg_u32_add_sample(&avg_alt, 1000000);
    data_util_running_avg_u32_add_sample(&avg_alt, 0);
    // Should sum to 2000000, average to 2000000/4 = 500000
    assert_int_equal(data_util_running_avg_u32_get(&avg_alt), 500000);
    assert_int_equal(avg_alt.summation, 2000000);
    assert_int_equal(avg_alt.num_samples, 4);

    // Test case: Verify rounding behavior
    running_u32_avg_t avg_round = {0};
    data_util_running_avg_u32_add_sample(&avg_round, 1);
    data_util_running_avg_u32_add_sample(&avg_round, 2);
    // Expected: (1 + 2) / 2 = 1.5, rounded to 2
    assert_int_equal(data_util_running_avg_u32_get(&avg_round), 2);
    assert_int_equal(avg_round.summation, 3);

    data_util_running_avg_u32_add_sample(&avg_round, 2);
    // Expected: (1 + 2 + 2) / 3 = 5/3 = 1.67, rounded to 2
    assert_int_equal(data_util_running_avg_u32_get(&avg_round), 2);
    assert_int_equal(avg_round.summation, 5);

    // Test case: Reset and verify clean state
    data_util_running_avg_u32_reset(&avg);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 0);
    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Verify functionality after reset
    data_util_running_avg_u32_add_sample(&avg, 42);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 42);
    assert_int_equal(avg.summation, 42);
    assert_int_equal(avg.num_samples, 1);

    // Test case: Summation overflow protection
    running_u32_avg_t avg_overflow = {0};
    avg_overflow.summation = UINT64_MAX - 1000000; // Close to overflow
    avg_overflow.num_samples = 1;

    // This should succeed (no overflow)
    data_util_running_avg_u32_add_sample(&avg_overflow, 500000);
    assert_int_equal(avg_overflow.summation, UINT64_MAX - 500000);
    assert_int_equal(avg_overflow.num_samples, 2);

    // This should be rejected (would overflow)
    data_util_running_avg_u32_add_sample(&avg_overflow, 1000000);
    // summation and num_samples should remain unchanged
    assert_int_equal(avg_overflow.summation, UINT64_MAX - 500000);
    assert_int_equal(avg_overflow.num_samples, 2);
}

TEST_FUNCTION(test_data_util_running_avg_u16_add_sample, test_setup, test_teardown)
{
    running_u16_avg_t avg = {0};

    // Test case: NULL pointer (should not crash and log error)
    data_util_running_avg_u16_add_sample(nullptr, 100);

    // Test case: First sample - should set summation to sample value
    data_util_running_avg_u16_add_sample(&avg, 1000);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 1000);
    assert_int_equal(avg.summation, 1000);
    assert_int_equal(avg.num_samples, 1);

    // Test case: Second sample - should add to summation
    data_util_running_avg_u16_add_sample(&avg, 2000);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 1500); // (1000 + 2000) / 2 = 1500
    assert_int_equal(avg.summation, 3000);
    assert_int_equal(avg.num_samples, 2);

    // Test case: Third sample - verify summation continues correctly
    data_util_running_avg_u16_add_sample(&avg, 3000);
    // Expected: (1000 + 2000 + 3000) / 3 = 6000/3 = 2000
    assert_int_equal(data_util_running_avg_u16_get(&avg), 2000);
    assert_int_equal(avg.summation, 6000);
    assert_int_equal(avg.num_samples, 3);

    // Test case: Add zero value
    data_util_running_avg_u16_add_sample(&avg, 0);
    // Expected: (6000 + 0) / 4 = 1500
    assert_int_equal(data_util_running_avg_u16_get(&avg), 1500);
    assert_int_equal(avg.summation, 6000);
    assert_int_equal(avg.num_samples, 4);

    // Test case: Add maximum uint16_t value
    running_u16_avg_t avg_max = {0};
    data_util_running_avg_u16_add_sample(&avg_max, UINT16_MAX);
    assert_int_equal(data_util_running_avg_u16_get(&avg_max), UINT16_MAX);
    assert_int_equal(avg_max.summation, UINT16_MAX);
    assert_int_equal(avg_max.num_samples, 1);

    // Test case: Add second value to max - should reduce average
    data_util_running_avg_u16_add_sample(&avg_max, 0);
    // Expected: (UINT16_MAX + 0) / 2 = UINT16_MAX / 2 = 32767.5, rounded to 32768
    uint16_t expected_avg = (uint16_t)(((uint32_t)UINT16_MAX + 1) / 2);
    assert_int_equal(data_util_running_avg_u16_get(&avg_max), expected_avg);
    assert_int_equal(avg_max.summation, UINT16_MAX);
    assert_int_equal(avg_max.num_samples, 2);

    // Test case: Sample count saturation at UINT16_MAX
    running_u16_avg_t avg_sat = {0};
    avg_sat.summation = 5000 * UINT16_MAX;
    avg_sat.num_samples = UINT16_MAX;

    data_util_running_avg_u16_add_sample(&avg_sat, 6000);
    // num_samples should remain at UINT16_MAX (saturated)
    assert_int_equal(avg_sat.num_samples, UINT16_MAX);
    // summation should remain unchanged since sample was rejected
    assert_int_equal(avg_sat.summation, 5000 * UINT16_MAX);

    // Test case: Many small samples to verify precision
    running_u16_avg_t avg_small = {0};
    for (int i = 1; i <= 100; i++)
    {
        data_util_running_avg_u16_add_sample(&avg_small, i);
    }
    // Sum of 1 to 100 = 5050, average = 5050/100 = 50.5, rounded to 51
    assert_int_equal(data_util_running_avg_u16_get(&avg_small), 51);
    assert_int_equal(avg_small.summation, 5050);
    assert_int_equal(avg_small.num_samples, 100);

    // Test case: Alternating high/low values
    running_u16_avg_t avg_alt = {0};
    data_util_running_avg_u16_add_sample(&avg_alt, 10000);
    data_util_running_avg_u16_add_sample(&avg_alt, 0);
    data_util_running_avg_u16_add_sample(&avg_alt, 10000);
    data_util_running_avg_u16_add_sample(&avg_alt, 0);
    // Should sum to 20000, average to 20000/4 = 5000
    assert_int_equal(data_util_running_avg_u16_get(&avg_alt), 5000);
    assert_int_equal(avg_alt.summation, 20000);
    assert_int_equal(avg_alt.num_samples, 4);

    // Test case: Verify rounding behavior
    running_u16_avg_t avg_round = {0};
    data_util_running_avg_u16_add_sample(&avg_round, 1);
    data_util_running_avg_u16_add_sample(&avg_round, 2);
    // Expected: (1 + 2) / 2 = 1.5, rounded to 2
    assert_int_equal(data_util_running_avg_u16_get(&avg_round), 2);
    assert_int_equal(avg_round.summation, 3);

    data_util_running_avg_u16_add_sample(&avg_round, 2);
    // Expected: (1 + 2 + 2) / 3 = 5/3 = 1.67, rounded to 2
    assert_int_equal(data_util_running_avg_u16_get(&avg_round), 2);
    assert_int_equal(avg_round.summation, 5);

    // Test case: Reset and verify clean state
    data_util_running_avg_u16_reset(&avg);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 0);
    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Verify functionality after reset
    data_util_running_avg_u16_add_sample(&avg, 42);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 42);
    assert_int_equal(avg.summation, 42);
    assert_int_equal(avg.num_samples, 1);

    // Test case: Summation overflow protection
    running_u16_avg_t avg_overflow = {0};
    avg_overflow.summation = UINT32_MAX - 1000; // Close to overflow
    avg_overflow.num_samples = 1;

    // This should succeed (no overflow)
    data_util_running_avg_u16_add_sample(&avg_overflow, 500);
    assert_int_equal(avg_overflow.summation, UINT32_MAX - 500);
    assert_int_equal(avg_overflow.num_samples, 2);

    // This should be rejected (would overflow)
    data_util_running_avg_u16_add_sample(&avg_overflow, 1000);
    // summation and num_samples should remain unchanged
    assert_int_equal(avg_overflow.summation, UINT32_MAX - 500);
    assert_int_equal(avg_overflow.num_samples, 2);
}

TEST_FUNCTION(test_data_util_running_avg_u16_get, test_setup, test_teardown)
{
    running_u16_avg_t avg = {0};

    // Test case: NULL pointer (should return 0 and log error)
    uint16_t result = data_util_running_avg_u16_get(nullptr);
    assert_int_equal(result, 0);

    // Test case: Zero state (should return 0)
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 0);

    // Test case: Single sample
    avg.summation = 12345;
    avg.num_samples = 1;
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 12345);

    // Test case: Multiple samples
    avg.summation = 6000;
    avg.num_samples = 3;
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 2000); // 6000/3 = 2000

    // Test case: Maximum value summation
    avg.summation = UINT16_MAX;
    avg.num_samples = 1;
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, UINT16_MAX);

    // Test case: After running average calculations
    data_util_running_avg_u16_reset(&avg);
    data_util_running_avg_u16_add_sample(&avg, 1000);
    data_util_running_avg_u16_add_sample(&avg, 2000);
    data_util_running_avg_u16_add_sample(&avg, 3000);
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 2000); // (1000 + 2000 + 3000) / 3 = 2000

    // Test case: Verify return type is consistent with calculation
    avg.summation = 45678;
    avg.num_samples = 5;
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 9136); // (45678 + 2) / 5 = 9136 (rounded)

    // Test case: Edge case with small values and rounding
    data_util_running_avg_u16_reset(&avg);
    data_util_running_avg_u16_add_sample(&avg, 1);
    data_util_running_avg_u16_add_sample(&avg, 2);
    data_util_running_avg_u16_add_sample(&avg, 3);
    result = data_util_running_avg_u16_get(&avg);
    assert_int_equal(result, 2); // (1 + 2 + 3) / 3 = 6/3 = 2

    // Test case: Overflow protection - large summation
    avg.summation = UINT32_MAX;
    avg.num_samples = UINT16_MAX;
    result = data_util_running_avg_u16_get(&avg);
    // Should not overflow: UINT32_MAX / UINT16_MAX = 65537
    // But result is clamped to UINT16_MAX
    assert_int_equal(result, UINT16_MAX);
}

TEST_FUNCTION(test_data_util_running_avg_u16_reset, test_setup, test_teardown)
{
    running_u16_avg_t avg = {0};

    // Test case: Reset from non-zero state
    avg.summation = 12345;
    avg.num_samples = 999;

    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Reset from zero state (should remain zero)
    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Reset from maximum values
    avg.summation = UINT32_MAX;
    avg.num_samples = UINT16_MAX;

    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Verify functionality after reset by adding samples
    data_util_running_avg_u16_add_sample(&avg, 100);
    data_util_running_avg_u16_add_sample(&avg, 200);

    assert_int_equal(data_util_running_avg_u16_get(&avg), 150);
    assert_int_equal(avg.summation, 300);
    assert_int_equal(avg.num_samples, 2);

    // Reset again and verify
    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 0);

    // Test case: Multiple resets in succession
    data_util_running_avg_u16_reset(&avg);
    data_util_running_avg_u16_reset(&avg);
    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: NULL pointer (should not crash and log error)
    data_util_running_avg_u16_reset(nullptr);

    // Test case: Reset after saturation conditions
    avg.summation = UINT32_MAX - 10;
    avg.num_samples = UINT16_MAX - 5;

    data_util_running_avg_u16_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Verify we can add samples normally after reset from saturation
    data_util_running_avg_u16_add_sample(&avg, 500);
    assert_int_equal(data_util_running_avg_u16_get(&avg), 500);
    assert_int_equal(avg.summation, 500);
    assert_int_equal(avg.num_samples, 1);
}

TEST_FUNCTION(test_data_util_running_avg_u32_get, test_setup, test_teardown)
{
    running_u32_avg_t avg = {0};

    // Test case: NULL pointer (should return 0 and log error)
    uint32_t result = data_util_running_avg_u32_get(nullptr);
    assert_int_equal(result, 0);

    // Test case: Zero state (should return 0)
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 0);

    // Test case: Single sample
    avg.summation = 12345;
    avg.num_samples = 1;
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 12345);

    // Test case: Multiple samples
    avg.summation = 6000;
    avg.num_samples = 3;
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 2000); // 6000/3 = 2000

    // Test case: Maximum value summation
    avg.summation = UINT32_MAX;
    avg.num_samples = 1;
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, UINT32_MAX);

    // Test case: After running average calculations
    data_util_running_avg_u32_reset(&avg);
    data_util_running_avg_u32_add_sample(&avg, 1000);
    data_util_running_avg_u32_add_sample(&avg, 2000);
    data_util_running_avg_u32_add_sample(&avg, 3000);
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 2000); // (1000 + 2000 + 3000) / 3 = 2000

    // Test case: Verify return type is consistent with calculation
    avg.summation = 45678;
    avg.num_samples = 5;
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 9136); // (45678 + 2) / 5 = 9136 (rounded)

    // Test case: Edge case with small values and rounding
    data_util_running_avg_u32_reset(&avg);
    data_util_running_avg_u32_add_sample(&avg, 1);
    data_util_running_avg_u32_add_sample(&avg, 2);
    data_util_running_avg_u32_add_sample(&avg, 3);
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, 2); // (1 + 2 + 3 + 1) / 3 = 7/3 = 2 (rounded)

    // Test case: Overflow protection - large summation that would exceed UINT32_MAX
    avg.summation = UINT64_MAX;
    avg.num_samples = 2;
    result = data_util_running_avg_u32_get(&avg);
    // Should clamp to UINT32_MAX since result exceeds UINT32_MAX range
    assert_int_equal(result, UINT32_MAX);

    // Test case: Large summation within UINT32_MAX range
    avg.summation = (uint64_t)UINT32_MAX * 2;
    avg.num_samples = 2;
    result = data_util_running_avg_u32_get(&avg);
    assert_int_equal(result, UINT32_MAX); // UINT32_MAX * 2 / 2 = UINT32_MAX
}

TEST_FUNCTION(test_data_util_running_avg_u32_reset, test_setup, test_teardown)
{
    running_u32_avg_t avg = {0};

    // Test case: Reset from non-zero state
    avg.summation = 54321;
    avg.num_samples = 999;

    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Reset from zero state (should remain zero)
    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Reset from maximum values
    avg.summation = UINT64_MAX;
    avg.num_samples = UINT32_MAX;

    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: Verify functionality after reset by adding samples
    data_util_running_avg_u32_add_sample(&avg, 100);
    data_util_running_avg_u32_add_sample(&avg, 200);

    assert_int_equal(data_util_running_avg_u32_get(&avg), 150);
    assert_int_equal(avg.summation, 300);
    assert_int_equal(avg.num_samples, 2);

    // Reset again and verify
    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 0);

    // Test case: Multiple resets in succession
    data_util_running_avg_u32_reset(&avg);
    data_util_running_avg_u32_reset(&avg);
    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Test case: NULL pointer (should not crash and log error)
    data_util_running_avg_u32_reset(nullptr);

    // Test case: Reset after saturation conditions
    avg.summation = UINT64_MAX - 10;
    avg.num_samples = UINT32_MAX - 5;

    data_util_running_avg_u32_reset(&avg);

    assert_int_equal(avg.summation, 0);
    assert_int_equal(avg.num_samples, 0);

    // Verify we can add samples normally after reset from saturation
    data_util_running_avg_u32_add_sample(&avg, 500);
    assert_int_equal(data_util_running_avg_u32_get(&avg), 500);
    assert_int_equal(avg.summation, 500);
    assert_int_equal(avg.num_samples, 1);
}
