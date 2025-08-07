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
#include "data_utilities_i.h"
#include "die_2_die_exchange_i.h"
#include "telemetry_events_i.h"

#include <core_info.h>
#include <corebits.h>
#include <stdbool.h> // for false, true
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t, uint16_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/
#define MICROWATTS_PER_MILLIWATT (1000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

computed_metrics_2_min_t computed_metrics_2_mins = {0};
computed_metrics_24_hrs_t computed_metrics_24_hrs = {0};
computed_metrics_d2d_2_min_t computed_metrics_d2d_2mins = {0};
computed_metrics_oob_t computed_metrics_oob = {0};

bool core_is_active[NUMBER_OF_CORES_PER_DIE] = {0};

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core(void)
{
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.average,
                                                 computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples,
                                                 computed_metrics_d2d_2mins.max_soc_temp_dC.max);

    memset(&computed_metrics_d2d_2mins, 0, sizeof(computed_metrics_d2d_2mins));
}

void comp_metrics_init(void)
{
    data_util_mov_avg_u16_init(&computed_metrics_oob.max_soc_temp_mov_avg_dC,
                               computed_metrics_oob.max_soc_temp_samples_dC,
                               TEMPERATURE_MOVING_AVG_NUM_SAMPLES);

    data_util_mov_avg_u32_init(&computed_metrics_oob.soc_total_pwr_mov_avg_mW,
                               computed_metrics_oob.soc_total_pwr_samples_mW,
                               TEMPERATURE_MOVING_AVG_NUM_SAMPLES);

    data_util_mov_avg_u16_init(&computed_metrics_oob.max_dimm_temp_mov_avg_dC,
                               computed_metrics_oob.max_dimm_temp_samples_dC,
                               DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES);

    data_util_mov_avg_u32_init(&computed_metrics_oob.dimm_total_pwr_mov_avg_mW,
                               computed_metrics_oob.dimm_total_pwr_samples_mW,
                               DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES);

    data_util_mov_avg_u16_init(&computed_metrics_oob.max_vr_temp_mov_avg_dC,
                               computed_metrics_oob.max_vr_temp_samples_dC,
                               VR_TEMP_MOVING_AVG_NUM_SAMPLES);

    data_util_mov_avg_u16_init(&computed_metrics_oob.pstate_mov_avg, computed_metrics_oob.pstate_samples, PSTATE_MOVING_AVG_NUM_SAMPLES);

    for (uint8_t dimm_idx = 0; dimm_idx < NUMBER_OF_DIMMS_PER_DIE; dimm_idx++)
    {
        data_util_mov_avg_u16_init(&computed_metrics_oob.dimm_chan_temp_mov_avg[dimm_idx],
                                   computed_metrics_oob.dimm_chan_temp_samples[dimm_idx],
                                   DIMM_TEMP_MOVING_AVG_NUM_SAMPLES);

        data_util_mov_avg_u16_init(&computed_metrics_oob.dimm_chan_pwr_mov_avg[dimm_idx],
                                   computed_metrics_oob.dimm_chan_pwr_samples[dimm_idx],
                                   DIMM_PWR_MOVING_AVG_NUM_SAMPLES);
    }

    comp_metrics_init_active_cores();
}

void comp_metrics_init_active_cores(void)
{
    corebits_t* sys_cores_in_die = core_info_get_enable_cores_result();

    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        const corebits_t* enabled_cores = sys_cores_in_die;
        if (corebits_is_bit_set(enabled_cores, core))
        {
            core_is_active[core] = true;
        }
    }
}

void comp_metrics_for_single_core_current(uint8_t core_id, uint16_t latest_value_mA)
{
    if (core_is_active[core_id])
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].current_mA, latest_value_mA);
    }
}

void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV)
{
    if (core_is_active[core_id])
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].voltage_mV, latest_value_mV);
    }
}

void comp_metrics_for_single_core_temperature(uint8_t core_id, uint16_t latest_temperature_dC)
{
    if (core_is_active[core_id])
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].temperature_dC, latest_temperature_dC);
    }
}

void comp_metrics_for_single_core_power(uint8_t core_id, uint16_t latest_power_mW)
{
    if (core_is_active[core_id])
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].power_mW, latest_power_mW);
    }
}

void comp_metrics_for_single_core_single_pstate(uint8_t core_id,
                                                uint8_t current_pstate,
                                                uint64_t timestamp_diff_uS,
                                                uint8_t new_pstate,
                                                bool update_pstate_entry)
{
    if (core_is_active[core_id])
    {
        computed_metrics_2_mins.cores[core_id].pstate[current_pstate].residency_uS += timestamp_diff_uS;
        if (update_pstate_entry)
        {
            computed_metrics_2_mins.cores[core_id].pstate[new_pstate].entry_count += 1;
        }
    }
}

