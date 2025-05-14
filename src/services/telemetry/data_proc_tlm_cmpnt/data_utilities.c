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
/*-- Symbolic Constant Macros (defines) --*/

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