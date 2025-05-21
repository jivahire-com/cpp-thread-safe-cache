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

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_prepare_data_for_inst_sample(void)
{
}

void data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg(void)
{
}

void comp_metrics_for_sample_period(void)
{
    comp_metrics_for_cores_for_sampling_period();
    comp_metrics_for_tiles_for_sampling_period();
    comp_metrics_for_soc_for_sampling_period();
}

void comp_metrics_for_cores_for_sampling_period(void)
{
    static uint64_t previous_core_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;
    // calculate the timestamp and time difference
    uint64_t time_diff_uS = data_util_calc_time_diff(&previous_core_timestamp_uS, &time_stamp_uS, PWR_TLM_CORE_UPDATE);

    // Go over all cores
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        /* Calculate  the general residency for the core*/
        core[core_id].time_counter_uS += time_diff_uS;

        /* Do not do any update for this core if the current packet timestamp is zero: disable core */
        if (core[core_id].current_pkt_timestamp != 0)
        {
            /* update pstate residency and power */
            comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

            // Check to update throttling and priorities
            comp_metrics_for_single_core_throttling(core_id, time_stamp_uS);

            /* update Core current*/
            comp_metrics_for_single_core_current(core_id, time_diff_uS, core[core_id].time_counter_uS);
        }

        /* Note that even if a core is disabled, voltage and temperature sensors
            are still running on those disabled cores */

        // Check to update Core temperature
        comp_metrics_for_single_core_temperature(core_id, time_diff_uS, core[core_id].time_counter_uS);

        // update residency to generate volt/temp histogram
        comp_metrics_for_single_core_histogram(core_id);
    }
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
        if (tile[tile_id].active_sample_max_temperature_dC > tile[tile_id].max_tile_temperature_dC)
        {
            // Update the new Max tile temperature
            tile[tile_id].max_tile_temperature_dC = tile[tile_id].active_sample_max_temperature_dC;
            tile[tile_id].max_tile_id = tile[tile_id].active_sample_max_id;
        }

        // Update the tile Vcpu and Vsys MMA
        comp_metrics_for_single_tile_vcpu(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
        comp_metrics_for_single_tile_vsys(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
    }
}

