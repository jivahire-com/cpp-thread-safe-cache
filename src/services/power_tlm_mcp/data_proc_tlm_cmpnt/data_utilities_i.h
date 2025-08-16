//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_utilities_i.h
 * Helper functions for data processing
 */

#pragma once

/*----------- Nested includes ------------*/

#include "data_sampling_i.h" // internal APIs

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/


// use the 32 bit cumulative average when the sample size is <= 16 bit
typedef struct
 {
    uint32_t sum;         // Running sum of all samples
    uint16_t num_samples; // Number of samples added
} cumulative_u16_avg_t;

typedef struct {
    cumulative_u16_avg_t cumulative_avg;
    uint16_t min;
    uint16_t max;
} mma_u16_t, *p_mma_u16_t;

// use the 32 bit running average when the sample size is greater than 16 bit
typedef struct {
    uint32_t average;
    uint16_t num_samples;
} running_u32_avg_t;

typedef struct {
    running_u32_avg_t running_avg;
    uint32_t min;
    uint32_t max;
} mma_u32_t, *p_mma_u32_t;

// use the duration average when the samples are not periodic and are weighted by duration
typedef struct {
    uint64_t sum_weighted;   // Accumulated value × duration_ms
    uint32_t total_time_uS;  // Accumulated time in milliseconds
} running_avg_dur_t;

typedef struct {
    running_avg_dur_t running_avg_dur;
    uint16_t min;
    uint16_t max;
} mma_u16_dur_t, *p_mma_u16_dur_t;

// use the moving average when its low pass filter characteristics are needed
typedef struct {
    uint16_t* samples;         // Pointer to circular sample array
    uint32_t  total_sum;       // Running sum of all values
    uint16_t   sample_capacity; // Total capacity of the sample array
    uint16_t   sample_index;    // Current circular write index
    uint16_t   sample_count;    // Current number of valid samples (<= sample_capacity)
} moving_avg_u16_t;

typedef struct {
    uint32_t* samples;          // Pointer to circular sample array
    uint32_t  total_sum;        // Running sum of all values
    uint16_t   sample_capacity; // Total capacity of the sample array
    uint16_t   sample_index;    // Current circular write index
    uint16_t   sample_count;    // Current number of valid samples (<= sample_capacity)
} moving_avg_u32_t;


/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief data_util_get_max_val function returns the maximum value among three given values.
 *
 * @param[in] val0 - First value to compare.
 * @param[in] val1 - Second value to compare.
 * @param[in] val2 - Third value to compare.
 *
 * @return The maximum value among the three input values.
 */
int16_t data_util_get_max_val(int16_t val0, int16_t val1, int16_t val2);

/**
 * @brief data_util_calc_time_diff_and_update function calculates the time difference between the current timestamp
 *        and the last recorded timestamp, updating the last timestamp in the process.
 *
 * @param[in,out] last_timestamp_uS - Pointer to the last recorded timestamp in microseconds.
 * @param[in,out] current_time_stamp_uS - Pointer to the current timestamp in microseconds.
 * @param[in,out] difference_uS - Difference in uS between timestamps.
 *
 * @return None
 */
void data_util_calc_time_diff_and_update(uint64_t* last_timestamp_uS, uint64_t* current_time_stamp_uS, uint64_t* difference_uS);

/**
 * @brief Update the min, max, and average using the latest value. Only supports uint16_t values.
 *
 * @param[in,out] mma - Pointer to the mma_u16_t structure containing min, max, and average values.
 * @param[in] mma_latest_value - The latest value to be used for updating the min, max, and average.
 *
 * @return None
 */
void data_util_calc_mma_u16(mma_u16_t* mma, uint16_t mma_latest_value);

/**
 * @brief Update the min, max, and average using the latest value. Supports uint32_t values.
 * @param[in,out] mma - Pointer to the mma_u32_t structure containing min, max, and average values.
 * @param[in] mma_latest_value - The latest value to be used for updating the min, max, and average.
 *
 * @return None
 */
void data_util_calc_mma_u32(mma_u32_t* mma, uint32_t mma_latest_value);

/**
 * @brief Update the min, max, and average using the latest value. Only supports uint16_t values.
 *
 * @param[in,out] mma_dur - Pointer to the mma_u16_dur_t structure containing min, max, and average values.
 * @param[in] mma_latest_value - The latest value to be used for updating the min, max, and average.
 * @param[in] duration_uS - The duration in microseconds for which the value is valid.
 *
 * @return None
 */
