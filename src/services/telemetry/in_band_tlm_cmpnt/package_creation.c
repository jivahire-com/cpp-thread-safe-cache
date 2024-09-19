//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file package_creation.c
 * This file handles creating telemetry report packages.
 */

/*------------- Includes -----------------*/

#include "in_band_tlm_cmpnt_i.h"
#include "package_creation_i.h"
#include "telemetry_package_defs.h"

#include <data_proc_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t record_number, uint32_t number_of_collections, uint32_t record_size);
static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        power_telemetry_event_t event,
                                        uint16_t collection_id,
                                        uint16_t number_of_events,
                                        uint32_t collection_size);
static void populate_perf_collection_hdr(p_telemetry_collection_hdr_t header,
                                         performance_telemetry_event_t perf_event,
                                         uint16_t collection_id,
                                         uint16_t number_of_events,
                                         uint32_t collection_size);

/*-- Declarations (Statics and globals) --*/
bool power_report_event_enable[POWER_TELEMETRY_EVENT_ID_MAX];
bool perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_ID_MAX];
uint32_t power_report_record_number[POWER_TELEMETRY_EVENT_ID_MAX];
uint32_t perf_report_record_number[PERFORMANCE_TELEMETRY_EVENT_ID_MAX];

size_t const power_report_record_sizes[POWER_TELEMETRY_EVENT_ID_MAX] = {sizeof(pwr_core_record_pstate_t),
                                                                        sizeof(pwr_core_record_cstate_t),
                                                                        sizeof(pwr_core_record_throttle_t),
                                                                        sizeof(pwr_core_record_rack_priorities_t),
                                                                        sizeof(pwr_core_record_voltage_t),
                                                                        sizeof(pwr_core_record_current_t),
                                                                        sizeof(pwr_core_record_temperature_t),
                                                                        sizeof(pwr_core_record_histogram_t),
                                                                        sizeof(pwr_soc_record_pc3_t),
                                                                        sizeof(pwr_soc_record_vr_rail_t),
                                                                        sizeof(pwr_soc_record_hnf_t),
                                                                        sizeof(pwr_soc_record_dimm_t),
                                                                        sizeof(pwr_soc_record_sensor_temp_t),
                                                                        sizeof(pwr_record_mpam_pstate_t),
                                                                        sizeof(pwr_record_mpam_throttle_t)};

uint32_t const perf_report_record_sizes[PERFORMANCE_TELEMETRY_EVENT_ID_MAX] = {
    sizeof(perf_core_record_summary_t),
    sizeof(perf_soc_record_rail_t),
    sizeof(perf_soc_record_dimm_runtime_t),
    sizeof(perf_soc_record_dimm_config_t),
    sizeof(perf_soc_record_sensor_temp_t),
    sizeof(perf_core_record_amu_counters_t)};

static power_telemetry_event_t s_starting_pwr_index = POWER_TELEMETRY_EVENT_CORE_PSTATE;
static performance_telemetry_event_t s_starting_perf_index = PERFORMANCE_TELEMETRY_EVENT_CORE;

/*------------- Functions ----------------*/

