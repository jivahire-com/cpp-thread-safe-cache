//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file package_interface.c
 * API's to retrieve data for in band packaging and out of band reporting
 */

/*------------- Includes -----------------*/
#include "data_proc_tlm_cmpnt.h"
#include "data_sampling_i.h" // internal APIs
#include "package_interface_i.h"
#include "telemetry_events_i.h"

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
            (*pstate_array)[pstate_index].pstate_id = core[core_id].pstate[pstate_index].pstate_id;
            (*pstate_array)[pstate_index].avg_power_mW = core[core_id].pstate[pstate_index].avg_power_mW;
            (*pstate_array)[pstate_index].min_power_mW = core[core_id].pstate[pstate_index].min_power_mW;
            (*pstate_array)[pstate_index].max_power_mW = core[core_id].pstate[pstate_index].max_power_mW;
            (*pstate_array)[pstate_index].frequency_Mhz = core[core_id].pstate[pstate_index].frequency_Mhz;
            (*pstate_array)[pstate_index].residency_mS =
                ROUND_USEC_TO_MSEC(core[core_id].pstate[pstate_index].residency_uS);
            (*pstate_array)[pstate_index].entry_count = core[core_id].pstate[pstate_index].entry_count;
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
            (*cstate_array)[cstate_index].cstate_id = core[core_id].cstate[cstate_index].cstate_id;
            (*cstate_array)[cstate_index].residency_mS =
                ROUND_USEC_TO_MSEC(core[core_id].cstate[cstate_index].residency_uS);
            (*cstate_array)[cstate_index].entry_count = core[core_id].cstate[cstate_index].entry_count;
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
            (*throttle_array)[throttle_index].avg_pstate = core[core_id].throttle_info[throttle_index].avg_pstate;
            (*throttle_array)[throttle_index].entry_count = core[core_id].throttle_info[throttle_index].entry_count;
            (*throttle_array)[throttle_index].max_pstate = core[core_id].throttle_info[throttle_index].max_pstate;
            (*throttle_array)[throttle_index].residency_mS = core[core_id].throttle_info[throttle_index].residency_mS;
            (*throttle_array)[throttle_index].type_id = core[core_id].throttle_info[throttle_index].type_id;
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
            (*rack_priority_array)[rack_index].priority_id = core[core_id].priorities[rack_index].priority_id;
            (*rack_priority_array)[rack_index].entry_count = core[core_id].priorities[rack_index].entry_count;
            (*rack_priority_array)[rack_index].residency_mS = core[core_id].priorities[rack_index].residency_mS;
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
        voltage_data->latest_value_mV = core[core_id].voltage.latest_value_mV;
        voltage_data->average_mV = core[core_id].voltage.average_mV;
        voltage_data->max_mV = core[core_id].voltage.max_mV;
        voltage_data->min_mV = core[core_id].voltage.min_mV;
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
        current_data->latest_value_mA = core[core_id].current.latest_value_mA;
        current_data->average_mA = core[core_id].current.average_mA;
        current_data->max_mA = core[core_id].current.max_mA;
        current_data->min_mA = core[core_id].current.min_mA;
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
        temperature_data->latest_value_dC = core[core_id].temperature.latest_value_dC;
        temperature_data->average_dC = core[core_id].temperature.average_dC;
        temperature_data->max_dC = core[core_id].temperature.max_dC;
        temperature_data->min_dC = core[core_id].temperature.min_dC;
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
        rail_data->current.average_mA = soc_info.rail[rail_id].current.average_mA;
        rail_data->current.latest_value_mA = soc_info.rail[rail_id].current.latest_value_mA;
        rail_data->current.max_mA = soc_info.rail[rail_id].current.max_mA;
        rail_data->current.min_mA = soc_info.rail[rail_id].current.min_mA;
        // get VR Temperature
        rail_data->temperature.latest_value_dC = soc_info.rail[rail_id].temperature.latest_value_dC;
        rail_data->temperature.average_dC = soc_info.rail[rail_id].temperature.average_dC;
        rail_data->temperature.max_dC = soc_info.rail[rail_id].temperature.max_dC;
        rail_data->temperature.min_dC = soc_info.rail[rail_id].temperature.min_dC;
        // get VR voltage
        rail_data->voltage.latest_value_mV = soc_info.rail[rail_id].voltage.latest_value_mV;
        rail_data->voltage.average_mV = soc_info.rail[rail_id].voltage.average_mV;
        rail_data->voltage.max_mV = soc_info.rail[rail_id].voltage.max_mV;
        rail_data->voltage.min_mV = soc_info.rail[rail_id].voltage.min_mV;
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
        hnf_data->latest_value_dC = soc_info.hnf[hnf_channel].latest_value_dC;
        hnf_data->average_dC = soc_info.hnf[hnf_channel].average_dC;
        hnf_data->max_dC = soc_info.hnf[hnf_channel].max_dC;
        hnf_data->min_dC = soc_info.hnf[hnf_channel].min_dC;
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
        // TODO: fill in complete data structure https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592133
        // DIMM temperature s0
        dimm_data->s0.latest_value_dC = soc_info.dimm[dimm_channel].s0.latest_value_dC;
        // DIMM temperature s1
        dimm_data->s1.latest_value_dC = soc_info.dimm[dimm_channel].s1.latest_value_dC;
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
        sensor_temp_data->latest_value_dC = soc_info.sensor_temp[sensor_id].latest_value_dC;
        sensor_temp_data->average_dC = soc_info.sensor_temp[sensor_id].average_dC;
        sensor_temp_data->max_dC = soc_info.sensor_temp[sensor_id].max_dC;
        sensor_temp_data->min_dC = soc_info.sensor_temp[sensor_id].min_dC;
    }
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
        core_summary_data->cstate = core[core_id].cstate_from_pstate_pkt;
        core_summary_data->frequency_Mhz = core[core_id].pstate[current_pstate].frequency_Mhz;
        core_summary_data->power_mW = core[core_id].average_pwr_mW;
        core_summary_data->voltage_mV = core[core_id].voltage.latest_value_mV;
        core_summary_data->current_mA = core[core_id].current.latest_value_mA;
        core_summary_data->temperature_dC = core[core_id].temperature.latest_value_dC;
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
            data_smpl_parse_throttling_get_index_from_status(core[core_id].throttling_status);
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
        rail_data->current_mA = soc_info.rail[rail_id].current.average_mA;
        rail_data->temperature_dC = soc_info.rail[rail_id].temperature.latest_value_dC;
        rail_data->voltage_mV = soc_info.rail[rail_id].voltage.latest_value_mV;
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
    // perf_soc_temp_fill_data for sensor
    if (sensor_id >= NUMBER_OF_SOC_TEMP_SENSORS || sensor_temp_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP);
    }
    else
    {
        // TODO: update via  https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584944

        // sensor_temp_data->latest_value_dC = soc_info.sensor_temp->latest_value_dC;
        // sensor_temp_data->average_dC = soc_info.sensor_temp->average_dC;
        // sensor_temp_data->max_dC = soc_info.sensor_temp->max_dC;
        // sensor_temp_data->min_dC = soc_info.sensor_temp->min_dC;
    }
}