//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file package_creation.c
 * This file handles creating telemetry packages.
 */

/*------------- Includes -----------------*/

#include "ddr_manager_i.h"
#include "in_band_tlm_cmpnt_i.h"
#include "package_creation_i.h"
#include "telemetry_package_defs.h"

#include <build_data.h>
#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <telemetry_events_i.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t record_number, uint32_t number_of_collections, uint32_t record_size);
static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        pwr_telemetry_element_id_t element_id,
                                        uint16_t collection_id,
                                        uint16_t number_of_elements,
                                        uint32_t collection_size);
static void populate_inst_collection_hdr(p_telemetry_collection_hdr_t header,
                                         instantaneous_telemetry_element_id_t element_id,
                                         uint16_t collection_id,
                                         uint16_t number_of_elements,
                                         uint32_t collection_size);

/*-- Declarations (Statics and globals) --*/

bool power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_ID_MAX];
bool inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_ID_MAX];
uint32_t power_pkg_record_number[POWER_TELEMETRY_ELEMENT_ID_MAX];
uint32_t inst_pkg_record_number[INST_TELEMETRY_ELEMENT_ID_MAX];

static_assert(sizeof(((telemetry_payload_header_t*)0)->manifest_id) <= sizeof(g_note_gnu_build_id.BuildId),
              "Source ID is too small");

/*------------- Functions ----------------*/
bool in_band_tlm_cmpnt_is_instantaneous_enabled(void)
{
    for (uint32_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        if (inst_pkg_element_enable[i])
        {
            return true;
        }
    }
    return false;
}

void package_create_enable_disable_pwr_record(pwr_telemetry_element_id_t element_id, bool enable_record)
{
    if (element_id >= POWER_TELEMETRY_ELEMENT_ID_MAX)
    {
        FPFW_ET_LOG(MtsMgrInvalidEventEnableDisable, EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA, element_id);
    }
    else
    {
        power_pkg_element_enable[element_id] = enable_record;
    }
}

void package_create_enable_disable_inst_record(instantaneous_telemetry_element_id_t element_id, bool enable_record)
{
    if (element_id >= INST_TELEMETRY_ELEMENT_ID_MAX)
    {
        FPFW_ET_LOG(MtsMgrInvalidEventEnableDisable, EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA, element_id);
    }
    else
    {
        inst_pkg_element_enable[element_id] = enable_record;
    }
}

void package_create_populate_hdr(p_telemetry_package_hdr_t package_hdr)
{
    package_hdr->decoder_header.payload_parser_version = DIAG_PWR_TELEMETRY_PAYLOAD_PARSER_V2;
    package_hdr->decoder_header.payload_parser_type = DIAG_PAYLOAD_PARSER_TELEMETRY;

    // the gnu build id is unique per core.  Use the first 16 bytes for the manifest id which needs to be
    // unique for the diagnostic decoder tool to decode the data
    memcpy((void*)&package_hdr->payload_header.manifest_id,
           (void*)g_note_gnu_build_id.BuildId,
           sizeof(package_hdr->payload_header.manifest_id));

    package_hdr->payload_header.timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    package_hdr->payload_header.timestamp_utc = 0; // TODO: get UTC time when available
    package_hdr->payload_header.package_number = 0;
    package_hdr->payload_header.number_of_records = 0;
    package_hdr->payload_header.package_payload_size = 0;
}

