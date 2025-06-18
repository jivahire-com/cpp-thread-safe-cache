//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file compute_metrics.c
 * Update metrics for telemetry data
 */

/*------------- Includes -----------------*/
#include "compute_metrics_i.h"
#include "data_proc_tlm_cmpnt.h"
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "die_2_die_exchange_i.h"
#include "telemetry_events_i.h"

#include <stdbool.h> // for false, true
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t, uint16_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

computed_metrics_2_min_t computed_metrics_2_mins = {0};
computed_metrics_24_hrs_t computed_metrics_24_hrs = {0};
computed_metrics_d2d_2_min_t computed_metrics_d2d_2mins = {0};

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core(void)
{
    die_2_die_exchange_write_pwr_pkg_max_die_temp(computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.average,
                                                  computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples,
                                                  computed_metrics_d2d_2mins.max_soc_temp_dC.max);

    memset(&computed_metrics_d2d_2mins, 0, sizeof(computed_metrics_d2d_2mins));
}

void comp_metrics_for_sample_period(void)
{
    comp_metrics_for_tiles_for_sampling_period();
}

void comp_metrics_for_tiles_for_sampling_period(void)
{
    static uint64_t previous_tile_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;

    uint64_t time_diff_uS = data_util_calc_time_diff(&previous_tile_timestamp_uS, &time_stamp_uS, PWR_TLM_TILE_UPDATE);

    // Go over all tiles
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        // update the time counter
        tile[tile_id].time_counter_uS += time_diff_uS;

        // Update the tile Vcpu and Vsys MMA
        comp_metrics_for_single_tile_vcpu(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
        comp_metrics_for_single_tile_vsys(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
    }
}

void comp_metrics_for_single_core_current(uint8_t core_id, uint16_t latest_value_mA)
{
    /* For core current :min, max avg calculation*/
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].current_mA, latest_value_mA);
}

void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV)
{
    /* For core voltage :min, max avg calculation*/
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].voltage_mV, latest_value_mV);
}

void comp_metrics_for_single_core_temperature(uint8_t core_id, uint16_t latest_temperature_dC)
{
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].temperature_dC, latest_temperature_dC);
}

void comp_metrics_for_single_core_power(uint8_t core_id, uint16_t latest_power_mW)
{
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].power_mW, latest_power_mW);
}

void comp_metrics_for_single_tile_vcpu(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For tile vcpu :min, max avg calculation*/
    // Update the vcpu voltage  min, max average
    data_util_calc_mma_res(&tile[tile_id].vcpu.min_mV,
                           &tile[tile_id].vcpu.max_mV,
                           &tile[tile_id].vcpu.average_mV,
                           &tile[tile_id].vcpu.latest_value_mV,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_single_tile_vsys(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For tile vsys :min, max avg calculation*/
    // Update the vsys voltage min, max average
    data_util_calc_mma_res(&tile[tile_id].vsys.min_mV,
                           &tile[tile_id].vsys.max_mV,
                           &tile[tile_id].vsys.average_mV,
                           &tile[tile_id].vsys.latest_value_mV,
                           time_diff_uS,
                           residency_uS);
}

/* SOC VR rails update */
void comp_metrics_for_soc_rail_voltage(uint16_t (*latest_rail_voltage_mV)[MAX_NUM_OF_VR_RAILS])
{
    for (uint16_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        // Update the rail voltage
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV,
                               (*latest_rail_voltage_mV)[vr_index]);
    }
}

void comp_metrics_for_soc_rail_current(uint16_t (*latest_rail_current_mA)[MAX_NUM_OF_VR_RAILS])
{
    for (uint16_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        // Update the rail current
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA,
                               (*latest_rail_current_mA)[vr_index]);
    }
}

void comp_metrics_for_soc_rail_temperature(uint16_t (*latest_rail_temperature_dC)[MAX_NUM_OF_VR_RAILS])
{
    for (uint16_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC,
                               (*latest_rail_temperature_dC)[vr_index]);
    }
}

void comp_metrics_for_single_hnf_channel(uint8_t hnf_channel, uint16_t latest_temperature_dC)
{
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel], latest_temperature_dC);
}

void comp_metrics_for_soc_top_temp_sensor(uint16_t (*latest_soc_top_temp_dC)[NUMBER_OF_SOC_TEMP_SENSORS])
{
    for (uint16_t sensor_index = 0; sensor_index < NUMBER_OF_SOC_TEMP_SENSORS; sensor_index++)
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_index],
                               (*latest_soc_top_temp_dC)[sensor_index]);
    }
}

