//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file compute_metrics.c
 * Update metrics for telemetry data
 */

/*------------- Includes -----------------*/
#include "aging_counters_i.h"
#include "compute_metrics_i.h"
#include "data_proc_tlm_cmpnt.h"
#include "data_utilities_i.h"
#include "die_2_die_exchange_i.h"
#include "telemetry_events_i.h"

#include <atu_api.h>
#include <core_info.h>
#include <corebits.h>
#include <kng_scp_tfa_shared.h>
#include <power_telemetry_common.h>
#include <pwr_tlm_core_exchange.h>
#include <stdbool.h> // for false, true
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t, uint16_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/
#define MICROWATTS_PER_MILLIWATT (1000)
#define MILLIAMP_TO_CENTIAMP(x)  ((x) / 10) // Convert milliamps to centiamps: 1 mA = 0.1 cA

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

#ifdef PWR_TLM_TEST_OVERRIDE

    #undef MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_BASE_ADDR
static uint8_t test_array_24hrs[ARSM_GET_REGION_OFFSET(D1_ARSM_MSCP_PWR_TLM_LARGE_DATA_BASE) + D1_ARSM_MSCP_PWR_TLM_LARGE_DATA_SIZE];
    #define MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_BASE_ADDR (test_array_24hrs)
#endif

computed_metrics_2_min_t computed_metrics_2_mins = {0};
computed_metrics_d2d_2_min_t computed_metrics_d2d_2mins = {0};
computed_metrics_oob_t computed_metrics_oob = {0};

computed_metrics_24_hrs_t* p_computed_metrics_24_hrs = (computed_metrics_24_hrs_t*)(uintptr_t)((
    MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_MSCP_PWR_TLM_LARGE_DATA_BASE)));

bool core_is_active[NUMBER_OF_CORES_PER_DIE] = {0};
bool in_band_publishing_active = false;

static bool metrics_24hrs_enabled = false;

/*------------- Functions ----------------*/

static_assert(sizeof(computed_metrics_24_hrs_t) <= D1_ARSM_MSCP_PWR_TLM_LARGE_DATA_SIZE,
              "computed_metrics_24_hrs_t size exceeds reserved memory region");

static_assert(NUMBER_OF_CORES_PER_DIE == NUM_AP_CORES_PER_DIE,
              "Mismatch between NUMBER_OF_CORES_PER_DIE and NUM_AP_CORES_PER_DIE");

void data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core(void)
{
    uint16_t max_soc_temp_avg_dC = data_util_running_avg_u16_get(&computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg);

    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(max_soc_temp_avg_dC,
                                                 computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples,
                                                 computed_metrics_d2d_2mins.max_soc_temp_dC.max);
    comp_metrics_reset_d2d_2_min_metrics();
}

void comp_metrics_init(bool is_single_die_system)
{
    metrics_24hrs_enabled = !is_single_die_system;

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

bool comp_metrics_core_and_inband_publishing_active(uint8_t core_id)
{
    return (core_is_active[core_id] && in_band_publishing_active);
}

static inline bool comp_metrics_24hrs_is_available(void)
{
    return metrics_24hrs_enabled;
}

void comp_metrics_for_single_core_current(uint8_t core_id, uint16_t latest_value_mA)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].current_mA, latest_value_mA);
    }
}

void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV, uint16_t latest_vcpu_voltage_mV)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].voltage_mV, latest_value_mV);
        // update ldo voltage for droop count.
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV, latest_vcpu_voltage_mV);
    }
}

void comp_metrics_for_single_core_temperature(uint8_t core_id, uint16_t latest_temperature_dC)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        data_util_calc_mma_u32(&computed_metrics_2_mins.cores[core_id].temperature_dC, latest_temperature_dC);
    }
}

void comp_metrics_for_single_core_power(uint8_t core_id, uint16_t latest_power_mW)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
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
    if (comp_metrics_core_and_inband_publishing_active(core_id))
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
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        computed_metrics_2_mins.cores[core_id].cstate[current_cstate].residency_uS += timestamp_diff_uS;
        // update entry count on compute metrics.
        if (update_cstate_entry)
        {
            computed_metrics_2_mins.cores[core_id].cstate[new_cstate].entry_count += 1;
        }
    }
}