void data_util_calc_mma_dur_u16(mma_u16_dur_t* mma_dur, uint16_t mma_latest_value, uint32_t duration_uS);

/**
 * @brief data_util_convert_systick_to_microseconds function converts a given tick count to microseconds.
 *
 * @param[in] tick_count - The tick count to be converted.
 *
 * @return The converted time in microseconds.
 */
uint64_t data_util_convert_systick_to_microseconds(uint64_t tick_count);

/** * @brief data_util_cumulative_avg_u16_add_sample function adds a sample to the cumulative average structure.
 *
 * @param[in,out] avg - Pointer to the cumulative_u16_avg_t structure where the sample will be added.
 * @param[in] sample - The sample value to be added to the cumulative average.
 */
void data_util_cumulative_avg_u16_add_sample(cumulative_u16_avg_t *avg, uint16_t sample);

/** * @brief data_util_cumulative_avg_u16_get function retrieves the current cumulative average from the structure.
 *
 * @param[in] avg - Pointer to the cumulative_u16_avg_t structure from which the average will be retrieved.
 * @return The current cumulative average value.
 */
uint16_t data_util_cumulative_avg_u16_get(const cumulative_u16_avg_t *avg);

/** * @brief data_util_cumulative_avg_u16_reset function resets the cumulative average to zero.
 *
 * @param[in,out] avg - Pointer to the cumulative_u16_avg_t structure to be reset.
 */
void data_util_cumulative_avg_u16_reset(cumulative_u16_avg_t* avg);

/** * @brief data_util_running_avg_u32_add_sample function adds a sample to the running average structure.
 *
 * @param[in,out] avg - Pointer to the running_u32_avg_t structure where the sample will be added.
 * @param[in] sample - The sample value to be added to the running average.
 */
void data_util_running_avg_u32_add_sample(running_u32_avg_t* avg, uint32_t sample);

/** * @brief data_util_running_avg_u32_get function retrieves the current running average from the structure.
 *
 * @param[in] avg - Pointer to the running_u32_avg_t structure from which the average will be retrieved.
 * @return The current running average value.
 */
uint32_t data_util_running_avg_u32_get(const running_u32_avg_t* avg);

/** * @brief data_util_running_avg_u32_reset function resets the running average to zero.
 *
 * @param[in,out] avg - Pointer to the running_u32_avg_t structure to be reset.
 */
void data_util_running_avg_u32_reset(running_u32_avg_t* avg);

/**
 * @brief data_util_running_avg_dur_reset function resets the running average duration data to zero.
 *
 * @param[in,out] ra - Pointer to the running average duration structure to be reset.
 */
void data_util_running_avg_dur_reset(running_avg_dur_t *ra);

/**
 * @brief data_util_running_avg_dur_update function updates the running average duration with a new value and its duration.
 *
 * @param[in,out] ra - Pointer to the running average duration structure to be updated.
 * @param[in] value - The new value to be added to the running average duration.
 * @param[in] duration_uS - The duration in microseconds for which the value is valid.
 */
void data_util_running_avg_dur_update(running_avg_dur_t *ra, uint16_t value, uint32_t duration_uS);

/**
 * @brief data_util_running_avg_dur_get function retrieves the average value from the running average duration structure.
 *
 * @param[in] ra - Pointer to the running average duration structure.
 *
 * @return The average value calculated from the running average duration.
 */
uint16_t data_util_running_avg_dur_get(const running_avg_dur_t *ra);

/**
 * @brief data_util_mean_of_means function calculates the mean of two means, taking into account their respective counts.
 *
 * @param[in] mean1 - The first mean value.
 * @param[in] count1 - The count associated with the first mean.
 * @param[in] mean2 - The second mean value.
 * @param[in] count2 - The count associated with the second mean.
 *
 * @return The calculated mean of the two means.
 */
uint16_t data_util_mean_of_means(uint16_t mean1, uint16_t count1, uint16_t mean2, uint16_t count2);

/**
 * @brief data_util_mean_of_summations function calculates the mean of two summations, taking into account their respective counts.
 *
 * @param[in] summation1 - The first summation value.
 * @param[in] count1 - The count associated with the first summation.
 * @param[in] summation2 - The second summation value.
 * @param[in] count2 - The count associated with the second summation.
 *
 * @return The calculated mean of the two summations.
 */
uint16_t data_util_mean_of_summations(uint32_t summation1, uint16_t count1, uint32_t summation2, uint16_t count2);