void comp_metrics_for_single_core_single_cstate(uint8_t core_id,
                                                uint8_t current_cstate,
                                                uint64_t timestamp_diff_uS,
                                                uint8_t new_cstate,
                                                bool update_cstate_entry)
{
    if (core_is_active[core_id])
    {
        computed_metrics_2_mins.cores[core_id].cstate[current_cstate].residency_uS += timestamp_diff_uS;
        // update entry count on compute metrics.
        if (update_cstate_entry)
        {
            computed_metrics_2_mins.cores[core_id].cstate[new_cstate].entry_count += 1;
        }
    }
}

void comp_metrics_for_single_core_power_per_pstate(uint8_t core_id, uint8_t current_pstate, uint16_t latest_power_mW, uint32_t duration_uS)
{
    if (core_is_active[core_id])
    {
        data_util_calc_mma_dur_u16(&computed_metrics_2_mins.cores[core_id].pstate[current_pstate].power_mW,
                                   latest_power_mW,
                                   duration_uS);
    }
}

void comp_metrics_for_single_core_single_throttle_overrun_count_update(uint8_t core_id, uint8_t throttle_index)
{
    if (core_is_active[core_id])
    {
        if (throttle_index == THROTTLE_SOURCE_ADAPTIVE_CLK || throttle_index == THROTTLE_SOURCE_CURRENT)
        {
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].overrun_count += 1;
        }
    }
}

void comp_metrics_for_single_core_single_throttle_update(uint8_t core_id, uint8_t throttle_index, uint64_t timestamp_diff_uS, bool throttle_start)
{
    if (core_is_active[core_id])
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
}

void comp_metrics_for_single_core_single_rack_throttle_update(uint8_t core_id,
                                                              uint8_t priority_id,
                                                              uint64_t timestamp_diff_uS,
                                                              bool rack_throttle_priority_start)
{
    if (core_is_active[core_id])
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
}

void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, int8_t throttle_index, uint32_t time_diff_uS, uint8_t latest_pstate)
{
    if (core_is_active[core_id])
    {
        /* For core throttling : max avg pstate calculation*/
        uint16_t temp_min_pstate = 0;
        uint16_t temp_avg_pstate = computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate;
        uint16_t temp_max_pstate = computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate;
        uint16_t temp_pstate = latest_pstate;
        uint32_t residency_mS = computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS;
        /* For core pstate- min, max avg calculation during throttle*/
        data_util_calc_mma_res(&temp_min_pstate, &temp_max_pstate, &temp_avg_pstate, &temp_pstate, time_diff_uS, residency_mS * 1000);
        // Update core throttle info
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate = temp_avg_pstate;
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate = temp_max_pstate;
    }
}

void comp_metrics_for_single_hnf_channel(uint8_t hnf_channel, uint16_t latest_temperature_dC)
{
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel], latest_temperature_dC);
}

void comp_metrics_for_soc_rails(uint16_t (*latest_rail_voltage_mV)[MAX_NUM_OF_VR_RAILS],
                                uint16_t (*latest_rail_current_mA)[MAX_NUM_OF_VR_RAILS])
{
    uint16_t num_rails = NUM_DIE0_VR_RAILS;
    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        num_rails = NUM_DIE1_VR_RAILS;
    }

    uint32_t total_power_mW = 0;

    for (uint16_t vr_index = 0; vr_index < num_rails; vr_index++)
    {
        // Update the rail voltage
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV,
                               (*latest_rail_voltage_mV)[vr_index]);

        // Update the rail current
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA,
                               (*latest_rail_current_mA)[vr_index]);

        // Calculate the power for each rail
        uint32_t rail_power_mW =
            ((*latest_rail_voltage_mV)[vr_index] * (*latest_rail_current_mA)[vr_index]) / MICROWATTS_PER_MILLIWATT;

        // Accumulate the total power
        total_power_mW += rail_power_mW;
    }

    // Update the total power moving average
    data_util_mov_avg_u32_add_sample(&computed_metrics_oob.soc_total_pwr_mov_avg_mW, total_power_mW);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_soc_pwr(computed_metrics_oob.soc_total_pwr_mov_avg_mW.total_sum,
                                                computed_metrics_oob.soc_total_pwr_mov_avg_mW.sample_count);
    }
}