fpfw_status_t package_create_power_report(uintptr_t pkg_location, size_t pkg_available_size)
{
    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;
    pkg_location += sizeof(telemetry_package_hdr_t);
    pkg_available_size -= sizeof(telemetry_package_hdr_t);
    package_hdr->client_header.timestamp =
        (uint64_t)tx_time_get(); // TODO: replace with higher resolution timer when available

    bool include_record[POWER_TELEMETRY_EVENT_ID_MAX];

    // records wil only be included if they are enabled and can fit in the package
    // FPFW_STATUS_BUFFER_TOO_SMALL will be returned if not all records can fit to signal the caller to allocate a new package
    fpfw_status_t status = get_power_record_include(pkg_available_size, &include_record);

    if (include_record[POWER_TELEMETRY_EVENT_CORE_PSTATE])
    {
        p_pwr_core_record_pstate_t pstate_record = (p_pwr_core_record_pstate_t)pkg_location;
        create_pwr_core_pstate_record(pstate_record);
        pkg_location += sizeof(pwr_core_record_pstate_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_CSTATE])
    {
        p_pwr_core_record_cstate_t cstate_record = (p_pwr_core_record_cstate_t)pkg_location;
        create_pwr_core_cstate_record(cstate_record);
        pkg_location += sizeof(pwr_core_record_cstate_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_THROTTLE])
    {
        p_pwr_core_record_throttle_t throttle_record = (p_pwr_core_record_throttle_t)pkg_location;
        create_pwr_core_throttle_record(throttle_record);
        pkg_location += sizeof(pwr_core_record_throttle_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES])
    {
        p_pwr_core_record_rack_priorities_t rack_priority_record = (p_pwr_core_record_rack_priorities_t)pkg_location;
        create_pwr_core_rack_priority_record(rack_priority_record);
        pkg_location += sizeof(pwr_core_record_rack_priorities_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_VOLTAGE])
    {
        p_pwr_core_record_voltage_t voltage_record = (p_pwr_core_record_voltage_t)pkg_location;
        create_pwr_core_voltage_record(voltage_record);
        pkg_location += sizeof(pwr_core_record_voltage_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_CURRENT])
    {
        p_pwr_core_record_current_t current_record = (p_pwr_core_record_current_t)pkg_location;
        create_pwr_core_current_record(current_record);
        pkg_location += sizeof(pwr_core_record_current_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_TEMPERATURE])
    {
        p_pwr_core_record_temperature_t temperature_record = (p_pwr_core_record_temperature_t)pkg_location;
        create_pwr_core_temperature_record(temperature_record);
        pkg_location += sizeof(pwr_core_record_temperature_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_CORE_HISTOGRAM])
    {
        p_pwr_core_record_histogram_t histogram_record = (p_pwr_core_record_histogram_t)pkg_location;
        create_pwr_core_histogram_record(histogram_record);
        pkg_location += sizeof(pwr_core_record_histogram_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_SOC_PC3])
    {
        p_pwr_soc_record_pc3_t pc3_record = (p_pwr_soc_record_pc3_t)pkg_location;
        create_pwr_soc_pc3_record(pc3_record);
        pkg_location += sizeof(pwr_soc_record_pc3_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_SOC_VR_RAILS])
    {
        p_pwr_soc_record_vr_rail_t vr_rail_record = (p_pwr_soc_record_vr_rail_t)pkg_location;
        create_pwr_soc_vr_rail_record(vr_rail_record);
        pkg_location += sizeof(pwr_soc_record_vr_rail_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_SOC_HNF])
    {
        p_pwr_soc_record_hnf_t hnf_record = (p_pwr_soc_record_hnf_t)pkg_location;
        create_pwr_soc_hnf_record(hnf_record);
        pkg_location += sizeof(pwr_soc_record_hnf_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_SOC_DIMM])
    {
        p_pwr_soc_record_dimm_t dimm_record = (p_pwr_soc_record_dimm_t)pkg_location;
        create_pwr_soc_dimm_record(dimm_record);
        pkg_location += sizeof(pwr_soc_record_dimm_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP])
    {
        p_pwr_soc_record_sensor_temp_t snsr_temp_record = (p_pwr_soc_record_sensor_temp_t)pkg_location;
        create_pwr_soc_sensor_temp_record(snsr_temp_record);
        pkg_location += sizeof(pwr_soc_record_sensor_temp_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_MPAM])
    {
        p_pwr_record_mpam_pstate_t mpam_record = (p_pwr_record_mpam_pstate_t)pkg_location;
        create_pwr_mpam_pstate_record(mpam_record);
        pkg_location += sizeof(pwr_record_mpam_pstate_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[POWER_TELEMETRY_EVENT_MPAM_THROTTLE])
    {
        p_pwr_record_mpam_throttle_t mpam_record = (p_pwr_record_mpam_throttle_t)pkg_location;
        create_pwr_mpam_throttle_record(mpam_record);
        pkg_location += sizeof(pwr_record_mpam_throttle_t);
        package_hdr->client_header.number_of_records++;
    }
    package_hdr->client_header.package_payload_size =
        pkg_location - (uintptr_t)package_hdr - sizeof(telemetry_package_hdr_t);

    return status;
}

fpfw_status_t package_create_perf_report(uintptr_t pkg_location, size_t pkg_available_size)
{
    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;
    pkg_location += sizeof(telemetry_package_hdr_t);
    pkg_available_size -= sizeof(telemetry_package_hdr_t);
    package_hdr->client_header.timestamp =
        (uint64_t)tx_time_get(); // TODO: replace with higher resolution timer when available

    bool include_record[PERFORMANCE_TELEMETRY_EVENT_ID_MAX];

    // records wil only be included if they are enabled and can fit in the package
    // FPFW_STATUS_BUFFER_TOO_SMALL will be returned if not all records can fit to signal the caller to allocate a new package
    fpfw_status_t status = get_performance_record_include(pkg_available_size, &include_record);

    if (include_record[PERFORMANCE_TELEMETRY_EVENT_CORE])
    {
        p_perf_core_record_summary_t summary_record = (p_perf_core_record_summary_t)pkg_location;
        create_perf_core_summary_record(summary_record);
        pkg_location += sizeof(perf_core_record_summary_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS])
    {
        p_perf_soc_record_rail_t rail_record = (p_perf_soc_record_rail_t)pkg_location;
        create_perf_soc_rail_record(rail_record);
        pkg_location += sizeof(perf_soc_record_rail_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT])
    {
        p_perf_soc_record_dimm_runtime_t dimm_record = (p_perf_soc_record_dimm_runtime_t)pkg_location;
        create_perf_soc_dimm_runtime_record(dimm_record);
        pkg_location += sizeof(perf_soc_record_dimm_runtime_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG])
    {
        p_perf_soc_record_dimm_config_t dimm_cfg_record = (p_perf_soc_record_dimm_config_t)pkg_location;
        create_perf_soc_dimm_config_record(dimm_cfg_record);
        pkg_location += sizeof(perf_soc_record_dimm_config_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR])
    {
        p_perf_soc_record_sensor_temp_t snsr_temp_record = (p_perf_soc_record_sensor_temp_t)pkg_location;
        create_perf_soc_sensor_temp_record(snsr_temp_record);
        pkg_location += sizeof(perf_soc_record_sensor_temp_t);
        package_hdr->client_header.number_of_records++;
    }
    if (include_record[PERFORMANCE_TELEMETRY_EVENT_AMU])
    {
        p_perf_core_record_amu_counters_t amu_record = (p_perf_core_record_amu_counters_t)pkg_location;
        create_perf_core_amu_counters_record(amu_record);
        pkg_location += sizeof(perf_core_record_amu_counters_t);
        package_hdr->client_header.number_of_records++;
    }
    package_hdr->client_header.package_payload_size =
        pkg_location - (uintptr_t)package_hdr - sizeof(telemetry_package_hdr_t);
    return status;
}

fpfw_status_t get_performance_record_include(size_t pkg_available_size,
                                             bool (*include_record)[PERFORMANCE_TELEMETRY_EVENT_ID_MAX])
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    // preset to false
    for (performance_telemetry_event_t perf_index = PERFORMANCE_TELEMETRY_EVENT_CORE;
         perf_index < PERFORMANCE_TELEMETRY_EVENT_ID_MAX;
         perf_index++)
    {
        (*include_record)[perf_index] = false;
    }
    for (performance_telemetry_event_t perf_index = s_starting_perf_index; perf_index < PERFORMANCE_TELEMETRY_EVENT_ID_MAX;
         perf_index++)
    {
        if (perf_report_event_enable[perf_index])
        {
            if (perf_report_record_sizes[perf_index] > pkg_available_size)
            {
                s_starting_perf_index = perf_index;
                return FPFW_STATUS_BUFFER_TOO_SMALL;
            }
            (*include_record)[perf_index] = true;
            pkg_available_size -= perf_report_record_sizes[perf_index];
        }
    }
    s_starting_perf_index = PERFORMANCE_TELEMETRY_EVENT_CORE;
    return status;
}

