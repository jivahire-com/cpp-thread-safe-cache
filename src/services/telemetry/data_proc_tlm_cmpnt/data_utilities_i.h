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