uint32_t package_create_power_pkg(uintptr_t pkg_location, size_t pkg_available_size)
{
    static uint32_t pwr_package_number = 1;

    if (pkg_available_size < POWER_PKG_MAX_SIZE)
    {
        FPFW_ET_LOG(PkgCreatePwrPkgNotEnoughSpace, pkg_available_size);
        return 0;
    }

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;

    package_create_populate_hdr(package_hdr);
    package_hdr->payload_header.package_number = pwr_package_number++;

    pkg_location += sizeof(telemetry_package_hdr_t);

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_PSTATE])
    {
        p_pwr_core_record_pstate_t pstate_record = (p_pwr_core_record_pstate_t)pkg_location;
        pkg_location += package_create_pwr_core_pstate_record(pstate_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_CSTATE])
    {
        p_pwr_core_record_cstate_t cstate_record = (p_pwr_core_record_cstate_t)pkg_location;
        pkg_location += package_create_pwr_core_cstate_record(cstate_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_THROTTLE])
    {
        p_pwr_core_record_throttle_t throttle_record = (p_pwr_core_record_throttle_t)pkg_location;
        pkg_location += package_create_pwr_core_throttle_record(throttle_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES])
    {
        p_pwr_core_record_rack_priorities_t rack_priority_record = (p_pwr_core_record_rack_priorities_t)pkg_location;
        pkg_location += package_create_pwr_core_rack_priority_record(rack_priority_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE])
    {
        p_pwr_core_record_voltage_t voltage_record = (p_pwr_core_record_voltage_t)pkg_location;
        pkg_location += package_create_pwr_core_voltage_record(voltage_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_CURRENT])
    {
        p_pwr_core_record_current_t current_record = (p_pwr_core_record_current_t)pkg_location;
        pkg_location += package_create_pwr_core_current_record(current_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE])
    {
        p_pwr_core_record_temperature_t temperature_record = (p_pwr_core_record_temperature_t)pkg_location;
        pkg_location += package_create_pwr_core_temperature_record(temperature_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM])
    {
        p_pwr_core_record_histogram_t histogram_record = (p_pwr_core_record_histogram_t)pkg_location;
        pkg_location += package_create_pwr_core_histogram_record(histogram_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3])
    {
        p_pwr_soc_record_pc3_t pc3_record = (p_pwr_soc_record_pc3_t)pkg_location;
        pkg_location += package_create_pwr_soc_pc3_record(pc3_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS])
    {
        p_pwr_soc_record_vr_rail_t vr_rail_record = (p_pwr_soc_record_vr_rail_t)pkg_location;
        pkg_location += package_create_pwr_soc_vr_rail_record(vr_rail_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_HNF])
    {
        p_pwr_soc_record_hnf_t hnf_record = (p_pwr_soc_record_hnf_t)pkg_location;
        pkg_location += package_create_pwr_soc_hnf_record(hnf_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_DIMM])
    {
        p_pwr_soc_record_dimm_t dimm_record = (p_pwr_soc_record_dimm_t)pkg_location;
        pkg_location += package_create_pwr_soc_dimm_record(dimm_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP])
    {
        p_pwr_soc_record_sensor_temp_t snsr_temp_record = (p_pwr_soc_record_sensor_temp_t)pkg_location;
        pkg_location += package_create_pwr_soc_sensor_temp_record(snsr_temp_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_MPAM])
    {
        p_pwr_record_mpam_pstate_t mpam_record = (p_pwr_record_mpam_pstate_t)pkg_location;
        pkg_location += package_create_pwr_mpam_pstate_record(mpam_record);
        package_hdr->payload_header.number_of_records++;
    }
    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE])
    {
        p_pwr_record_mpam_throttle_t mpam_record = (p_pwr_record_mpam_throttle_t)pkg_location;
        pkg_location += package_create_pwr_mpam_throttle_record(mpam_record);
        package_hdr->payload_header.number_of_records++;
    }

    uint32_t pkg_size = pkg_location - (uintptr_t)package_hdr;
    if (pkg_size == sizeof(telemetry_package_hdr_t))
    {
        // no records enabled, valid case, no event trace
        return 0;
    }
    package_hdr->payload_header.package_payload_size = pkg_size - sizeof(telemetry_package_hdr_t);
    return pkg_size;
}