fpfw_status_t get_power_record_include(size_t pkg_available_size, bool (*include_record)[POWER_TELEMETRY_EVENT_ID_MAX])
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    // preset to false
    for (power_telemetry_event_t pwr_index = POWER_TELEMETRY_EVENT_CORE_PSTATE; pwr_index < POWER_TELEMETRY_EVENT_ID_MAX;
         pwr_index++)
    {
        (*include_record)[pwr_index] = false;
    }
    for (power_telemetry_event_t pwr_index = s_starting_pwr_index; pwr_index < POWER_TELEMETRY_EVENT_ID_MAX; pwr_index++)
    {
        if (power_report_event_enable[pwr_index])
        {
            if (power_report_record_sizes[pwr_index] > pkg_available_size)
            {
                s_starting_pwr_index = pwr_index;
                return FPFW_STATUS_BUFFER_TOO_SMALL;
            }
            (*include_record)[pwr_index] = true;
            pkg_available_size -= power_report_record_sizes[pwr_index];
        }
    }
    s_starting_pwr_index = POWER_TELEMETRY_EVENT_CORE_PSTATE;
    return status;
}

static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t record_number, uint32_t number_of_collections, uint32_t record_size)
{
    header->timestamp = (uint64_t)tx_time_get(); // TODO: replace with higher resolution timer when available
    header->record_number = record_number;
    header->number_of_collections = number_of_collections;
    header->record_payload_size = record_size - sizeof(telemetry_record_hdr_t);
}