void comp_metrics_for_soc_max_temp(uint16_t latest_max_soc_temp_dC)
{
    data_util_calc_mma_u16(&computed_metrics_d2d_2mins.max_soc_temp_dC, latest_max_soc_temp_dC);
}

void comp_metrics_for_single_soc_dimm_temp(uint8_t dimm_id, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC)
{
    // Update  min, max average S0
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s0_dC, latest_dimm_temp_s0_dC);
    // Update each temperature data for S1
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s1_dC, latest_dimm_temp_s1_dC);
}

void comp_metrics_for_single_soc_dimm_power(uint8_t dimm_id, uint16_t latest_dimm_power_mW)
{
    // Update  min, max average dimm power
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_id].power_mW, latest_dimm_power_mW);
}

void comp_metrics_for_single_core_single_cstate(uint8_t core_id, uint8_t cstate, uint64_t timestamp_diff_uS, uint8_t update_cstate_entry)
{
    computed_metrics_2_mins.cores[core_id].cstate[cstate].residency_uS += timestamp_diff_uS;
    // update entry count on compute matrics.
    if (update_cstate_entry)
    {
        computed_metrics_2_mins.cores[core_id].cstate[cstate].entry_count += 1;
    }
}

void comp_metrics_for_single_core_single_pstate(uint8_t core_id, uint8_t pstate, uint64_t timestamp_diff_uS, uint8_t update_pstate_entry)
{
    computed_metrics_2_mins.cores[core_id].pstate[pstate].residency_uS += timestamp_diff_uS;
    if (update_pstate_entry)
    {
        computed_metrics_2_mins.cores[core_id].pstate[pstate].entry_count += 1;
    }
}

void comp_metrics_for_single_core_single_throttle_overrun_count_update(uint8_t core_id, uint8_t throttle_index)
{

    if (throttle_index == THROTTLE_SOURCE_ADAPTIVE_CLK || throttle_index == THROTTLE_SOURCE_CURRENT)
    {
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].overrun_count += 1;
    }
}

void comp_metrics_for_single_core_single_throttle_update(uint8_t core_id, uint8_t throttle_index, uint64_t timestamp_diff_uS, bool throttle_start)
{
    if (throttle_start)
    {
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].entry_count += 1;
    }
    else
    {
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS +=
            MICROSECONDS_TO_MILLISECONDS(timestamp_diff_uS);
    }
}

void comp_metrics_for_single_core_single_rack_throttle_update(uint8_t core_id,
                                                              uint8_t priority_id,
                                                              uint64_t timestamp_diff_uS,
                                                              bool rack_throttle_priority_start)
{
    /* Note :  rack_throttle_priority_start means a new rack prority started */
    if (rack_throttle_priority_start)
    {
        computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].entry_count += 1;
    }
    else
    {
        computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].residency_mS +=
            MICROSECONDS_TO_MILLISECONDS(timestamp_diff_uS);
    }
}

void comp_metrics_for_single_core_power_per_pstate(uint8_t core_id, uint8_t pstate_index, uint16_t latest_power_mW)
{
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW, latest_power_mW);
}

void comp_metrics_for_mpam(uint8_t core_id, uint16_t mpam_id, uint8_t pstate)
{

    FPFW_UNUSED(core_id);
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(pstate);
    // Update the core - mpam - pstate instantaneous power
    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319779
}

void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id,
                                                    int8_t throttle_index,
                                                    uint32_t time_diff_uS,
                                                    uint32_t residency_mS,
                                                    uint8_t latest_pstate)
{
    /* For core throttling : max avg pstate calculation*/
    uint16_t temp_min_pstate = 0;
    uint16_t temp_avg_pstate = computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate;
    uint16_t temp_max_pstate = computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate;
    uint16_t temp_pstate = latest_pstate;
    /* For core pstate- min, max avg calculation during throttle*/
    data_util_calc_mma_res(&temp_min_pstate, &temp_max_pstate, &temp_avg_pstate, &temp_pstate, time_diff_uS, residency_mS * 1000);
    // Update core throttle info
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate = temp_avg_pstate;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate = temp_max_pstate;
}

void comp_metrics_reset_2_mins_metrics()
{
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    if (die_2_die_exchange_get_this_die_id() == PRIMARY_DIE_ID)
    {
        // primary die can clear now as the data was just sent to the host
        // secondary die's will clear as soon as they write to the data exchange
        memset(&computed_metrics_d2d_2mins, 0, sizeof(computed_metrics_d2d_2mins));
    }
}

void comp_metrics_reset_24_hrs_metrics()
{
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));
}