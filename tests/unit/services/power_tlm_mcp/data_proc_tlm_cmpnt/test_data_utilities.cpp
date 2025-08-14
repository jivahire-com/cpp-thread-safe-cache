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

    data_util_calc_mma_u16(nullptr, 150);
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