static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        power_telemetry_event_t pwr_event,
                                        uint16_t collection_id,
                                        uint16_t number_of_events,
                                        uint32_t collection_size)
{
    header->provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM;
    header->event_id = pwr_event;
    header->collection_id = collection_id;
    header->number_of_events = number_of_events;
    header->collection_payload_size = collection_size - sizeof(telemetry_collection_hdr_t);
}

static void populate_perf_collection_hdr(p_telemetry_collection_hdr_t header,
                                         performance_telemetry_event_t perf_event,
                                         uint16_t collection_id,
                                         uint16_t number_of_events,
                                         uint32_t collection_size)
{
    header->provider_id = EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM;
    header->event_id = perf_event;
    header->collection_id = collection_id;
    header->number_of_events = number_of_events;
    header->collection_payload_size = collection_size - sizeof(telemetry_collection_hdr_t);
}

void create_pwr_core_pstate_record(p_pwr_core_record_pstate_t pstate_record)
{
    populate_record_hdr(&pstate_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_PSTATE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_pstate_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&pstate_record->pstate_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_PSTATE,
                                    core_id,
                                    NUMBER_OF_PSTATES,
                                    sizeof(pwr_core_collection_pstate_t));

        data_proc_tlm_cmpnt_get_pwr_core_pstate_data(core_id, &pstate_record->pstate_collection[core_id].pstate_event);
    }
}

void create_pwr_core_cstate_record(p_pwr_core_record_cstate_t cstate_record)
{
    populate_record_hdr(&cstate_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_CSTATE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_cstate_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&cstate_record->cstate_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_CSTATE,
                                    core_id,
                                    NUMBER_OF_CSTATES,
                                    sizeof(pwr_core_collection_cstate_t));

        data_proc_tlm_cmpnt_get_pwr_core_cstate_data(core_id, &cstate_record->cstate_collection[core_id].cstate_event);
    }
}