void comp_metrics_for_single_core_power_per_pstate(uint8_t core_id, uint8_t current_pstate, uint16_t latest_power_mW)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].pstate[current_pstate].power_mW, latest_power_mW);
    }
}

void comp_metrics_for_single_core_histogram(uint8_t core_id, uint16_t core_voltage_mV, uint16_t core_temperature_dC)
{
    if (!comp_metrics_core_and_inband_publishing_active(core_id))
    {
        return;
    }

    if (!comp_metrics_24hrs_is_available())
    {
        return;
    }

    // Voltage bin upper limits in mV (20 bins) - descending order
    // Array size is NUMBER_OF_HS_VOLTAGE_SCALES - 1 because the highest bin (bin 0) has no upper limit
    // Bin ranges use exclusive lower bounds and inclusive upper bounds: (lower, upper]
    // Bin 0: (974, ∞] mV, Bin 1: (966, 974] mV, Bin 2: (958, 966] mV, ..., Bin 18: (830, 838] mV, Bin 19: [0, 830] mV
    static const uint16_t voltage_bin_upper_limits_mV[NUMBER_OF_HS_VOLTAGE_SCALES - 1] =
        {974, 966, 958, 950, 942, 934, 926, 918, 910, 902, 894, 886, 878, 870, 862, 854, 846, 838, 830};

    // Temperature bin upper limits in dC (15 bins) - descending order
    // Array size is NUMBER_OF_HS_TEMP_SCALES - 1 because the highest bin (bin 0) has no upper limit
    // Bin ranges use exclusive lower bounds and inclusive upper bounds: (lower, upper]
    // Bin 0: (965, ∞] dC, Bin 1: (935, 965] dC, Bin 2: (905, 935] dC, ..., Bin 12: (605, 635] dC, Bin 13: (575, 605] dC, Bin 14: [0, 575] dC
    static const uint16_t temp_bin_upper_limits_dC[NUMBER_OF_HS_TEMP_SCALES - 1] =
        {965, 935, 905, 875, 845, 815, 785, 755, 725, 695, 665, 635, 605, 575};

    // Find voltage bin - bin 0 is highest values, bin 19 is lowest values
    uint8_t voltage_bin = NUMBER_OF_HS_VOLTAGE_SCALES - 1; // Default to lowest bin
    for (uint8_t i = 0; i < (NUMBER_OF_HS_VOLTAGE_SCALES - 1); i++)
    {
        if (core_voltage_mV > voltage_bin_upper_limits_mV[i])
        {
            voltage_bin = i;
            break;
        }
    }

    // Find temperature bin - bin 0 is highest values, bin 14 is lowest values
    uint8_t temperature_bin = NUMBER_OF_HS_TEMP_SCALES - 1; // Default to lowest bin
    for (uint8_t i = 0; i < (NUMBER_OF_HS_TEMP_SCALES - 1); i++)
    {
        if (core_temperature_dC > temp_bin_upper_limits_dC[i])
        {
            temperature_bin = i;
            break;
        }
    }

    // Increment the appropriate histogram bin
    computed_metrics_24_hrs.cores[core_id].histogram.bin_count[voltage_bin][temperature_bin] += 1;
}

void comp_metrics_for_single_core_throttle_overrun(uint8_t core_id, uint8_t throttle_source)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        if (throttle_source == THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN || throttle_source == THROTTLE_SOURCE_CURRENT_OVERRUN)
        {
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count += 1;
        }
    }
}

void comp_metrics_for_single_core_single_throttle_source(uint8_t core_id, uint8_t throttle_source, uint64_t timestamp_diff_uS, bool throttle_start)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        if (throttle_start)
        {
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count += 1;
        }
        else
        {
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS += timestamp_diff_uS;
        }
    }
}