/**
 * @brief Returns the MAX(summation1/count1, summation2/count2).
 *
 * @param[in] summation1 - The first summation value.
 * @param[in] count1 - The count associated with the first summation.
 * @param[in] summation2 - The second summation value.
 * @param[in] count2 - The count associated with the second summation.
 *
 * @return The calculated maximum average of the two summations.
 */
uint16_t data_util_max_avg_of_summations(uint32_t summation1, uint16_t count1, uint32_t summation2, uint16_t count2);

/**
 * Moving Average handler
 *
 * Implements an efficient circular-buffer based simple moving average for
 * streaming uint16_t data.
 *
 * - The caller provides external storage for the sample buffer.
 * - Each new sample replaces the oldest sample when the buffer is full.
 * - The total sum is maintained incrementally to allow O(1) insertion and O(1) average calculation.
 * - Overflow protection is applied on the running sum; if an overflow occurs during summation,
 *   the total sum saturates to UINT32_MAX.
 * - Invalid parameters (NULL pointers or zero capacity) are safely rejected by initialization.
 * - After initialization, all public functions are safe to call with NULL; they will return 0 or no-op.
 * - The `data_util_mov_avg_u16_clear()` function clears sample history while preserving configuration.
 */

 /**
 * Initialize moving average structure.
 *
 * @param[in/out] ma Pointer to moving_avg_u16_t object.
 * @param[in/out] samples Pointer to user-provided sample array (uint16_t array).
 * @param[in] sample_capacity Capacity of the sample array ie, number of uint16_t samples it can hold.
 * @return None
 */
void data_util_mov_avg_u16_init(moving_avg_u16_t* ma, uint16_t* samples, uint16_t sample_capacity);

/**
 * Add one sample to the moving average.
 *
 * @param ma Pointer to moving_avg_u16_t object.
 * @param value The new sample to add.
 * @return None
 */
void data_util_mov_avg_u16_add_sample(moving_avg_u16_t* ma, uint16_t value);

/**
 * Retrieve the current moving average.
 *
 * @param ma Pointer to moving_avg_u16_t object.
 * @return Current average, or 0 if no samples or NULL input.
 * @return None
 */
uint16_t data_util_mov_avg_u16_get(const moving_avg_u16_t* ma);

/**
 * Clear accumulated samples and reset running sum.
 *
 * Keeps configuration and sample buffer intact.
 *
 * @param ma Pointer to moving_avg_u16_t object.
 * @return None
 */
void data_util_mov_avg_u16_clear(moving_avg_u16_t* ma);

/**
 * Moving Average handler
 *
 * Implements an efficient circular-buffer based simple moving average for
 * streaming uint32_t data.
 *
 * - The caller provides external storage for the sample buffer.
 * - Each new sample replaces the oldest sample when the buffer is full.
 * - The total sum is maintained incrementally to allow O(1) insertion and O(1) average calculation.
 * - Overflow protection is applied on the running sum; if an overflow occurs during summation,
 *   the total sum saturates to UINT32_MAX.
 * - Invalid parameters (NULL pointers or zero capacity) are safely rejected by initialization.
 * - After initialization, all public functions are safe to call with NULL; they will return 0 or no-op.
 * - The `data_util_mov_avg_u32_clear()` function clears sample history while preserving configuration.
 */

/**
 * Initialize moving average structure.
 *
 * @param[in/out] ma Pointer to moving_avg_u32_t object.
 * @param[in/out] samples Pointer to user-provided sample array (uint32_t array).
 * @param[in] sample_capacity Capacity of the sample array ie, number of uint16_t samples it can hold.
 * @return None
 */
void data_util_mov_avg_u32_init(moving_avg_u32_t* ma, uint32_t* samples, uint16_t sample_capacity);

/**
 * Add one sample to the moving average.
 *
 * @param ma Pointer to moving_avg_u32_t object.
 * @param value The new sample to add.
 * @return None
 */
void data_util_mov_avg_u32_add_sample(moving_avg_u32_t* ma, uint32_t value);

/**
 * Retrieve the current moving average.
 *
 * @param ma Pointer to moving_avg_u32_t object.
 * @return Current average, or 0 if no samples or NULL input.
 * @return None
 */
uint32_t data_util_mov_avg_u32_get(const moving_avg_u32_t* ma);

/**
 * Clear accumulated samples and reset running sum.
 *
 * Keeps configuration and sample buffer intact.
 *
 * @param ma Pointer to moving_avg_u32_t object.
 * @return None
 */
void data_util_mov_avg_u32_clear(moving_avg_u32_t* ma);