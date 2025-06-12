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
#include "die_2_die_exchange_i.h"
#include "package_interface_i.h"
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
            (*pstate_array)[pstate_index].avg_power_mW =
                computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.running_avg.average;
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
                                                    pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_TYPES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || throttle_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_THROTTLE);
    }
    else
    {
        for (uint16_t throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
        {
            (*throttle_array)[throttle_index].avg_pstate =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate;
            (*throttle_array)[throttle_index].entry_count =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].entry_count;
            (*throttle_array)[throttle_index].max_pstate =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate;
            (*throttle_array)[throttle_index].residency_mS =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS;
            (*throttle_array)[throttle_index].type_id = throttle_index;
            (*throttle_array)[throttle_index].overrun_count =
                computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].overrun_count;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id,
                                                         pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_PRIORITIES])
{

    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || rack_priority_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES);
    }
    else
    {
        for (uint16_t rack_index = 0; rack_index < NUMBER_OF_RACK_PRIORITIES; rack_index++)
        {
            (*rack_priority_array)[rack_index].priority_id = rack_index;
            (*rack_priority_array)[rack_index].entry_count =
                computed_metrics_2_mins.cores[core_id].rack_priorities[rack_index].entry_count;
            (*rack_priority_array)[rack_index].residency_mS =
                computed_metrics_2_mins.cores[core_id].rack_priorities[rack_index].residency_mS;
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
        voltage_data->average_mV = computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.average;
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
        current_data->latest_value_mA = core[core_id].latest_current_mA;
        current_data->average_mA = computed_metrics_2_mins.cores[core_id].current_mA.running_avg.average;
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
        temperature_data->average_dC = computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg.average;
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
        power_data->average_mW = computed_metrics_2_mins.cores[core_id].power_mW.running_avg.average;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(
    uint16_t core_id,
    pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES])
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(histogram_array);
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
        // get VR Current. voltage and temperature entry.
        rail_data->current.average_mA = computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.running_avg.average;
        rail_data->current.max_mA = computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.max;
        rail_data->current.min_mA = computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.min;

        // get VR Temperature
        rail_data->temperature.average_dC =
            computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.running_avg.average;
        rail_data->temperature.max_dC = computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.max;
        rail_data->temperature.min_dC = computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.min;
        // get VR voltage
        rail_data->voltage.average_mV = computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.running_avg.average;
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
        hnf_data->average_dC = computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.average;
        hnf_data->max_dC = computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max;
        hnf_data->min_dC = computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(uint16_t dimm_channel, p_pwr_soc_element_dimm_temp_t dimm_data)
{
    // parameter check: dimm_channel, check if correct
    if (dimm_channel >= NUMBER_OF_DIMM_CHANNELS || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE);
    }
    else
    {
        // DIMM temperature s0
        dimm_data->s0.max_dC = computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s0_dC.max;
        dimm_data->s0.min_dC = computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s0_dC.min;
        dimm_data->s0.average_dC =
            computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s0_dC.running_avg.average;
        // DIMM temperature s1
        dimm_data->s1.max_dC = computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s1_dC.max;
        dimm_data->s1.min_dC = computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s1_dC.min;
        dimm_data->s1.average_dC =
            computed_metrics_2_mins.soc.dimm[dimm_channel].temperature_s1_dC.running_avg.average;
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
        sensor_temp_data->average_dC = computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].running_avg.average;
        sensor_temp_data->max_dC = computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].max;
        sensor_temp_data->min_dC = computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].min;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(p_pwr_soc_element_max_soc_temp_t max_temp_data)
{
    // record is only created on die 0
    max_die_temps_t die1_max_temp = {0};
    die_2_die_exchange_read_pwr_pkg_max_die_temp_dC(1, &die1_max_temp);

    max_temp_data->average_max_dC =
        data_util_mean_of_means(computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.average,
                                computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples,
                                die1_max_temp.average_max_temp_dC,
                                die1_max_temp.num_samples);
    max_temp_data->peak_max_dC = FPFW_MAX(computed_metrics_d2d_2mins.max_soc_temp_dC.max, die1_max_temp.peak_temp_dC);
}

void data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(uint16_t mpam_id,
                                                  pwr_soc_element_mpam_pstate_t (*mpam_pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(mpam_pstate_array);
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
        // Depending on the throttling status, we need to use a different source for what pstate id the
        // core is currently in.
        uint8_t current_pstate = core[core_id].pstate_from_pstate_pkt;
        if (core[core_id].throttling_status != NO_THROTTLE)
        {
            /* Note : DVFS engine can generate a lot of Pstate changes during throttling the
            Pstate telemetry get suppressed and the only way to see Pstate samples is from
            harvesting them from the periodic current telemetry packets */
            current_pstate = core[core_id].pstate_from_current_pkt;
        }

        core_summary_data->pstate = current_pstate;
        core_summary_data->cstate = core[core_id].latest_cstate;
        core_summary_data->frequency_Mhz = dvfs_get_freq_from_plimit(current_pstate);
        core_summary_data->power_mW = core[core_id].latest_power_mW;
        core_summary_data->voltage_mV = core[core_id].latest_voltage_mV;
        core_summary_data->current_mA = core[core_id].latest_current_mA;
        core_summary_data->temperature_dC = core[core_id].latest_max_value_dC;
        core_summary_data->plimit = core[core_id].active_sample_plimit;

        // TODO :Below items need to be updated, when corresponding records will have implementation.
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584933
        core_summary_data->mpam_id = 0;
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584924
        core_summary_data->cstate_entry_latency_uS = 0;
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584925
        core_summary_data->cstate_exit_latency_uS = 0;
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584939
        core_summary_data->guard_band_voltage_mV = 0;
        core_summary_data->velocity_boost_priority = 0;
        // TODO: what if not throttling and what if rack throttling (rack type + vm priority )?
        core_summary_data->throttling_type_and_rack_priority =
            data_smpl_parse_throttling_state_change_get_index_from_status(core[core_id].throttling_status);
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
        rail_data->current_mA = soc_info.latest_rail_current_mA[rail_id];
        rail_data->temperature_dC = soc_info.latest_rail_temperature_dC[rail_id];
        rail_data->voltage_mV = soc_info.latest_rail_voltage_mV[rail_id];
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_module, p_inst_soc_element_dimm_runtime_t dimm_data)
{
    // parameter check: dimm_channel, check if correct
    if (dimm_module >= NUMBER_OF_DIMM_CHANNELS || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_DIMM_RT);
    }
    else
    {

        // TODO: update via https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592610
        // // DIMM temperature s0
        // dimm_data->temp_s0_latest_dC = soc_info.dimm[dimm_module].s0.latest_value_dC;
        // // DIMM temperature s1
        // dimm_data->temp_s1_latest_dC = soc_info.dimm[dimm_module].s1.latest_value_dC;
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
        sensor_temp_data->temperature_dC = soc_info.latest_soc_top_temp_dC[sensor_id];
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_max_temp_data(p_inst_soc_element_max_temp_t max_temp_data)
{
    // note:  packaging won't call this api for secondary dies
    max_temp_data->die0_max_temperature_dC = soc_info.latest_max_die_temp_dC;
    max_temp_data->die1_max_temperature_dC = die_2_die_exchange_read_inst_max_die_temp_dC(1);
}