void comp_metrics_for_single_core_single_rack_priority(uint8_t core_id,
                                                       uint8_t current_priority,
                                                       uint64_t timestamp_diff_uS,
                                                       uint8_t new_priority,
                                                       bool rack_throttle_priority_start)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        if (rack_throttle_priority_start)
        {
            computed_metrics_2_mins.cores[core_id].rack_priorities[new_priority].entry_count += 1;
        }

        // timestamp_diff_uS will be zero if there was no previous priority throttling in progress
        computed_metrics_2_mins.cores[core_id].rack_priorities[current_priority].residency_uS += timestamp_diff_uS;
    }
}

void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, uint8_t throttle_source, uint8_t latest_pstate)
{
    if (comp_metrics_core_and_inband_publishing_active(core_id))
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate, latest_pstate);
    }
}

void comp_metrics_for_single_hnf_channel(uint8_t hnf_channel, uint16_t latest_temperature_dC)
{
    if (in_band_publishing_active)
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel], latest_temperature_dC);
    }
}

void comp_metrics_for_soc_rails(uint16_t (*latest_rail_voltage_mV)[MAX_NUM_OF_VR_RAILS],
                                uint32_t (*latest_rail_current_mA)[MAX_NUM_OF_VR_RAILS])
{
    uint8_t die_id = die_2_die_exch_get_this_die_id();
    uint16_t num_rails = (die_id == PRIMARY_DIE_ID) ? NUM_DIE0_VR_RAILS : NUM_DIE1_VR_RAILS;

    uint32_t total_power_mW = 0;

    for (uint16_t vr_index = 0; vr_index < num_rails; vr_index++)
    {
        if (in_band_publishing_active)
        {
            // Update the rail voltage
            data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV,
                                   (*latest_rail_voltage_mV)[vr_index]);

            // Update the rail current
            data_util_calc_mma_u32(&computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA,
                                   (*latest_rail_current_mA)[vr_index]);
        }

        // Calculate the power for each rail - use 64-bit arithmetic to prevent overflow
        uint64_t power_calculation = ((uint64_t)(*latest_rail_voltage_mV)[vr_index]) *
                                     ((uint64_t)(*latest_rail_current_mA)[vr_index]) / MICROWATTS_PER_MILLIWATT;

        uint32_t net_rail_power_mW = (uint32_t)power_calculation;

        uint32_t loadline_loss_power_mW = 0;
        if (die_id == PRIMARY_DIE_ID)
        {
            // die 0 vr_index maps to power_telemetry_rail_id_die0_t
            loadline_loss_power_mW =
                power_telemetry_loadline_loss_die0(vr_index, MILLIAMP_TO_CENTIAMP((*latest_rail_current_mA)[vr_index]));
        }
        else
        {
            // die 1 vr_index maps to power_telemetry_rail_id_die1_t
            loadline_loss_power_mW =
                power_telemetry_loadline_loss_die1(vr_index, MILLIAMP_TO_CENTIAMP((*latest_rail_current_mA)[vr_index]));
        }

        // Prevent underflow when subtracting load line loss
        uint32_t effective_rail_power_mW =
            (net_rail_power_mW > loadline_loss_power_mW) ? (net_rail_power_mW - loadline_loss_power_mW) : 0;

        // Accumulate the total power
        total_power_mW += effective_rail_power_mW;
    }

    // Update the total power moving average
    data_util_mov_avg_u32_add_sample(&computed_metrics_oob.soc_total_pwr_mov_avg_mW, total_power_mW);

    if (die_id != PRIMARY_DIE_ID)
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
        if (in_band_publishing_active)
        {
            data_util_calc_mma_u16(&computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC,
                                   (*latest_rail_temperature_dC)[vr_index]);
        }

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
    if (in_band_publishing_active)
    {
        for (uint16_t sensor_index = 0; sensor_index < NUMBER_OF_SOC_TEMP_SENSORS; sensor_index++)
        {
            data_util_calc_mma_u16(&computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_index],
                                   (*latest_soc_top_temp_dC)[sensor_index]);
        }
    }
}
void comp_metrics_for_soc_max_temp(uint16_t latest_max_soc_temp_dC)
{
    if (in_band_publishing_active)
    {
        data_util_calc_mma_u16(&computed_metrics_d2d_2mins.max_soc_temp_dC, latest_max_soc_temp_dC);
    }

    data_util_mov_avg_u16_add_sample(&computed_metrics_oob.max_soc_temp_mov_avg_dC, latest_max_soc_temp_dC);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_max_die_temp(computed_metrics_oob.max_soc_temp_mov_avg_dC.total_sum,
                                                     computed_metrics_oob.max_soc_temp_mov_avg_dC.sample_count);
    }
}

