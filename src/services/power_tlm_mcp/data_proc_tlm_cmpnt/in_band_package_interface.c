//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file package_interface.c
 * API's to retrieve data for in band packaging and out of band reporting
 */

/*------------- Includes -----------------*/

#include "compute_metrics_i.h"
#include "data_proc_tlm_cmpnt.h"
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "die_2_die_exchange_i.h"
#include "in_band_package_interface_i.h"
#include "telemetry_events_i.h"

#include <dvfs.h>
#include <stdbool.h> // for false, true
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint8_t, uint16_t
#include <string.h>  // for memset

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void package_inf_init(void)
{
    // need a call into this file, otherwise the linker will remove this file from the build
}

void data_proc_tlm_cmpnt_get_pwr_core_pstate_data(uint16_t core_id, pwr_core_element_pstate_t (*pstate_array)[NUMBER_OF_PSTATES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || pstate_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_PSTATE);
    }
    else
    {
        for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
        {
            (*pstate_array)[pstate_index].pstate_id = pstate_index;

            (*pstate_array)[pstate_index].avg_power_mW = data_util_running_avg_u16_get(
                &computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.running_avg);

            (*pstate_array)[pstate_index].min_power_mW =
                computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.min;

            (*pstate_array)[pstate_index].max_power_mW =
                computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.max;

            (*pstate_array)[pstate_index].frequency_Mhz = dvfs_get_freq_from_plimit(pstate_index);

            (*pstate_array)[pstate_index].residency_mS =
                ROUND_USEC_TO_MSEC(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].residency_uS);

            (*pstate_array)[pstate_index].entry_count =
                computed_metrics_2_mins.cores[core_id].pstate[pstate_index].entry_count;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_cstate_data(uint16_t core_id, pwr_core_element_cstate_t (*cstate_array)[NUMBER_OF_CSTATES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || cstate_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_CSTATE);
    }
    else
    {
        for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
        {
            (*cstate_array)[cstate_index].cstate_id = cstate_index;
            (*cstate_array)[cstate_index].residency_mS =
                ROUND_USEC_TO_MSEC(computed_metrics_2_mins.cores[core_id].cstate[cstate_index].residency_uS);
            (*cstate_array)[cstate_index].entry_count =
                computed_metrics_2_mins.cores[core_id].cstate[cstate_index].entry_count;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_throttle_data(uint16_t core_id,
                                                    pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_SOURCES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || throttle_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_THROTTLE);
    }
    else
    {
        for (uint16_t throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
        {
            (*throttle_array)[throttle_source].avg_pstate = data_util_running_avg_u16_get(
                &computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg);

            (*throttle_array)[throttle_source].entry_count =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count;

            (*throttle_array)[throttle_source].max_pstate =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.max;

            (*throttle_array)[throttle_source].residency_mS =
                ROUND_USEC_TO_MSEC(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS);

            (*throttle_array)[throttle_source].type_id = throttle_source;

            (*throttle_array)[throttle_source].overrun_count =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id,
                                                         pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_THROTTLE_PRIORITIES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || rack_priority_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES);
    }
    else
    {
        for (uint16_t rack_throttle_priority = 0; rack_throttle_priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES;
             rack_throttle_priority++)
        {
            (*rack_priority_array)[rack_throttle_priority].priority_id = rack_throttle_priority;

            (*rack_priority_array)[rack_throttle_priority].entry_count =
                computed_metrics_2_mins.cores[core_id].rack_priorities[rack_throttle_priority].entry_count;

            (*rack_priority_array)[rack_throttle_priority].residency_mS = ROUND_USEC_TO_MSEC(
                computed_metrics_2_mins.cores[core_id].rack_priorities[rack_throttle_priority].residency_uS);
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_voltage_data(uint16_t core_id, p_pwr_core_element_voltage_t voltage_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || voltage_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE);
    }
    else
    {
        voltage_data->average_mV =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg);
        voltage_data->max_mV = computed_metrics_2_mins.cores[core_id].voltage_mV.max;
        voltage_data->min_mV = computed_metrics_2_mins.cores[core_id].voltage_mV.min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_current_data(uint16_t core_id, p_pwr_core_element_current_t current_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || current_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_CURRENT);
    }
    else
    {
        current_data->average_mA =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].current_mA.running_avg);
        current_data->max_mA = computed_metrics_2_mins.cores[core_id].current_mA.max;
        current_data->min_mA = computed_metrics_2_mins.cores[core_id].current_mA.min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_temperature_data(uint16_t core_id, p_pwr_core_element_temperature_t temperature_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || temperature_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE);
    }
    else
    {
        temperature_data->average_dC =
            data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg);
        temperature_data->max_dC = computed_metrics_2_mins.cores[core_id].temperature_dC.max;
        temperature_data->min_dC = computed_metrics_2_mins.cores[core_id].temperature_dC.min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_power_data(uint16_t core_id, p_pwr_core_element_power_t power_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || power_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_POWER);
    }
    else
    {
        // Going with assignment over memcpy for clarity due to different structs
        power_data->min_mW = computed_metrics_2_mins.cores[core_id].power_mW.min;
        power_data->max_mW = computed_metrics_2_mins.cores[core_id].power_mW.max;
        power_data->average_mW =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].power_mW.running_avg);
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(
    uint16_t core_id,
    pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES])
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(histogram_array);
}

