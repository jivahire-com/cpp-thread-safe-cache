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

uint64_t data_util_calc_time_diff(uint64_t* previous_timestamp_uS, uint64_t* time_stamp_uS, pwr_tlm_update_t update_type)
{
    uint64_t temp_stamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    uint64_t time_diff_uS = 0;

    // Calculate the general residency for the core
    if (*previous_timestamp_uS != 0 && (temp_stamp_uS > *previous_timestamp_uS))
    {
        time_diff_uS = temp_stamp_uS - *previous_timestamp_uS;

        if (update_type == PWR_TLM_SOC_UPDATE)
        {
            soc_info.time_counter_uS += time_diff_uS;
        }
    }

    *previous_timestamp_uS = temp_stamp_uS;
    *time_stamp_uS = temp_stamp_uS;

    return time_diff_uS;
}

void data_util_calc_mma_res(uint16_t* mma_min,
                            uint16_t* mma_max,
                            uint16_t* mma_average,
                            uint16_t* mma_latest_value,
                            uint32_t time_diff_uS,
                            uint32_t residency_uS)
{
    // Check parameter bounds
    if ((time_diff_uS == 0) || (time_diff_uS == UINT32_MAX))
    {
        // FPFW_ET_LOG(DataUpdateMMAWrongInValidTimeStamp); //TODO: re-enable once  test systems support it
    }
    // Calculate the average
    if (*mma_latest_value != 0)
    {
        // Check for maximum
        if (*mma_latest_value > *mma_max)
        {
            *mma_max = *mma_latest_value;
        }
        // Check for minimum
        if ((*mma_latest_value < *mma_min || *mma_min == 0))
        {
            *mma_min = *mma_latest_value;
        }
        // Check to calculate average
        if ((residency_uS > time_diff_uS) && (residency_uS != 0) && (*mma_average != 0))
        {
            uint32_t weighted_previous_average = (uint32_t)*mma_average * (residency_uS - time_diff_uS);
            uint32_t weighted_latest_value = (uint32_t)*mma_latest_value * time_diff_uS;
            uint32_t average = (weighted_previous_average + weighted_latest_value) / residency_uS;
            // Ensure the calculated average does not exceed UINT16_MAX
            if (average > UINT16_MAX)
            {
                average = UINT16_MAX;
                FPFW_ET_LOG(DataUpdateMMAvgOverflow);
            }
            *mma_average = (uint16_t)average;
        }
        else
        {
            *mma_average = *mma_latest_value;
        }
    }
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

uint64_t data_utils_update_residency(uint64_t current_timestamp_uS, uint64_t* previous_timestamp_uS, uint64_t* residency_uS)
{
    // Calculate the general residency
    uint64_t time_diff_uS = 0;
    /* Invalid case :
       Example :
       On boot up , when previous timestamp is 0 and new timestamp may be in the order of 10000 us,
       that will add to the overall residency, which will not be a true reflection of residency calculation.
    */
    if (*previous_timestamp_uS != 0)
    {
        time_diff_uS = current_timestamp_uS - *previous_timestamp_uS;
        *residency_uS += time_diff_uS;
    }
    else
    {
        FPFW_ET_LOG(DataHelperResidencyUpdateInValidTimeStamp);
    }

    *previous_timestamp_uS = current_timestamp_uS;
    return time_diff_uS;
}