uint32_t package_create_append_to_inst_pkg(uintptr_t curr_pkg_position, size_t pkg_remaining_size, p_telemetry_package_hdr_t pkg_hdr)
{
    if (pkg_remaining_size < sizeof(inst_full_package_t))
    {
        FPFW_ET_LOG(PkgCreateInstPkgNotEnoughSpace, pkg_remaining_size);
        return 0;
    }

    uintptr_t next_position = curr_pkg_position;

    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE])
    {
        p_inst_core_record_summary_t summary_record = (p_inst_core_record_summary_t)next_position;
        next_position += package_create_inst_core_summary_record(summary_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_RAILS])
    {
        p_inst_soc_record_rail_t rail_record = (p_inst_soc_record_rail_t)next_position;
        next_position += package_create_inst_soc_rail_record(rail_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_DIMM_RT])
    {
        p_inst_soc_record_dimm_runtime_t dimm_record = (p_inst_soc_record_dimm_runtime_t)next_position;
        next_position += package_create_inst_soc_dimm_runtime_record(dimm_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG])
    {
        p_inst_soc_record_dimm_config_t dimm_cfg_record = (p_inst_soc_record_dimm_config_t)next_position;
        next_position += package_create_inst_soc_dimm_config_record(dimm_cfg_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR])
    {
        p_inst_soc_record_sensor_temp_t snsr_temp_record = (p_inst_soc_record_sensor_temp_t)next_position;
        next_position += package_create_inst_soc_sensor_temp_record(snsr_temp_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_AMU])
    {
        p_inst_core_record_amu_counters_t amu_record = (p_inst_core_record_amu_counters_t)next_position;
        next_position += package_create_inst_core_amu_counters_record(amu_record);
        pkg_hdr->payload_header.number_of_records++;
    }

    return next_position - curr_pkg_position;
}

static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t record_number, uint32_t number_of_collections, uint32_t record_size)
{
    header->timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    header->record_number = record_number;
    header->number_of_collections = number_of_collections;
    header->record_payload_size = record_size - sizeof(telemetry_record_hdr_t);
}

static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        pwr_telemetry_element_id_t element_id,
                                        uint16_t collection_id,
                                        uint16_t number_of_elements,
                                        uint32_t collection_size)
{
    header->provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA;
    header->element_id = element_id;
    header->collection_id = collection_id;
    header->number_of_elements = number_of_elements;
    header->collection_payload_size = collection_size - sizeof(telemetry_collection_hdr_t);
}

static void populate_inst_collection_hdr(p_telemetry_collection_hdr_t header,
                                         instantaneous_telemetry_element_id_t element_id,
                                         uint16_t collection_id,
                                         uint16_t number_of_elements,
                                         uint32_t collection_size)
{
    header->provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    header->element_id = element_id;
    header->collection_id = collection_id;
    header->number_of_elements = number_of_elements;
    header->collection_payload_size = collection_size - sizeof(telemetry_collection_hdr_t);
}

uint32_t package_create_pwr_core_pstate_record(p_pwr_core_record_pstate_t pstate_record)
{
    populate_record_hdr(&pstate_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_PSTATE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_pstate_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&pstate_record->pstate_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_PSTATE,
                                    core_id,
                                    NUMBER_OF_PSTATES,
                                    sizeof(pwr_core_collection_pstate_t));

        data_proc_tlm_cmpnt_get_pwr_core_pstate_data(core_id, &pstate_record->pstate_collection[core_id].pstate_element);
    }

    return sizeof(pwr_core_record_pstate_t);
}

uint32_t package_create_pwr_core_cstate_record(p_pwr_core_record_cstate_t cstate_record)
{
    populate_record_hdr(&cstate_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_CSTATE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_cstate_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&cstate_record->cstate_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_CSTATE,
                                    core_id,
                                    NUMBER_OF_CSTATES,
                                    sizeof(pwr_core_collection_cstate_t));

        data_proc_tlm_cmpnt_get_pwr_core_cstate_data(core_id, &cstate_record->cstate_collection[core_id].cstate_element);
    }
    return sizeof(pwr_core_record_cstate_t);
}

