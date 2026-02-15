//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_utilities.c
 * Helper functions for data processing
 */

/*------------- Includes -----------------*/
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "telemetry_events_i.h"

#include <exec_tlm_cmpnt.h>
#include <gtimer_prodfw.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MICROSECONDS_PER_SECOND (1000000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int16_t data_util_get_max_val(int16_t val0, int16_t val1, int16_t val2)
{
    int16_t ret_val = (val0 > val1) ? val0 : val1;
    ret_val = (ret_val > val2) ? ret_val : val2;
    return ret_val;
}

void data_util_calc_time_diff_and_update(uint64_t* last_timestamp_uS, uint64_t* current_time_stamp_uS, uint64_t* difference_uS)
{
    if (last_timestamp_uS == NULL || current_time_stamp_uS == NULL || difference_uS == NULL)
    {
        FPFW_ET_LOG(DataUtilCalcError);
        return;
    }

    if (*current_time_stamp_uS > *last_timestamp_uS)
    {
        *difference_uS = (*current_time_stamp_uS - *last_timestamp_uS);
    }
    else
    {
        *difference_uS = 0;
    }
    *last_timestamp_uS = *current_time_stamp_uS;
}

void data_util_calc_mma_u16(mma_u16_t* mma, uint16_t mma_latest_value)
{
    if (mma == NULL)
    {
        FPFW_ET_LOG(MMAU16NullPointer);
        return;
    }
    if (mma_latest_value > mma->max)
    {
        mma->max = mma_latest_value;
    }
    if ((mma_latest_value < mma->min) || (mma->running_avg.num_samples == 0))
    {
        mma->min = mma_latest_value;
    }
    data_util_running_avg_u16_add_sample(&mma->running_avg, mma_latest_value);
}

void data_util_calc_mma_u32(mma_u32_t* mma, uint32_t mma_latest_value)
{
    if (mma == NULL)
    {
        FPFW_ET_LOG(MMAU32NullPointer);
        return;
    }
    if (mma_latest_value > mma->max)
    {
        mma->max = mma_latest_value;
    }
    if ((mma_latest_value < mma->min) || (mma->running_avg.num_samples == 0))
    {
        mma->min = mma_latest_value;
    }
    data_util_running_avg_u32_add_sample(&mma->running_avg, mma_latest_value);
}

void data_util_calc_mma_dur_u16(mma_u16_dur_t* mma_dur, uint16_t mma_latest_value, uint32_t duration_uS)
{
    if (mma_dur == NULL)
    {
        FPFW_ET_LOG(MMAU16NullPointer);
        return;
    }

    if (mma_latest_value > mma_dur->max)
    {
        mma_dur->max = mma_latest_value;
    }

    if ((mma_latest_value < mma_dur->min) || (mma_dur->min == 0))
    {
        mma_dur->min = mma_latest_value;
    }
    data_util_running_avg_dur_update(&mma_dur->running_avg_dur, mma_latest_value, duration_uS);
}

uint64_t data_util_convert_systick_to_microseconds(uint64_t tick_count)
{
    uint32_t frequency = gtimer_prodfw_get_frequency();

    if (frequency == 0)
    {
        FPFW_ET_LOG(GtimerIsZero);
        return 0; // Avoid division by zero
    }

    uint64_t timestamp_us = (tick_count * MICROSECONDS_PER_SECOND) / frequency;
    return timestamp_us;
}