void comp_metrics_for_single_soc_dimm(uint8_t dimm_idx,
                                      uint16_t latest_dimm_temp_s0_dC,
                                      uint16_t latest_dimm_temp_s1_dC,
                                      uint16_t latest_dimm_power_mW,
                                      uint16_t entry_count,
                                      uint8_t throttle_duration_mS,
                                      uint8_t throttle_source)
{
    // update in-band metrics
    if (in_band_publishing_active)
    {
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s0_dC, latest_dimm_temp_s0_dC);
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s1_dC, latest_dimm_temp_s1_dC);
        data_util_calc_mma_u16(&computed_metrics_2_mins.soc.dimm[dimm_idx].power_mW, latest_dimm_power_mW);

        if (computed_metrics_2_mins.soc.dimm[dimm_idx].entry_counts <= UINT32_MAX - entry_count)
        {
            computed_metrics_2_mins.soc.dimm[dimm_idx].entry_counts += entry_count;
        }
        if (computed_metrics_2_mins.soc.dimm[dimm_idx].duration_mS <= UINT32_MAX - throttle_duration_mS)
        {
            computed_metrics_2_mins.soc.dimm[dimm_idx].duration_mS += throttle_duration_mS;
        }
        computed_metrics_2_mins.soc.dimm[dimm_idx].throttle_source = throttle_source;
    }

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

void comp_metrics_for_cores_droop_counts(void)
{
    if (in_band_publishing_active)
    {
        uint64_t droop_counts[NUMBER_OF_CORES_PER_DIE];

        pwr_tlm_core_exch_mcp_read_droop_counts(&droop_counts);

        for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            if (comp_metrics_core_and_inband_publishing_active(core_id))
            {
                computed_metrics_2_mins.cores[core_id].droop_count = droop_counts[core_id];
            }
        }
    }
}

void comp_metrics_for_single_core_aging_counters(uint8_t core_id,
                                                 uint16_t latest_voltage_mV,
                                                 uint16_t latest_max_value_dC,
                                                 uint64_t this_pwr_pkg_timestamp_uS,
                                                 uint32_t latest_aged_counter,
                                                 uint32_t latest_unaged_counter,
                                                 uint8_t counter_id)
{
    if (!comp_metrics_24hrs_is_available())
    {
        return;
    }

    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].counter_id = counter_id;
    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter = latest_aged_counter;
    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter = latest_unaged_counter;
    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS = this_pwr_pkg_timestamp_uS;
    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].temperature_dC = latest_max_value_dC;
    computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].voltage_mV = latest_voltage_mV;
}

void comp_metrics_for_soc_package_cstate(uint8_t pkg_cstate, uint32_t duration_mS)
{
    if (!comp_metrics_24hrs_is_available())
    {
        return;
    }

    // Accumulate PC3/PC4 residency durations in 24-hour metrics
    if (pkg_cstate == ALL_CORES_IN_C3_state)
    {
        computed_metrics_24_hrs.soc.pc3_residency_mS += duration_mS;
    }
    else if (pkg_cstate == ALL_CORES_IN_C4_state)
    {
        computed_metrics_24_hrs.soc.pc4_residency_mS += duration_mS;
    }
}

uint32_t comp_metrics_get_memory_avg_pwr_mW(void)
{
    return data_util_running_avg_u32_get(&computed_metrics_2_mins.soc.memory_avg_pwr_mW);
}