void data_proc_tlm_cmpnt_get_pwr_core_aging_data(uint16_t core_id, pwr_core_element_aging_t (*aging_data)[NUMBER_OF_AGING_COUNTER_PAIRS])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || aging_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_AGING);
    }
    else
    {
        for (uint16_t counter_id = 0; counter_id < NUMBER_OF_AGING_COUNTER_PAIRS; counter_id++)
        {
            (*aging_data)[counter_id].counter_id = counter_id;

            (*aging_data)[counter_id].aged_counter =
                computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter;
            (*aging_data)[counter_id].unaged_counter =
                computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter;
            (*aging_data)[counter_id].temperature_dC =
                computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].temperature_dC;
            (*aging_data)[counter_id].voltage_mV =
                computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].voltage_mV;
            (*aging_data)[counter_id].timestamp_uS =
                computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_droop_count_data(uint16_t core_id, p_pwr_core_element_droop_count_t droop_count)
{
    if (core_id >= NUMBER_OF_CORES_PER_DIE || droop_count == NULL)
    {
        FPFW_ET_LOG(DataPackageDroopCountRrecordError, POWER_TELEMETRY_ELEMENT_CORE_DROOPS);
    }
    else
    {
        droop_count->droop_count = computed_metrics_2_mins.cores[core_id].droop_count;

        droop_count->ldo_output_voltage.average_mV =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg);
        droop_count->ldo_output_voltage.min_mV = computed_metrics_2_mins.cores[core_id].voltage_mV.min;
        droop_count->ldo_output_voltage.max_mV = computed_metrics_2_mins.cores[core_id].voltage_mV.max;

        droop_count->vcpu_input_voltage.average_mV =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg);
        droop_count->vcpu_input_voltage.max_mV = computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.max;
        droop_count->vcpu_input_voltage.min_mV = computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(p_pwr_soc_element_pkg_monitor_t soc_pkg_mon_data)
{
    FPFW_UNUSED(soc_pkg_mon_data);
}