uint32_t package_create_pwr_core_throttle_record(p_pwr_core_record_throttle_t throttle_record)
{
    populate_record_hdr(&throttle_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_THROTTLE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_throttle_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&throttle_record->throttle_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_THROTTLE,
                                    core_id,
                                    NUMBER_OF_THROTTLE_TYPES,
                                    sizeof(pwr_core_collection_throttle_t));

        data_proc_tlm_cmpnt_get_pwr_core_throttle_data(core_id, &throttle_record->throttle_collection[core_id].throttle_element);
    }
    return sizeof(pwr_core_record_throttle_t);
}

uint32_t package_create_pwr_core_rack_priority_record(p_pwr_core_record_rack_priorities_t rack_priority_record)
{
    populate_record_hdr(&rack_priority_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_rack_priorities_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&rack_priority_record->rack_priority_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES,
                                    core_id,
                                    NUMBER_OF_RACK_PRIORITIES,
                                    sizeof(pwr_core_collection_rack_priorities_t));

        data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(
            core_id,
            &rack_priority_record->rack_priority_collection[core_id].rack_priority_element);
    }
    return sizeof(pwr_core_record_rack_priorities_t);
}

uint32_t package_create_pwr_core_voltage_record(p_pwr_core_record_voltage_t voltage_record)
{
    populate_record_hdr(&voltage_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_voltage_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&voltage_record->voltage_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_voltage_t));

        data_proc_tlm_cmpnt_get_pwr_core_voltage_data(core_id, &voltage_record->voltage_collection[core_id].voltage_element);
    }
    return sizeof(pwr_core_record_voltage_t);
}

uint32_t package_create_pwr_core_current_record(p_pwr_core_record_current_t current_record)
{
    populate_record_hdr(&current_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_CURRENT],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_current_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&current_record->current_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_CURRENT,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_current_t));

        data_proc_tlm_cmpnt_get_pwr_core_current_data(core_id, &current_record->current_collection[core_id].current_element);
    }
    return sizeof(pwr_core_record_current_t);
}

uint32_t package_create_pwr_core_temperature_record(p_pwr_core_record_temperature_t temperature_record)
{
    populate_record_hdr(&temperature_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_temperature_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&temperature_record->temperature_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE,
                                    core_id,
                                    1,
                                    sizeof(pwr_core_collection_temperature_t));

        data_proc_tlm_cmpnt_get_pwr_core_temperature_data(core_id,
                                                          &temperature_record->temperature_collection[core_id].temperature_element);
    }
    return sizeof(pwr_core_record_temperature_t);
}

uint32_t package_create_pwr_core_histogram_record(p_pwr_core_record_histogram_t histogram_record)
{
    populate_record_hdr(&histogram_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_histogram_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&histogram_record->histogram_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM,
                                    core_id,
                                    NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES,
                                    sizeof(pwr_core_collection_histogram_t));

        data_proc_tlm_cmpnt_get_pwr_core_histogram_data(core_id,
                                                        &histogram_record->histogram_collection[core_id].histogram_element);
    }
    return sizeof(pwr_core_record_histogram_t);
}

uint32_t package_create_pwr_soc_pc3_record(p_pwr_soc_record_pc3_t pc3_record)
{
    populate_record_hdr(&pc3_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_PC3],
                        1,
                        sizeof(pwr_soc_record_pc3_t));

    populate_pwr_collection_hdr(&pc3_record->pc3_collection.collection_header,
                                POWER_TELEMETRY_ELEMENT_SOC_PC3,
                                1,
                                1,
                                sizeof(pwr_soc_collection_pc3_t));

    data_proc_tlm_cmpnt_get_pwr_soc_pc3_data(&pc3_record->pc3_collection.pc3_element);

    return sizeof(pwr_soc_record_pc3_t);
}

uint32_t package_create_pwr_soc_vr_rail_record(p_pwr_soc_record_vr_rail_t vr_rail_record)
{
    populate_record_hdr(&vr_rail_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS],
                        MAX_NUM_OF_VR_RAILS,
                        sizeof(pwr_soc_record_vr_rail_t));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        populate_pwr_collection_hdr(&vr_rail_record->rail_collection[rail_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS,
                                    rail_id,
                                    1,
                                    sizeof(pwr_soc_collection_vr_rail_t));

        data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(rail_id, &vr_rail_record->rail_collection[rail_id].rail_element);
    }

    return sizeof(pwr_soc_record_vr_rail_t);
}