void comp_metrics_for_single_d2dss_interface_all_links(uint8_t d2dss_id,
                                                       uint64_t (*tx_res_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                       uint64_t (*rx_res_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                       uint64_t (*bw_tx_flit_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                       uint64_t (*bw_rx_flit_counter_diff)[NUMBER_OF_D2D_LINKS_STATE])
{
    for (uint8_t link_state = 0; link_state < NUMBER_OF_D2D_LINKS_STATE; ++link_state)
    {
        computed_metrics_2_mins.d2dss[d2dss_id].d2d_link[link_state].tx_residency_count +=
            (*tx_res_counter_diff)[link_state];
        computed_metrics_2_mins.d2dss[d2dss_id].d2d_link[link_state].rx_residency_count +=
            (*rx_res_counter_diff)[link_state];
        computed_metrics_2_mins.d2dss[d2dss_id].d2d_link[link_state].bw_tx_flit_count +=
            (*bw_tx_flit_counter_diff)[link_state];
        computed_metrics_2_mins.d2dss[d2dss_id].d2d_link[link_state].bw_rx_flit_count +=
            (*bw_rx_flit_counter_diff)[link_state];
    }
}

void comp_metrics_for_mpam_data(mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS])
{
    if (in_band_publishing_active)
    {
        if (mpam_data_array == NULL)
        {
            FPFW_ET_LOG(CompMetricsMpamPwrNullPointer);
            return;
        }

        for (uint8_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
        {
            if ((*mpam_data_array)[mpam_id].active)
            {
                data_util_calc_mma_u32(&computed_metrics_2_mins.mpam[mpam_id].core_power,
                                       (*mpam_data_array)[mpam_id].latest_total_pwr_mW);

                data_util_calc_mma_u16(&computed_metrics_2_mins.mpam[mpam_id].active_pstate,
                                       (*mpam_data_array)[mpam_id].latest_pstate);
            }
        }
    }
}

void comp_metrics_for_mpam_throttling(uint8_t mpam_id, uint32_t residency_uS, uint8_t nominal_pstate)
{
    if (in_band_publishing_active)
    {
        if (mpam_id >= NUMBER_OF_MPAMS)
        {
            FPFW_ET_LOG(CompMetricsMpamIdOutOfRange, mpam_id);
            return;
        }

        computed_metrics_2_mins.mpam[mpam_id].residency_uS += residency_uS;
        computed_metrics_2_mins.mpam[mpam_id].nominal_pstate = nominal_pstate;
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
    if (in_band_publishing_active)
    {
        data_util_running_avg_u32_add_sample(&computed_metrics_2_mins.soc.memory_avg_pwr_mW, dimm_total_pwr_mW);
    }

    data_util_mov_avg_u32_add_sample(&computed_metrics_oob.dimm_total_pwr_mov_avg_mW, dimm_total_pwr_mW);

    if (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID)
    {
        die_2_die_exch_oob_write_window_dimm_pwr(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.total_sum,
                                                 computed_metrics_oob.dimm_total_pwr_mov_avg_mW.sample_count);

        die_2_die_exch_ib_write_total_memory_power_mW(
            data_util_running_avg_u32_get(&computed_metrics_2_mins.soc.memory_avg_pwr_mW));
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

void comp_metrics_for_per_die_mesh_tlm(uint32_t m1_entry_count,
                                       uint32_t m2_entry_count,
                                       uint32_t m0_residency_count,
                                       uint32_t m1_residency_count,
                                       uint32_t m2_residency_count,
                                       uint32_t delivered_perf_count)
{
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_entry_count += (uint64_t)m1_entry_count;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_entry_count += (uint64_t)m2_entry_count;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m0_residency_count += (uint64_t)m0_residency_count;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_residency_count += (uint64_t)m1_residency_count;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_residency_count += (uint64_t)m2_residency_count;
    computed_metrics_2_mins.mesh.die_mesh_pwr.delivered_perf_count += (uint64_t)delivered_perf_count;
}

void comp_metrics_reset_local_2_min_metrics()
{
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));
}

void comp_metrics_reset_d2d_2_min_metrics()
{
    memset(&computed_metrics_d2d_2mins, 0, sizeof(computed_metrics_d2d_2mins));
}

void comp_metrics_reset_24_hrs_metrics()
{
    if (!comp_metrics_24hrs_is_available())
    {
        return;
    }

    memset(p_computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs_t));
}