void data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(uint16_t rail_id, p_pwr_soc_element_vr_rail_t rail_data)
{
    // parameter check: rail_id, check if correct
    if (rail_id >= MAX_NUM_OF_VR_RAILS || rail_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS);
    }
    else
    {
        rail_data->current.average_mA =
            data_util_running_avg_u32_get(&computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.running_avg);
        rail_data->current.max_mA = computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.max;
        rail_data->current.min_mA = computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.min;

        rail_data->temperature.average_dC =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.running_avg);
        rail_data->temperature.max_dC = computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.max;
        rail_data->temperature.min_dC = computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.min;

        rail_data->voltage.average_mV =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.running_avg);
        rail_data->voltage.max_mV = computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.max;
        rail_data->voltage.min_mV = computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(uint16_t hnf_channel, p_pwr_soc_element_hnf_t hnf_data)
{
    // parameter check: hnf_channel, check if correct
    if (hnf_channel >= NUMBER_OF_HNF_CHANNELS_PER_DIE || hnf_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP);
    }
    else
    {
        hnf_data->average_dC =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg);
        hnf_data->max_dC = computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max;
        hnf_data->min_dC = computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(uint16_t dimm_idx, p_pwr_soc_element_dimm_temp_t dimm_data)
{
    // parameter check: dimm_idx, check if correct
    if (dimm_idx >= NUMBER_OF_DIMMS_PER_DIE || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE);
    }
    else
    {
        // DIMM temperature s0
        dimm_data->s0.max_dC = computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s0_dC.max;
        dimm_data->s0.min_dC = computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s0_dC.min;
        dimm_data->s0.average_dC =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s0_dC.running_avg);
        // DIMM temperature s1
        dimm_data->s1.max_dC = computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s1_dC.max;
        dimm_data->s1.min_dC = computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s1_dC.min;
        dimm_data->s1.average_dC =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[dimm_idx].temperature_s1_dC.running_avg);
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(uint16_t dimm_idx, p_pwr_soc_element_dimm_power_t dimm_data)
{
    // parameter check: dimm_idx, check if correct
    if (dimm_idx >= NUMBER_OF_DIMMS_PER_DIE || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER);
    }
    else
    {
        dimm_data->power_mW.min_mW = computed_metrics_2_mins.soc.dimm[dimm_idx].power_mW.min;
        dimm_data->power_mW.max_mW = computed_metrics_2_mins.soc.dimm[dimm_idx].power_mW.max;
        dimm_data->power_mW.average_mW =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[dimm_idx].power_mW.running_avg);
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(uint16_t sensor_id, p_pwr_soc_element_sensor_temp_t sensor_temp_data)
{
    // parameter check: sensor_id, check if correct
    if (sensor_id >= NUMBER_OF_SOC_TEMP_SENSORS || sensor_temp_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP);
    }
    else
    {
        sensor_temp_data->average_dC =
            data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].running_avg);
        sensor_temp_data->max_dC = computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].max;
        sensor_temp_data->min_dC = computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(p_pwr_soc_element_max_soc_temp_t max_temp_data)
{
    // record is only created on die 0
    max_die_temps_t die1_max_temp = {0};
    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(1, &die1_max_temp);

    uint16_t primary_die_max_soc_temp_avg_dC =
        data_util_running_avg_u16_get(&computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg);

    max_temp_data->average_max_dC =
        data_util_mean_of_means(primary_die_max_soc_temp_avg_dC,
                                computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples,
                                die1_max_temp.average_max_temp_dC,
                                die1_max_temp.num_samples);
    max_temp_data->peak_max_dC = FPFW_MAX(computed_metrics_d2d_2mins.max_soc_temp_dC.max, die1_max_temp.peak_temp_dC);
}

void data_proc_tlm_cmpnt_get_pwr_mpam_core_pwr_data(uint16_t mpam_id, p_pwr_soc_element_mpam_core_power_t mpam_core_pwr_data)
{
    mpam_vm_core_pwr_data_t die1_mpam_data;
    die_2_die_exch_ib_read_pwr_pkg_mpam_core_pwr(1, mpam_id, &die1_mpam_data);

    mpam_core_pwr_data->average_mW =
        data_util_running_avg_u32_get(&computed_metrics_d2d_2mins.mpam[mpam_id].core_power.running_avg) +
        die1_mpam_data.average_pwr_mW;

    mpam_core_pwr_data->max_mW = computed_metrics_d2d_2mins.mpam[mpam_id].core_power.max + die1_mpam_data.max_pwr_mW;
}

