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

typedef struct {
    uint16_t average;
    uint16_t num_samples;
} running_avg_t;

typedef struct {
    uint16_t min;
    uint16_t max;
    running_avg_t running_avg;
} mma_u16_t, *p_mma_u16_t;

typedef struct {
    uint16_t* samples;         // Pointer to circular sample array
    uint16_t   sample_capacity; // Total capacity of the sample array
    uint16_t   sample_index;    // Current circular write index
    uint16_t   sample_count;    // Current number of valid samples (<= sample_capacity)
    uint32_t  total_sum;       // Running sum of all values
} moving_avg_u16_t;

typedef struct {
    uint32_t* samples;          // Pointer to circular sample array
    uint16_t   sample_capacity; // Total capacity of the sample array
    uint16_t   sample_index;    // Current circular write index
    uint16_t   sample_count;    // Current number of valid samples (<= sample_capacity)
    uint32_t  total_sum;        // Running sum of all values
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
 * @brief data_util_calc_time_diff function calculates the time difference between the current timestamp
 *        and the previous timestamp. It updates the previous timestamp with the current timestamp.
 *
 * @param[in,out] previous_timestamp_uS - Pointer to the variable that stores the previous timestamp in microseconds.
 * @param[in,out] time_stamp_uS - Pointer to the variable that stores the current timestamp in microseconds.
 * @param[in] update_type - The type of update (e.g., SOC update).
 *
 * @return The calculated time difference in microseconds.
 */
uint64_t data_util_calc_time_diff(uint64_t* previous_timestamp_uS, uint64_t* time_stamp_uS, pwr_tlm_update_t update_type);

/**
 * @brief data_util_calc_mma_res function calculates the minimum, maximum, and average values of a
 *          given metric over a specified time period. It updates the provided pointers with the
 *          calculated results based on the latest value and the time difference.
 *
 * @param[in,out] mma_min -Pointer to the variable that stores the minimum value of the metric.
 *                 This value will be updated based on the latest value and the time difference.
 * @param[in,out] mma_max -Pointer to the variable that stores the maximum value of the metric.
 * @param[in,out] mma_average -Pointer to the variable that stores the average value of the metric.
 * @param[in] mma_latest_value  -Pointer to the variable that stores the latest value of the metric
 * @param[in] time_diff_uS - The time difference in microseconds between the current and previous measurements
 * @param[in] residency_uS The total residency time in microseconds over which the average is calculated
 */
void data_util_calc_mma_res(uint16_t* mma_min, uint16_t* mma_max, uint16_t* mma_average, uint16_t* mma_latest_value, uint32_t time_diff_uS, uint32_t residency_uS);

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
 * @brief data_util_convert_systick_to_microseconds function converts a given tick count to microseconds.
 *
 * @param[in] tick_count - The tick count to be converted.
 *
 * @return The converted time in microseconds.
 */
uint64_t data_util_convert_systick_to_microseconds(uint64_t tick_count);

/**
 * @brief data_util_running_avg_update function updates the running average with a new value.
 *
 * @param[in,out] running_avg - Pointer to the running average structure to be updated.
 * @param[in] new_value - The new value to be added to the running average.
 */
void data_util_running_avg_update(running_avg_t *running_avg, uint16_t new_value);

/**
 * @brief data_util_running_avg_reset function resets the running average structure to its initial state.
 *
 * @param[in,out] running_avg - Pointer to the running average structure to be reset.
 */
void data_util_running_avg_reset(running_avg_t *running_avg);

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