/**
 * @brief Add a sample to the 16-bit running average structure using cumulative sum approach
 *
 * This function implements a robust cumulative sum algorithm that replaced an alternative
 * incremental averaging method due to critical precision loss issues.
 *
 * ALGORITHM COMPARISON:
 * =====================
 * Alternative Implementation (Incremental Averaging):
 *   avg->average = ((avg->average * avg->num_samples) + sample) / (avg->num_samples + 1);
 *   avg->num_samples++;
 *
 * Critical Shortcomings of Alternative Method:
 * --------------------------------------------
 * 1. PRECISION LOSS: Integer division in avg->average calculation caused truncation
 *    General case: When accumulated samples have mixed values (zeros and non-zeros),
 *    repeated integer division causes progressive information loss
 *
 *    Concrete example with 40,000 samples (20,000 zeros + 20,000 of value 840):
 *    - Mathematically correct average: 420
 *    - Alternative method result: Often 0 due to repeated truncation
 *
 * 2. "STUCK AT ZERO" BUG: When average became 0, formula became:
 *    new_avg = (0 * count + sample) / (count + 1) = sample / (count + 1)
 *    For large sample counts, this approaches 0, making recovery impossible
 *    regardless of subsequent sample values. Since the incremental increase
 *    (sample / (count + 1)) is less than 1, integer truncation prevents any
 *    movement from zero, creating a permanent stuck state
 *
 * 3. CUMULATIVE ERROR: Each division step introduced error that compounded over time,
 *    causing drift from the true mathematical average
 *
 * 4. NON-DETERMINISTIC: Order of samples affected final result due to rounding errors,
 *    violating mathematical properties of averages
 *
 * Current (Robust) Implementation:
 * ================================
 * Uses cumulative summation approach:
 * - Maintains running sum of all samples (summation field)
 * - Tracks total number of samples (num_samples field)
 * - Average calculated on-demand as: (summation + num_samples/2) / num_samples
 *
 * Benefits of Current Method:
 * ---------------------------
 * 1. MATHEMATICALLY CORRECT: Always produces correct average regardless of sample order
 * 2. NO PRECISION LOSS: Summation preserves all information until final division
 * 3. DETERMINISTIC: Same set of samples always produces same result
 * 4. OVERFLOW PROTECTION: Prevents arithmetic overflow with proper bounds checking
 * 5. SATURATION HANDLING: Gracefully handles edge cases at maximum values
 *
 * OVERFLOW PROTECTION STRATEGY:
 * =============================
 * Two-tier protection system:
 * 1. Sample Count Saturation: Stop accepting samples when num_samples reaches UINT16_MAX
 * 2. Summation Overflow Protection: Reject samples that would cause summation to exceed UINT32_MAX
 *
 * Example Scenarios:
 * ==================
 * Precision Recovery Case (40,000 samples):
 * - 20,000 samples of 0: summation = 0, num_samples = 20,000
 * - 20,000 samples of 840: summation = 16,800,000, num_samples = 40,000
 * - Final average: 16,800,000 / 40,000 = 420 (mathematically correct!)
 *
 * General High-Volume Case (N samples with mixed values):
 * - Summation maintains exact total regardless of sample order
 * - Average = total_sum / sample_count (always mathematically precise)
 * - No information loss until final division (with proper rounding)
 *
 * @param[in,out] avg - Pointer to running average structure to update
 * @param[in] sample - New sample value to add to the running average
 *
 * @note This function will reject samples that would cause overflow, logging appropriate events
 * @note For mathematical derivation of rounding formula, see data_util_running_avg_u16_get()
 */
void data_util_running_avg_u16_add_sample(running_u16_avg_t* avg, uint16_t sample)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }

    // Stop accepting samples if num_samples is saturated
    if (avg->num_samples == UINT16_MAX)
    {
        // TODO: Re‑instate this change once the root cause of the issue mentioned above is identified and
        // validated. https://azurecsi.visualstudio.com/Dev/_workitems/edit/3334457
        FPFW_ET_LOG(RunningAvg16AddSampleNumSat, (uint32_t)avg);
        return;
    }

    // Check for overflow before adding
    if (avg->summation > UINT32_MAX - sample)
    {
        // TODO: Re‑instate this change once the root cause of the issue mentioned above is identified and
        // validated. https://azurecsi.visualstudio.com/Dev/_workitems/edit/3334457
        FPFW_ET_LOG(RunningAvg16AddSampleSumSat, (uint32_t)avg);
        return; // Don't add sample to prevent overflow
    }

    avg->summation += sample;
    avg->num_samples += 1;
}

uint16_t data_util_running_avg_u16_get(const running_u16_avg_t* avg)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return 0;
    }
    if (avg->num_samples == 0)
    {
        return 0;
    }

    // Use uint64_t to prevent overflow - plenty of headroom
    uint64_t result = ((uint64_t)avg->summation + avg->num_samples / 2) / avg->num_samples;

    // Clamp to UINT16_MAX to prevent overflow
    if (result > UINT16_MAX)
    {
        FPFW_ET_LOG(RunningAvg16GetClamp, (uint32_t)avg);
        return UINT16_MAX;
    }

    return (uint16_t)result;
}
void data_util_running_avg_u16_reset(running_u16_avg_t* avg)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }

    avg->summation = 0;
    avg->num_samples = 0;
}

/**
 * @brief See explanation for data_util_running_avg_u16_add_sample(), this is the larger 32 bit version
 */
void data_util_running_avg_u32_add_sample(running_u32_avg_t* avg, uint32_t sample)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }

    // Stop accepting samples if num_samples is saturated
    if (avg->num_samples == UINT32_MAX)
    {
        // TODO: Re‑instate this change once the root cause of the issue mentioned above is identified and
        // validated. https://azurecsi.visualstudio.com/Dev/_workitems/edit/3334457
        FPFW_ET_LOG(RunningAvg32AddSampleNumSat, (uint32_t)avg);
        return;
    }

    // Check for overflow before adding
    if (avg->summation > UINT64_MAX - sample)
    {
        // TODO: Re‑instate this change once the root cause of the issue mentioned above is identified and
        // validated. https://azurecsi.visualstudio.com/Dev/_workitems/edit/3334457
        FPFW_ET_LOG(RunningAvg32AddSampleSumSat, (uint32_t)avg);
        return; // Don't add sample to prevent overflow
    }

    avg->summation += sample;
    avg->num_samples += 1;
}