uint32_t package_create_pwr_soc_hnf_record(p_pwr_soc_record_hnf_t hnf_record)
{
    populate_record_hdr(&hnf_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_HNF],
                        NUMBER_OF_HNF_CHANNELS_PER_DIE,
                        sizeof(pwr_soc_record_hnf_t));

    for (uint16_t channel = 0; channel < NUMBER_OF_HNF_CHANNELS_PER_DIE; channel++)
    {
        populate_pwr_collection_hdr(&hnf_record->hnf_collection[channel].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_HNF,
                                    channel,
                                    1,
                                    sizeof(pwr_soc_collection_hnf_t));

        data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(channel, &hnf_record->hnf_collection[channel].hnf_element);
    }
    return sizeof(pwr_soc_record_hnf_t);
}

uint32_t package_create_pwr_soc_dimm_record(p_pwr_soc_record_dimm_t dimm_record)
{
    populate_record_hdr(&dimm_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_DIMM],
                        NUMBER_OF_DIMM_MODULES,
                        sizeof(pwr_soc_record_dimm_t));

    for (uint16_t dimm_module = 0; dimm_module < NUMBER_OF_DIMM_MODULES; dimm_module++)
    {
        populate_pwr_collection_hdr(&dimm_record->dimm_collection[dimm_module].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_DIMM,
                                    dimm_module,
                                    1,
                                    sizeof(pwr_soc_collection_dimm_t));

        data_proc_tlm_cmpnt_get_pwr_soc_dimm_data(dimm_module, &dimm_record->dimm_collection[dimm_module].dimm_element);
    }
    return sizeof(pwr_soc_record_dimm_t);
}

uint32_t package_create_pwr_soc_sensor_temp_record(p_pwr_soc_record_sensor_temp_t snsr_temp_record)
{
    populate_record_hdr(&snsr_temp_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP],
                        NUMBER_OF_SOC_TEMP_SENSORS,
                        sizeof(pwr_soc_record_sensor_temp_t));

    for (uint16_t snsr_id = 0; snsr_id < NUMBER_OF_SOC_TEMP_SENSORS; snsr_id++)
    {
        populate_pwr_collection_hdr(&snsr_temp_record->sensor_temp_collection[snsr_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP,
                                    snsr_id,
                                    1,
                                    sizeof(pwr_soc_collection_sensor_temp_t));

        data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(snsr_id,
                                                       &snsr_temp_record->sensor_temp_collection[snsr_id].sensor_temp_element);
    }
    return sizeof(pwr_soc_record_sensor_temp_t);
}

uint32_t package_create_pwr_mpam_pstate_record(p_pwr_record_mpam_pstate_t mpam_record)
{
    populate_record_hdr(&mpam_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_MPAM],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_record_mpam_pstate_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_record->mpam_pstate_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_MPAM,
                                    mpam_id,
                                    NUMBER_OF_PSTATES,
                                    sizeof(pwr_collection_mpam_pstate_t));

        data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(mpam_id, &mpam_record->mpam_pstate_collection[mpam_id].mpam_pstate_element);
    }
    return sizeof(pwr_record_mpam_pstate_t);
}

uint32_t package_create_pwr_mpam_throttle_record(p_pwr_record_mpam_throttle_t mpam_throttle_record)
{
    populate_record_hdr(&mpam_throttle_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_record_mpam_throttle_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_throttle_record->mpam_throttle_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE,
                                    mpam_id,
                                    1,
                                    sizeof(pwr_collection_mpam_throttle_t));

        data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(
            mpam_id,
            &mpam_throttle_record->mpam_throttle_collection[mpam_id].mpam_throttle_element);
    }
    return sizeof(pwr_record_mpam_throttle_t);
}