void comp_metrics_for_soc_for_sampling_period(void)
{
    static uint64_t previous_soc_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;

    // calculate the timestamp and time difference
    uint64_t time_diff_uS = data_util_calc_time_diff(&previous_soc_timestamp_uS, &time_stamp_uS, PWR_TLM_SOC_UPDATE);

    // Update the Rail information
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        comp_metrics_for_soc_rail_voltage(vr_index, time_diff_uS, soc_info.time_counter_uS);
        comp_metrics_for_soc_rail_current(vr_index, time_diff_uS, soc_info.time_counter_uS);
        comp_metrics_for_soc_rail_temperature(vr_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Update the HNF Temperature information'
    for (uint8_t hnf_index = 0; hnf_index < NUMBER_OF_HNF_CHANNELS_PER_DIE; hnf_index++)
    {
        comp_metrics_for_single_hnf_channel(hnf_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Update the PVT Sensor information'
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        comp_metrics_for_single_soc_temp_sensor(pvt_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Check current cstate for all cores
    // only if ALL cores are in pc3, increment residency
    // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023433
}

void comp_metrics_for_single_core_pstate(uint8_t core_id, uint64_t time_stamp_uS)
{
    // Check if the current core has a pstate update
    // Update the PState Residency
    uint8_t pstate_index;
    // Check first if we are throttling
    if (core[core_id].throttling_status == NO_THROTTLE)
    {
        /* get pstate from pstate packet/pstate fifo */
        pstate_index = core[core_id].pstate_from_pstate_pkt;
    }
    else
    {
        /* in case of throttle pstate is not reported by pstate packet, get from current telemetry packet*/
        pstate_index = core[core_id].pstate_from_current_pkt;
    }

    if (core[core_id].flags.id_change_bit == 0 && core[core_id].flags.pstate_change == 0)
    {
        /*Update the current pstate residencies*/
        comp_metrics_for_single_core_single_pstate(core_id, pstate_index, time_stamp_uS);
        // Check if there are power samples for this core
        if (core[core_id].num_pwr_samples > 0)
        {
            /* average of all the sample reported so far*/
            core[core_id].average_pwr_mW = (core_pwr_samples_accumulation_mW[core_id] / core[core_id].num_pwr_samples);
        }
        /* core[core_id].average_power_samples_value has running average of the power samples, update latest_value_mW */
        core[core_id].pstate[pstate_index].latest_value_mW = core[core_id].average_pwr_mW;
        if (core[core_id].average_pwr_mW)
        {
            comp_metrics_for_single_core_pstate_power(core_id, pstate_index);
        }
        // Update the core - mpam - pstate instantaneous power
        comp_metrics_for_mpam(core_id, core[core_id].active_sample_mpam_id, pstate_index);
    }
    else
    {
        // Clear the pstate change indicator for this core
        core[core_id].flags.id_change_bit = 0;
        core[core_id].flags.pstate_change = 0;
        pstate_accum_uS[core_id][pstate_index] = 0;
    }
}

void comp_metrics_for_single_core_current(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For core current :min, max avg calculation*/
    // Update the core current min, max average.
    data_util_calc_mma_res(&core[core_id].current.min_mA,
                           &core[core_id].current.max_mA,
                           &core[core_id].current.average_mA,
                           &core[core_id].current.latest_value_mA,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV)
{
    /* For core voltage :min, max avg calculation*/
    data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].voltage_mV, latest_value_mV);
}

void comp_metrics_for_single_core_temperature(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For core temperature :min, max avg calculation*/
    data_util_calc_mma_res(&core[core_id].temperature.min_dC,
                           &core[core_id].temperature.max_dC,
                           &core[core_id].temperature.average_dC,
                           &core[core_id].temperature.latest_value_dC,
                           time_diff_uS,
                           residency_uS);
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
void comp_metrics_for_soc_rail_voltage(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail voltage :min, max avg calculation*/
    // Update the rail voltage min, max average
    data_util_calc_mma_res(&soc_info.rail[vr_index].voltage.min_mV,
                           &soc_info.rail[vr_index].voltage.max_mV,
                           &soc_info.rail[vr_index].voltage.average_mV,
                           &soc_info.rail[vr_index].voltage.latest_value_mV,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_soc_rail_current(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail current:min, max avg calculation*/
    // Update the rail current min, max average
    data_util_calc_mma_res(&soc_info.rail[vr_index].current.min_mA,
                           &soc_info.rail[vr_index].current.max_mA,
                           &soc_info.rail[vr_index].current.average_mA,
                           &soc_info.rail[vr_index].current.latest_value_mA,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_soc_rail_temperature(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail temperature:min, max avg calculation*/
    // Update the vr rails temperature min, max average
    data_util_calc_mma_res(&soc_info.rail[vr_index].temperature.min_dC,
                           &soc_info.rail[vr_index].temperature.max_dC,
                           &soc_info.rail[vr_index].temperature.average_dC,
                           &soc_info.rail[vr_index].temperature.latest_value_dC,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_single_hnf_channel(uint8_t hnf_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc hnf temperature :min, max avg calculation*/
    // Update the hnf temp min, max average
    data_util_calc_mma_res(&soc_info.hnf[hnf_index].min_dC,
                           &soc_info.hnf[hnf_index].max_dC,
                           &soc_info.hnf[hnf_index].average_dC,
                           &soc_info.hnf[hnf_index].latest_value_dC,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_single_soc_temp_sensor(uint8_t pvt_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc pvt temperature :min, max avg calculation*/
    // Update the soc pvt temp min, max average
    // Store new values
    data_util_calc_mma_res(&soc_info.sensor_temp[pvt_index].min_dC,
                           &soc_info.sensor_temp[pvt_index].max_dC,
                           &soc_info.sensor_temp[pvt_index].average_dC,
                           &soc_info.sensor_temp[pvt_index].latest_value_dC,
                           time_diff_uS,
                           residency_uS);
}

void comp_metrics_for_single_soc_dimm_temp(uint8_t dimm_id, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC)
{
    // Update  min, max average S0
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s0_dC, latest_dimm_temp_s0_dC);
    // Update each temperature data for S1
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s1_dC, latest_dimm_temp_s1_dC);
}

fpfw_status_t comp_metrics_for_single_core_single_pstate(uint8_t core_id, uint8_t pstate, uint64_t timestamp_uS)
{
    // Update the residency of the previous pstate
    if (core[core_id].pstate_timestamp_uS != 0 && core[core_id].pstate_timestamp_uS < timestamp_uS)
    {
        //  obtain the time stamp difference @ uS
        uint64_t timestamp_diff_uS = timestamp_uS - core[core_id].pstate_timestamp_uS;
        core[core_id].pstate[pstate].residency_uS += timestamp_diff_uS;
        pstate_accum_uS[core_id][pstate] += timestamp_diff_uS; // for MPAM only .
    }

    core[core_id].pstate_timestamp_uS = timestamp_uS;
    return FPFW_STATUS_SUCCESS;
}

void comp_metrics_for_single_core_pstate_power(uint8_t core_id, uint8_t pstate_index)
{

    pwr_pstate_t* pstate = &core[core_id].pstate[pstate_index];
    uint16_t latest_value_mW = pstate->latest_value_mW;

    // Update min, max, and average power values
    if (latest_value_mW < pstate->min_power_mW || pstate->min_power_mW == 0)
    {
        pstate->min_power_mW = latest_value_mW;
    }
    if (latest_value_mW > pstate->max_power_mW)
    {
        pstate->max_power_mW = latest_value_mW;
    }

    // Calculate the weighted average power value
    if (core[core_id].num_pwr_samples <= 1)
    {
        pstate->avg_power_mW = latest_value_mW;
    }
    else
    {
        uint32_t weighted_previous_average = (uint32_t)pstate->avg_power_mW * (core[core_id].num_pwr_samples - 1);
        uint32_t weighted_latest_value = latest_value_mW;
        uint32_t new_avg_power_mW = (weighted_previous_average + weighted_latest_value) / core[core_id].num_pwr_samples;

        // Ensure the calculated average does not exceed UINT16_MAX
        if (new_avg_power_mW > UINT16_MAX)
        {
            new_avg_power_mW = UINT16_MAX;
            FPFW_ET_LOG(PstatePWRUpdateMMAvgOverflow);
        }

        // Check if the calculated average goes lower but not lower than the new value
        if (new_avg_power_mW < latest_value_mW)
        {
            pstate->avg_power_mW = latest_value_mW;
        }
        else
        {
            pstate->avg_power_mW = new_avg_power_mW;
        }
    }
}

void comp_metrics_for_mpam(uint8_t core_id, uint16_t mpam_id, uint8_t pstate)
{

    FPFW_UNUSED(core_id);
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(pstate);
    // Update the core - mpam - pstate instantaneous power
    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319779
}

void comp_metrics_for_single_core_throttling(uint8_t core_id, uint64_t time_stamp_uS)
{
    int8_t i = 0;
    // End all active throttling and update residency
    for (i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        if (core[core_id].core_throttling_tracker[i])
        {
            if (core[core_id].throttle_previous_timestamp_uS[i] != 0 &&
                time_stamp_uS > core[core_id].throttle_previous_timestamp_uS[i])
            {
                // Get the Throttling time stamp now and subtract from previous
                uint64_t time_diff_uS = time_stamp_uS - core[core_id].throttle_previous_timestamp_uS[i];
                // This is the per core and per type throttling residency in uS
                core[core_id].throttle_info[i].residency_mS += MICROSECONDS_TO_MILLISECONDS(time_diff_uS);
                // Use per throttle type accumualated residency for max and avg calculation.
                /* For core pstate- min, max avg calculation during throttle*/
                comp_metrics_for_single_core_throttling_pstate(core_id,
                                                               i,
                                                               time_diff_uS,
                                                               core[core_id].throttle_info[i].residency_mS);
            }
        }
        core[core_id].throttle_previous_timestamp_uS[i] = time_stamp_uS;
    }
}

void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, int8_t throttle_index, uint32_t time_diff_uS, uint32_t residency_mS)
{
    /* For core throttling : max avg pstate calculation*/
    uint16_t temp_min_pstate = 0;
    uint16_t temp_avg_pstate = core[core_id].throttle_info[throttle_index].avg_pstate;
    uint16_t temp_max_pstate = core[core_id].throttle_info[throttle_index].max_pstate;
    uint16_t temp_pstate = core[core_id].pstate_from_current_pkt;
    /* For core pstate- min, max avg calculation during throttle*/
    data_util_calc_mma_res(&temp_min_pstate, &temp_max_pstate, &temp_avg_pstate, &temp_pstate, time_diff_uS, residency_mS * 1000);
    // Update core throttle info
    core[core_id].throttle_info[throttle_index].avg_pstate = temp_avg_pstate;
    core[core_id].throttle_info[throttle_index].max_pstate = temp_max_pstate;
}

void comp_metrics_for_single_core_histogram(uint8_t core_id)
{
    FPFW_UNUSED(core_id);
    // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319991
}

void comp_metrics_reset_2_mins_metrics()
{
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));
}

void comp_metrics_reset_24_hrs_metrics()
{
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));
}