void create_pwr_core_throttle_record(p_pwr_core_record_throttle_t throttle_record)
{
    populate_record_hdr(&throttle_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_THROTTLE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_throttle_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&throttle_record->throttle_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_THROTTLE,
                                    core_id,
                                    NUMBER_OF_THROTTLE_TYPES,
                                    sizeof(pwr_core_collection_throttle_t));

        data_proc_tlm_cmpnt_get_pwr_core_throttle_data(core_id, &throttle_record->throttle_collection[core_id].throttle_event);
    }
}

void create_pwr_core_rack_priority_record(p_pwr_core_record_rack_priorities_t rack_priority_record)
{
    populate_record_hdr(&rack_priority_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_rack_priorities_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&rack_priority_record->rack_priority_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES,
                                    core_id,
                                    NUMBER_OF_RACK_PRIORITIES,
                                    sizeof(pwr_core_collection_rack_priorities_t));

        data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(
            core_id,
            &rack_priority_record->rack_priority_collection[core_id].rack_priority_event);
    }
}

void create_pwr_core_voltage_record(p_pwr_core_record_voltage_t voltage_record)
{
    populate_record_hdr(&voltage_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_VOLTAGE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_voltage_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&voltage_record->voltage_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_VOLTAGE,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_voltage_t));

        data_proc_tlm_cmpnt_get_pwr_core_voltage_data(core_id, &voltage_record->voltage_collection[core_id].voltage_event);
    }
}

void create_pwr_core_current_record(p_pwr_core_record_current_t current_record)
{
    populate_record_hdr(&current_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_CURRENT],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_current_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&current_record->current_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_CURRENT,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_current_t));

        data_proc_tlm_cmpnt_get_pwr_core_current_data(core_id, &current_record->current_collection[core_id].current_event);
    }
}

void create_pwr_core_temperature_record(p_pwr_core_record_temperature_t temperature_record)
{
    populate_record_hdr(&temperature_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_TEMPERATURE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_temperature_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&temperature_record->temperature_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_TEMPERATURE,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_temperature_t));

        data_proc_tlm_cmpnt_get_pwr_core_temperature_data(core_id,
                                                          &temperature_record->temperature_collection[core_id].temperature_event);
    }
}

void create_pwr_core_histogram_record(p_pwr_core_record_histogram_t histogram_record)
{
    populate_record_hdr(&histogram_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_CORE_HISTOGRAM],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_histogram_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&histogram_record->histogram_collection[core_id].collection_header,
                                    POWER_TELEMETRY_EVENT_CORE_HISTOGRAM,
                                    core_id,
                                    NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES,
                                    sizeof(pwr_core_collection_histogram_t));

        data_proc_tlm_cmpnt_get_pwr_core_histogram_data(core_id,
                                                        &histogram_record->histogram_collection[core_id].histogram_event);
    }
}

void create_pwr_soc_pc3_record(p_pwr_soc_record_pc3_t pc3_record)
{
    populate_record_hdr(&pc3_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_SOC_PC3],
                        1,
                        sizeof(pwr_soc_record_pc3_t));

    populate_pwr_collection_hdr(&pc3_record->pc3_collection.collection_header,
                                POWER_TELEMETRY_EVENT_SOC_PC3,
                                1,
                                1,
                                sizeof(pwr_soc_collection_pc3_t));

    data_proc_tlm_cmpnt_get_pwr_soc_pc3_data(&pc3_record->pc3_collection.pc3_event);
}

void create_pwr_soc_vr_rail_record(p_pwr_soc_record_vr_rail_t vr_rail_record)
{
    populate_record_hdr(&vr_rail_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_SOC_VR_RAILS],
                        MAX_NUM_OF_VR_RAILS,
                        sizeof(pwr_soc_record_vr_rail_t));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        populate_pwr_collection_hdr(&vr_rail_record->rail_collection[rail_id].collection_header,
                                    POWER_TELEMETRY_EVENT_SOC_VR_RAILS,
                                    rail_id,
                                    1,
                                    sizeof(pwr_soc_collection_vr_rail_t));

        data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(rail_id, &vr_rail_record->rail_collection[rail_id].rail_event);
    }
}

