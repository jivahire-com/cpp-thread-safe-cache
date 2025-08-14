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
    if ((mma_latest_value < mma->min) || (mma->min == 0))
    {
        mma->min = mma_latest_value;
    }
    data_util_running_avg_update(&mma->running_avg, mma_latest_value);
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

void data_util_running_avg_update(running_avg_t* running_avg, uint16_t new_value)
{
    if (running_avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }

    if (running_avg->num_samples == 0)
    {
        running_avg->average = new_value;
        running_avg->num_samples = 1;
        return;
    }

    // Cap sample count at UINT16_MAX to prevent rollover
    if (running_avg->num_samples < UINT16_MAX)
    {
        running_avg->num_samples += 1;
    }

    uint32_t total = (uint32_t)(running_avg->average) * (running_avg->num_samples - 1) + new_value;
    uint32_t updated_avg = (total + running_avg->num_samples / 2) / running_avg->num_samples;

    // Clamping to UINT16_MAX for average is not needed here because:
    //  - Both the existing average and the new input are uint16_t values (max 65535).
    //  - The updated average is a weighted mean of these two values.
    //  - A weighted mean of two bounded values can never exceed the maximum of those values.
    //  - Integer overflow is prevented by computing the sum and mean using uint32_t.
    running_avg->average = (uint16_t)updated_avg;
}

void data_util_running_avg_reset(running_avg_t* running_avg)
{
    if (running_avg == NULL)
    {
        FPFW_ET_LOG(RunningAverageNullPointer);
        return;
    }
    running_avg->average = 0;
    running_avg->num_samples = 0;
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