uint32_t data_util_running_avg_u32_get(const running_u32_avg_t* avg)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return 0;
    }

    if (avg->num_samples == 0)
    {
        return 0;
    }

    uint64_t result;
    uint32_t half_samples = avg->num_samples / 2;

    // Check if adding rounding term would overflow
    if (avg->summation <= UINT64_MAX - half_samples)
    {
        // Safe to add rounding term
        result = (avg->summation + half_samples) / avg->num_samples;
    }
    else
    {
        // Close to overflow, calculate without rounding
        result = avg->summation / avg->num_samples;
    }

    // Clamp result to UINT32_MAX if it exceeds the range
    if (result > UINT32_MAX)
    {
        FPFW_ET_LOG(RunningAvg32GetClamp, (uint32_t)avg);
        return UINT32_MAX;
    }

    return (uint32_t)result;
}

void data_util_running_avg_u32_reset(running_u32_avg_t* avg)
{
    if (avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }

    avg->summation = 0;
    avg->num_samples = 0;
}

void data_util_running_avg_dur_reset(running_avg_dur_t* ra)
{
    if (ra == NULL)
    {
        FPFW_ET_LOG(RunningAvgDurResetNullPointer);
        return;
    }
    ra->sum_weighted = 0;
    ra->total_time_uS = 0;
}

void data_util_running_avg_dur_update(running_avg_dur_t* ra, uint16_t value, uint32_t duration_uS)
{
    if (ra == NULL)
    {
        FPFW_ET_LOG(RunningAvgDurUpdateNullPointer);
        return;
    }
    if (duration_uS == 0)
    {
        return;
    }

    ra->sum_weighted += (uint64_t)value * duration_uS;
    ra->total_time_uS += duration_uS;
}

uint16_t data_util_running_avg_dur_get(const running_avg_dur_t* ra)
{
    if (ra == NULL)
    {
        FPFW_ET_LOG(RunningAvgDurGetNullPointer);
        return 0;
    }
    if (ra->total_time_uS == 0)
    {
        return 0;
    }

    return (uint16_t)((ra->sum_weighted + ra->total_time_uS / 2) / ra->total_time_uS); // Rounded
}

uint16_t data_util_mean_of_means(uint16_t mean1, uint16_t count1, uint16_t mean2, uint16_t count2)
{
    // Reduce counts directly to ensure no overflow in (mean * count)
    // unlikely for counts to be that high and preferable to uint64_t calculations
    if (count1 > UINT16_MAX / 2)
    {
        FPFW_ET_LOG(MeanofMeansHighCount, count1);
        count1 = UINT16_MAX / 2;
    }

    if (count2 > UINT16_MAX / 2)
    {
        FPFW_ET_LOG(MeanofMeansHighCount, count2);
        count2 = UINT16_MAX / 2;
    }

    uint32_t weighted_sum1 = (uint32_t)mean1 * count1;
    uint32_t weighted_sum2 = (uint32_t)mean2 * count2;
    uint32_t total_sum = weighted_sum1 + weighted_sum2;
    uint32_t total_count = (uint32_t)count1 + count2;

    if (total_count == 0)
    {
        return 0; // Avoid division by zero, is expected to happen in some cases with zero'd out data
    }

    uint32_t mean = (total_sum + total_count / 2) / total_count; // round up

    // Clamping to UINT16_MAX is not needed either because of the same reasons as above.

    return mean;
}

uint16_t data_util_mean_of_summations(uint32_t summation1, uint16_t count1, uint32_t summation2, uint16_t count2)
{
    uint64_t total_sum = (uint64_t)summation1 + (uint64_t)summation2;
    uint32_t total_count = (uint32_t)count1 + (uint32_t)count2;

    if (total_count == 0)
    {
        return 0; // Avoid division by zero, is expected to happen in some cases with zero'd out data
    }

    uint64_t mean = ((uint64_t)total_sum + total_count / 2) / total_count; // round up

    if (mean > UINT16_MAX)
    {
        FPFW_ET_LOG(MeanofSummationHighCount, mean);
        mean = UINT16_MAX; // Saturate on overflow
    }

    return (uint16_t)mean;
}