void data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(uint16_t mpam_id, p_pwr_soc_element_mpam_throttle_t mpam_throttle_data)
{
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(mpam_throttle_data);
}

void data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(uint16_t core_id, p_inst_core_element_summary_t core_summary_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || core_summary_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_CORE);
    }
    else
    {
        core_summary_data->pstate = core_rt[core_id].latest_pstate;
        core_summary_data->cstate = core_rt[core_id].latest_cstate;
        core_summary_data->frequency_Mhz = dvfs_get_freq_from_plimit(core_rt[core_id].latest_pstate);
        core_summary_data->power_mW = core_rt[core_id].latest_power_mW;
        core_summary_data->voltage_mV = core_rt[core_id].latest_voltage_mV;
        core_summary_data->current_mA = core_rt[core_id].latest_current_mA;
        core_summary_data->temperature_dC = core_rt[core_id].latest_max_value_dC;
        core_summary_data->plimit = core_rt[core_id].latest_plimit;
        core_summary_data->mpam_id = core_rt[core_id].latest_mpam_id;

        // TODO :Below items need to be updated, when corresponding records will have implementation.

        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584924
        core_summary_data->cstate_entry_latency_uS = 0;

        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584925
        core_summary_data->cstate_exit_latency_uS = 0;
        core_summary_data->velocity_boost_priority = 0;

        /* Note : Every bit represt an active throttling*/
        // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2684261
        core_summary_data->throttling_type = data_smpl_get_active_throttle_sources(core_id);
        /* Note:  latest rack priority id*/
        core_summary_data->throttling_rack_priority = core_rt[core_id].latest_rack_throttle_priority;
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_rail_data(uint16_t rail_id, p_inst_soc_element_rail_t rail_data)
{
    // parameter check: core_id, check if correct
    if (rail_id >= MAX_NUM_OF_VR_RAILS || rail_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS);
    }
    else
    {
        // Create Voltage, Current and Temperature Information
        rail_data->current_mA = soc_rt.latest_rail_current_mA[rail_id];
        rail_data->temperature_dC = soc_rt.latest_rail_temperature_dC[rail_id];
        rail_data->voltage_mV = soc_rt.latest_rail_voltage_mV[rail_id];
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_idx, p_inst_soc_element_dimm_runtime_t dimm_data)
{
    // parameter check: dimm_idx, check if correct
    if (dimm_idx >= NUMBER_OF_DIMMS_PER_DIE || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_DIMM_RT);
    }
    else
    {
        /* Note : DIMM Temperatures obtained shall be the max of the two sensors in each DIMM. Each DIMM has two temperature sensors.  */
        dimm_data->temperature_dC = dimm_rt.latest_dimm[dimm_idx].temperature_dC;
        dimm_data->throttling_flags = dimm_rt.latest_dimm[dimm_idx].throttling_flags;
        dimm_data->memory_freq_id = dimm_rt.latest_dimm[dimm_idx].memory_freq_id;
        dimm_data->power_mW = dimm_rt.latest_dimm[dimm_idx].power_mW;

        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592610
        dimm_data->threshold_dC = dimm_rt.latest_dimm[dimm_idx].threshold_dC;
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(uint16_t sensor_id, p_inst_soc_element_die_temp_t sensor_temp_data)
{
    if (sensor_id >= NUMBER_OF_SOC_TEMP_SENSORS || sensor_temp_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP);
    }
    else
    {
        sensor_temp_data->temperature_dC = soc_rt.latest_soc_top_temp_dC[sensor_id];
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_max_temp_data(p_inst_soc_element_max_temp_t max_temp_data)
{
    // note:  packaging won't call this api for secondary dies
    max_temp_data->die0_max_temperature_dC = soc_rt.latest_max_die_temp_dC;
    max_temp_data->die1_max_temperature_dC = die_2_die_exch_ib_read_inst_max_die_temp_dC(1);
}