void create_pwr_soc_hnf_record(p_pwr_soc_record_hnf_t hnf_record)
{
    populate_record_hdr(&hnf_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_SOC_HNF],
                        NUMBER_OF_HNF_CHANNELS_PER_DIE,
                        sizeof(pwr_soc_record_hnf_t));

    for (uint16_t channel = 0; channel < NUMBER_OF_HNF_CHANNELS_PER_DIE; channel++)
    {
        populate_pwr_collection_hdr(&hnf_record->hnf_collection[channel].collection_header,
                                    POWER_TELEMETRY_EVENT_SOC_HNF,
                                    channel,
                                    1,
                                    sizeof(pwr_soc_collection_hnf_t));

        data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(channel, &hnf_record->hnf_collection[channel].hnf_event);
    }
}

void create_pwr_soc_dimm_record(p_pwr_soc_record_dimm_t dimm_record)
{
    populate_record_hdr(&dimm_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_SOC_DIMM],
                        NUMBER_OF_DIMM_MODULES,
                        sizeof(pwr_soc_record_dimm_t));

    for (uint16_t dimm_module = 0; dimm_module < NUMBER_OF_DIMM_MODULES; dimm_module++)
    {
        populate_pwr_collection_hdr(&dimm_record->dimm_collection[dimm_module].collection_header,
                                    POWER_TELEMETRY_EVENT_SOC_DIMM,
                                    dimm_module,
                                    1,
                                    sizeof(pwr_soc_collection_dimm_t));

        data_proc_tlm_cmpnt_get_pwr_soc_dimm_data(dimm_module, &dimm_record->dimm_collection[dimm_module].dimm_event);
    }
}

void create_pwr_soc_sensor_temp_record(p_pwr_soc_record_sensor_temp_t snsr_temp_record)
{
    populate_record_hdr(&snsr_temp_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP],
                        NUMBER_OF_SOC_TEMP_SENSORS,
                        sizeof(pwr_soc_record_sensor_temp_t));

    for (uint16_t snsr_id = 0; snsr_id < NUMBER_OF_SOC_TEMP_SENSORS; snsr_id++)
    {
        populate_pwr_collection_hdr(&snsr_temp_record->sensor_temp_collection[snsr_id].collection_header,
                                    POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP,
                                    snsr_id,
                                    1,
                                    sizeof(pwr_soc_collection_sensor_temp_t));

        data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(snsr_id,
                                                       &snsr_temp_record->sensor_temp_collection[snsr_id].sensor_temp_event);
    }
}

void create_pwr_mpam_pstate_record(p_pwr_record_mpam_pstate_t mpam_record)
{
    populate_record_hdr(&mpam_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_MPAM],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_record_mpam_pstate_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_record->mpam_pstate_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_EVENT_MPAM,
                                    mpam_id,
                                    NUMBER_OF_PSTATES,
                                    sizeof(pwr_collection_mpam_pstate_t));

        data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(mpam_id, &mpam_record->mpam_pstate_collection[mpam_id].mpam_pstate_event);
    }
}

void create_pwr_mpam_throttle_record(p_pwr_record_mpam_throttle_t mpam_throttle_record)
{
    populate_record_hdr(&mpam_throttle_record->record_header,
                        ++power_report_record_number[POWER_TELEMETRY_EVENT_MPAM_THROTTLE],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_record_mpam_throttle_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_throttle_record->mpam_throttle_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_EVENT_MPAM_THROTTLE,
                                    mpam_id,
                                    1,
                                    sizeof(pwr_collection_mpam_throttle_t));

        data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(
            mpam_id,
            &mpam_throttle_record->mpam_throttle_collection[mpam_id].mpam_throttle_event);
    }
}

