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