uint16_t data_util_max_avg_of_summations(uint32_t summation1, uint16_t count1, uint32_t summation2, uint16_t count2)
{
    uint64_t avg1 = 0;
    if (count1 > 0)
    {
        avg1 = ((uint64_t)summation1 + count1 / 2) / count1; // round up, use uint64_t to avoid overflow
        if (avg1 > UINT16_MAX)
        {
            avg1 = UINT16_MAX;
        }
    }
    uint64_t avg2 = 0;
    if (count2 > 0)
    {
        avg2 = ((uint64_t)summation2 + count2 / 2) / count2; // round up, use uint64_t to avoid overflow
        if (avg2 > UINT16_MAX)
        {
            avg2 = UINT16_MAX;
        }
    }

    uint16_t max_avg = (avg1 > avg2) ? (uint16_t)avg1 : (uint16_t)avg2;

    return max_avg;
}

void data_util_mov_avg_u16_init(moving_avg_u16_t* ma, uint16_t* samples, uint16_t sample_capacity)
{
    if (ma == NULL || samples == NULL || sample_capacity == 0)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    ma->samples = samples;
    ma->sample_capacity = sample_capacity;
    ma->sample_index = 0;
    ma->sample_count = 0;
    ma->total_sum = 0;

    memset(samples, 0, sample_capacity * sizeof(uint16_t));
}

void data_util_mov_avg_u16_add_sample(moving_avg_u16_t* ma, uint16_t value)
{
    if ((ma == NULL) || (ma->samples == NULL) || (ma->sample_capacity == 0))
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    uint16_t old_value = ma->samples[ma->sample_index];
    ma->total_sum -= old_value;

    uint32_t new_total = ma->total_sum + value;
    if (new_total < ma->total_sum)
    {
        ma->total_sum = UINT32_MAX; // Saturate on overflow
    }
    else
    {
        ma->total_sum = new_total;
    }

    ma->samples[ma->sample_index] = value;

    if (ma->sample_count < ma->sample_capacity)
    {
        ma->sample_count++;
    }

    ma->sample_index = (ma->sample_index + 1) % ma->sample_capacity;
}

uint16_t data_util_mov_avg_u16_get(const moving_avg_u16_t* ma)
{
    if (ma == NULL)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return 0;
    }
    if (ma->sample_count == 0)
    {
        return 0; // don't log error since valid case.
    }

    return (uint16_t)(ma->total_sum / ma->sample_count);
}

void data_util_mov_avg_u16_clear(moving_avg_u16_t* ma)
{
    if (ma == NULL)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    memset(ma->samples, 0, ma->sample_capacity * sizeof(uint16_t));
    ma->sample_index = 0;
    ma->sample_count = 0;
    ma->total_sum = 0;
}

void data_util_mov_avg_u32_init(moving_avg_u32_t* ma, uint32_t* samples, uint16_t sample_capacity)
{
    if (ma == NULL || samples == NULL || sample_capacity == 0)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    ma->samples = samples;
    ma->sample_capacity = sample_capacity;
    ma->sample_index = 0;
    ma->sample_count = 0;
    ma->total_sum = 0;

    memset(samples, 0, sample_capacity * sizeof(uint32_t));
}

void data_util_mov_avg_u32_add_sample(moving_avg_u32_t* ma, uint32_t value)
{
    if ((ma == NULL) || (ma->samples == NULL) || (ma->sample_capacity == 0))
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    if (ma->total_sum == UINT32_MAX)
    {
        return; // Avoid adding samples if already saturated
    }

    uint32_t old_value = ma->samples[ma->sample_index];
    ma->total_sum -= old_value;

    uint32_t new_total = ma->total_sum + value;
    if (new_total < ma->total_sum)
    {
        ma->total_sum = UINT32_MAX; // Saturate on overflow
    }
    else
    {
        ma->total_sum = new_total;
    }

    ma->samples[ma->sample_index] = value;

    if (ma->sample_count < ma->sample_capacity)
    {
        ma->sample_count++;
    }

    ma->sample_index = (ma->sample_index + 1) % ma->sample_capacity;
}

uint32_t data_util_mov_avg_u32_get(const moving_avg_u32_t* ma)
{
    if (ma == NULL)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return 0;
    }
    if (ma->sample_count == 0)
    {
        return 0; // don't log error since valid case.
    }

    return (ma->total_sum / ma->sample_count);
}

void data_util_mov_avg_u32_clear(moving_avg_u32_t* ma)
{
    if (ma == NULL)
    {
        FPFW_ET_LOG(MovingAvgParamError);
        return;
    }

    memset(ma->samples, 0, ma->sample_capacity * sizeof(uint32_t));
    ma->sample_index = 0;
    ma->sample_count = 0;
    ma->total_sum = 0;
}