void create_perf_core_summary_record(p_perf_core_record_summary_t summary_record)
{
    populate_record_hdr(&summary_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_CORE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(perf_core_record_summary_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_perf_collection_hdr(&summary_record->perf_core_summary_collection[core_id].collection_header,
                                     PERFORMANCE_TELEMETRY_EVENT_CORE,
                                     core_id,
                                     1,
                                     sizeof(perf_core_collection_summary_t));

        data_proc_tlm_cmpnt_get_perf_soc_core_summary_data(
            core_id,
            &summary_record->perf_core_summary_collection[core_id].summary_event);
    }
}

void create_perf_soc_rail_record(p_perf_soc_record_rail_t rail_record)
{
    populate_record_hdr(&rail_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS],
                        MAX_NUM_OF_VR_RAILS,
                        sizeof(perf_soc_record_rail_t));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        populate_perf_collection_hdr(&rail_record->rail_collection[rail_id].collection_header,
                                     PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS,
                                     rail_id,
                                     1,
                                     sizeof(perf_soc_collection_rail_t));

        data_proc_tlm_cmpnt_get_perf_soc_rail_data(rail_id, &rail_record->rail_collection[rail_id].rail_event);
    }
}

void create_perf_soc_dimm_runtime_record(p_perf_soc_record_dimm_runtime_t dimm_record)
{
    populate_record_hdr(&dimm_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT],
                        NUMBER_OF_DIMM_MODULES,
                        sizeof(perf_soc_record_dimm_runtime_t));

    for (uint16_t module_id = 0; module_id < NUMBER_OF_DIMM_MODULES; module_id++)
    {
        populate_perf_collection_hdr(&dimm_record->dimm_collection[module_id].collection_header,
                                     PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT,
                                     module_id,
                                     1,
                                     sizeof(perf_soc_collection_dimm_runtime_t));

        data_proc_tlm_cmpnt_get_perf_soc_dimm_runtime_data(module_id, &dimm_record->dimm_collection[module_id].dimm_rt_event);
    }
}

void create_perf_soc_dimm_config_record(p_perf_soc_record_dimm_config_t dimm_cfg_record)
{
    populate_record_hdr(&dimm_cfg_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG],
                        1,
                        sizeof(perf_soc_record_dimm_config_t));

    populate_perf_collection_hdr(&dimm_cfg_record->dimm_config_collection.collection_header,
                                 PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG,
                                 1,
                                 1,
                                 sizeof(perf_soc_collection_dimm_config_t));

    data_proc_tlm_cmpnt_get_perf_soc_dimm_config_data(&dimm_cfg_record->dimm_config_collection.dimm_config_event);
}

void create_perf_soc_sensor_temp_record(p_perf_soc_record_sensor_temp_t snsr_temp_record)
{
    populate_record_hdr(&snsr_temp_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR],
                        NUMBER_OF_SOC_TEMP_SENSORS,
                        sizeof(perf_soc_record_sensor_temp_t));

    for (uint16_t snsr_id = 0; snsr_id < NUMBER_OF_SOC_TEMP_SENSORS; snsr_id++)
    {
        populate_perf_collection_hdr(&snsr_temp_record->temperature_collection[snsr_id].collection_header,
                                     PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR,
                                     snsr_id,
                                     1,
                                     sizeof(perf_soc_collection_sensor_temp_t));

        data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data(snsr_id,
                                                        &snsr_temp_record->temperature_collection[snsr_id].temperature_event);
    }
}

void create_perf_core_amu_counters_record(p_perf_core_record_amu_counters_t amu_record)
{
    populate_record_hdr(&amu_record->record_header,
                        ++power_report_record_number[PERFORMANCE_TELEMETRY_EVENT_AMU],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(perf_core_record_amu_counters_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_perf_collection_hdr(&amu_record->amu_counter_collection[core_id].collection_header,
                                     PERFORMANCE_TELEMETRY_EVENT_AMU,
                                     core_id,
                                     1,
                                     sizeof(perf_core_collection_amu_counters_t));

        data_proc_tlm_cmpnt_get_perf_core_amu_data(core_id, &amu_record->amu_counter_collection[core_id].amu_counter_event);
    }
}