uint32_t package_create_inst_core_summary_record(p_inst_core_record_summary_t summary_record)
{
    populate_record_hdr(&summary_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_CORE],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(inst_core_record_summary_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_inst_collection_hdr(&summary_record->inst_core_summary_collection[core_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_CORE,
                                     core_id,
                                     1,
                                     sizeof(inst_core_collection_summary_t));

        data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(
            core_id,
            &summary_record->inst_core_summary_collection[core_id].summary_element);
    }
    return sizeof(inst_core_record_summary_t);
}

uint32_t package_create_inst_soc_rail_record(p_inst_soc_record_rail_t rail_record)
{
    populate_record_hdr(&rail_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_RAILS],
                        MAX_NUM_OF_VR_RAILS,
                        sizeof(inst_soc_record_rail_t));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        populate_inst_collection_hdr(&rail_record->rail_collection[rail_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_RAILS,
                                     rail_id,
                                     1,
                                     sizeof(inst_soc_collection_rail_t));

        data_proc_tlm_cmpnt_get_inst_soc_rail_data(rail_id, &rail_record->rail_collection[rail_id].rail_element);
    }
    return sizeof(inst_soc_record_rail_t);
}

uint32_t package_create_inst_soc_dimm_runtime_record(p_inst_soc_record_dimm_runtime_t dimm_record)
{
    populate_record_hdr(&dimm_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_DIMM_RT],
                        NUMBER_OF_DIMM_MODULES,
                        sizeof(inst_soc_record_dimm_runtime_t));

    for (uint16_t module_id = 0; module_id < NUMBER_OF_DIMM_MODULES; module_id++)
    {
        populate_inst_collection_hdr(&dimm_record->dimm_collection[module_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
                                     module_id,
                                     1,
                                     sizeof(inst_soc_collection_dimm_runtime_t));

        data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(module_id, &dimm_record->dimm_collection[module_id].dimm_rt_element);
    }

    return sizeof(inst_soc_record_dimm_runtime_t);
}

uint32_t package_create_inst_soc_dimm_config_record(p_inst_soc_record_dimm_config_t dimm_cfg_record)
{
    populate_record_hdr(&dimm_cfg_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG],
                        1,
                        sizeof(inst_soc_record_dimm_config_t));

    populate_inst_collection_hdr(&dimm_cfg_record->dimm_config_collection.collection_header,
                                 INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG,
                                 1,
                                 1,
                                 sizeof(inst_soc_collection_dimm_config_t));

    data_proc_tlm_cmpnt_get_inst_soc_dimm_config_data(&dimm_cfg_record->dimm_config_collection.dimm_config_element);

    return sizeof(inst_soc_record_dimm_config_t);
}

uint32_t package_create_inst_soc_sensor_temp_record(p_inst_soc_record_sensor_temp_t snsr_temp_record)
{
    populate_record_hdr(&snsr_temp_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR],
                        NUMBER_OF_SOC_TEMP_SENSORS,
                        sizeof(inst_soc_record_sensor_temp_t));

    for (uint16_t snsr_id = 0; snsr_id < NUMBER_OF_SOC_TEMP_SENSORS; snsr_id++)
    {
        populate_inst_collection_hdr(&snsr_temp_record->temperature_collection[snsr_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR,
                                     snsr_id,
                                     1,
                                     sizeof(inst_soc_collection_sensor_temp_t));

        data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(snsr_id,
                                                        &snsr_temp_record->temperature_collection[snsr_id].temperature_element);
    }

    return sizeof(inst_soc_record_sensor_temp_t);
}

uint32_t package_create_inst_core_amu_counters_record(p_inst_core_record_amu_counters_t amu_record)
{
    populate_record_hdr(&amu_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_AMU],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(inst_core_record_amu_counters_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_inst_collection_hdr(&amu_record->amu_counter_collection[core_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_AMU,
                                     core_id,
                                     1,
                                     sizeof(inst_core_collection_amu_counters_t));

        data_proc_tlm_cmpnt_get_inst_core_amu_data(core_id, &amu_record->amu_counter_collection[core_id].amu_counter_element);
    }

    return sizeof(inst_core_record_amu_counters_t);
}