void comp_metrics_for_soc_rail_temperature(uint16_t (*latest_rail_temperature_dC)[MAX_NUM_OF_VR_RAILS])
{
    uint16_t num_rails = NUM_DIE0_VR_RAILS;
    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        num_rails = NUM_DIE1_VR_RAILS;
    }

    uint16_t max_rail_temp_dC = 0;

    for (uint16_t vr_index = 0; vr_index < num_rails; vr_index++)
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC,
                               (*latest_rail_temperature_dC)[vr_index]);

        if ((*latest_rail_temperature_dC)[vr_index] > max_rail_temp_dC)
        {
            max_rail_temp_dC = (*latest_rail_temperature_dC)[vr_index];
        }
    }

    // Update the vr temp moving average
    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.max_vr_temp_mov_avg_dC, max_rail_temp_dC);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_max_vr_temp(computed_metrics_oob.max_vr_temp_mov_avg_dC.total_sum,
                                                    computed_metrics_oob.max_vr_temp_mov_avg_dC.sample_count);
    }
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

    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.max_soc_temp_mov_avg_dC, latest_max_soc_temp_dC);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_max_die_temp(computed_metrics_oob.max_soc_temp_mov_avg_dC.total_sum,
                                                     computed_metrics_oob.max_soc_temp_mov_avg_dC.sample_count);
    }
}

void comp_metrics_for_single_soc_dimm(uint8_t dimm_idx, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC, uint16_t latest_dimm_power_mW)
{
    // update in-band metrics
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s0_dC, latest_dimm_temp_s0_dC);
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s1_dC, latest_dimm_temp_s1_dC);
    data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].power_mW, latest_dimm_power_mW);

    // update out-of-band metrics
    uint16_t latest_dimm_temp_dC =
        (latest_dimm_temp_s0_dC > latest_dimm_temp_s1_dC) ? latest_dimm_temp_s0_dC : latest_dimm_temp_s1_dC;
    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.dimm_chan_temp_mov_avg[dimm_idx], latest_dimm_temp_dC);

    if (latest_dimm_temp_dC > computed_metrics_oob.dimm_chan_max_temp_dC[dimm_idx])
    {
        // NOTE: max dimm temp is pertinent since the last read of the data via out-of-band,
        //  the following value is used on the PRIMARY Die, and cleared when read from out of band.
        computed_metrics_oob.dimm_chan_max_temp_dC[dimm_idx] = latest_dimm_temp_dC;
    }
    uint16_t average_temp_dC = data_util_mov_avg_u16_get(&computed_metrics_oob.dimm_chan_temp_mov_avg[dimm_idx]);

    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.dimm_chan_pwr_mov_avg[dimm_idx], latest_dimm_power_mW);
    uint16_t average_pwr_mW = data_util_mov_avg_u16_get(&computed_metrics_oob.dimm_chan_pwr_mov_avg[dimm_idx]);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        // there is no light computational way for the secondary die to know when die 0 out of band has read the
        // dimm channel data, which it would need to reset the max temp value for the next interval.
        // instead, the latest dimm temp value is written to the exchange, and internally the exchange keeps track of
        // the max temp value. when read by out of band, it is cleared, resetting the max temp value for the next interval.
        // therefore, computed_metrics_oob.dimm_chan_max_temp_dC[] is unused on the secondary die.

        die_2_die_exch_oob_write_dimm_info(dimm_idx, average_pwr_mW, average_temp_dC, latest_dimm_temp_dC);
    }
}

void comp_metrics_for_max_dimm_temp(uint16_t latest_max_dimm_temp_dC)
{
    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.max_dimm_temp_mov_avg_dC, latest_max_dimm_temp_dC);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_max_dimm_temp(computed_metrics_oob.max_dimm_temp_mov_avg_dC.total_sum,
                                                      computed_metrics_oob.max_dimm_temp_mov_avg_dC.sample_count);
    }
}

void comp_metrics_for_total_dimm_pwr(uint32_t dimm_total_pwr_mW)
{
    data_util_mov_avg_u32_add_sample(&computed_metrics_oob.dimm_total_pwr_mov_avg_mW, dimm_total_pwr_mW);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_dimm_pwr(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.total_sum,
                                                 computed_metrics_oob.dimm_total_pwr_mov_avg_mW.sample_count);
    }
}

void comp_metrics_for_soc_avg_pstate(uint8_t (*pstate)[NUMBER_OF_CORES_PER_DIE])
{
    uint32_t total_pstate = 0;
    uint16_t num_active_cores = 0;

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        if ((core_is_active[core_id]) & ((*pstate)[core_id] != INVALID_PSTATE))
        {
            total_pstate += (*pstate)[core_id];
            num_active_cores++;
        }
    }

    if (num_active_cores > 0)
    {
        // round up
        uint16_t avg_pstate = (total_pstate + num_active_cores / 2) / num_active_cores;
        data_util_mov_avg_u16_add_sample(&computed_metrics_oob.pstate_mov_avg, avg_pstate);

        if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
        {
            die_2_die_exch_oob_write_window_avg_pstate(computed_metrics_oob.pstate_mov_avg.total_sum,
                                                       computed_metrics_oob.pstate_mov_avg.sample_count);
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

void comp_metrics_reset_2_mins_metrics()
{
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    if (die_2_die_exch_get_this_die_id() == PRIMARY_DIE_ID)
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