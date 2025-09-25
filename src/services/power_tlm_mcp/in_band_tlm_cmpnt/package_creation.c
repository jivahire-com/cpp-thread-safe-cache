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
#include <message_transfer_service.h>
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

uint8_t core_id_offset_per_die = 0;
uint8_t voltage_rail_id_offset_per_die = 0;
uint8_t hnf_id_offset_per_die = 0;
uint8_t temp_id_offset_per_die = 0;
uint8_t dimm_id_offset_per_die = 0;

static_assert(sizeof(((telemetry_payload_header_t*)0)->manifest_id) <= sizeof(g_note_gnu_build_id.BuildId),
              "Source ID is too small");

/*------------- Functions ----------------*/

void package_creation_init()
{
    uint8_t die_id = mts_get_this_die_id();

    //
    // Since each die can produce it's own telemetry we need a way to distinguish between the elements
    // on that die and the elements on other die (other than die id). Set up those offsets here, and
    // use them when we create the collections in the records in the packages.
    //
    core_id_offset_per_die = die_id * NUMBER_OF_CORES_PER_DIE;
    voltage_rail_id_offset_per_die = die_id * MAX_NUM_OF_VR_RAILS;
    hnf_id_offset_per_die = die_id * NUMBER_OF_HNF_CHANNELS_PER_DIE;
    temp_id_offset_per_die = die_id * NUMBER_OF_SOC_TEMP_SENSORS;
    dimm_id_offset_per_die = die_id * NUMBER_OF_DIMMS_PER_DIE;
}

bool in_band_tlm_cmpnt_is_any_instantaneous_enabled(void)
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

bool in_band_tlm_cmpnt_is_power_record_enabled(pwr_telemetry_element_id_t element_id)
{
    return (element_id < POWER_TELEMETRY_ELEMENT_ID_MAX) && power_pkg_element_enable[element_id];
}

bool in_band_tlm_cmpnt_is_inst_record_enabled(instantaneous_telemetry_element_id_t element_id)
{
    return (element_id < INST_TELEMETRY_ELEMENT_ID_MAX) && inst_pkg_element_enable[element_id];
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
        FPFW_ET_LOG(PwrPkgRecordEnable, element_id, enable_record);
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
        FPFW_ET_LOG(InstPkgRecordEnable, element_id, enable_record);
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
    package_hdr->payload_header.source_die = mts_get_this_die_id();
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

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_POWER])
    {
        p_pwr_core_record_power_t power_record = (p_pwr_core_record_power_t)pkg_location;
        pkg_location += package_create_pwr_core_power_record(power_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_DROOPS])
    {
        p_pwr_core_record_droop_count_t droop_count_record = (p_pwr_core_record_droop_count_t)pkg_location;
        pkg_location += package_create_pwr_core_droop_count_record(droop_count_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS])
    {
        p_pwr_soc_record_vr_rail_t vr_rail_record = (p_pwr_soc_record_vr_rail_t)pkg_location;
        pkg_location += package_create_pwr_soc_vr_rail_record(vr_rail_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE])
    {
        p_pwr_soc_record_dimm_temp_t dimm_temp_record = (p_pwr_soc_record_dimm_temp_t)pkg_location;
        pkg_location += package_create_pwr_soc_dimm_temp_record(dimm_temp_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER])
    {
        p_pwr_soc_record_dimm_power_t dimm_pwr_record = (p_pwr_soc_record_dimm_power_t)pkg_location;
        pkg_location += package_create_pwr_soc_dimm_power_record(dimm_pwr_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP])
    {
        p_pwr_soc_record_hnf_t hnf_record = (p_pwr_soc_record_hnf_t)pkg_location;
        pkg_location += package_create_pwr_soc_hnf_record(hnf_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP])
    {
        p_pwr_soc_record_sensor_temp_t snsr_temp_record = (p_pwr_soc_record_sensor_temp_t)pkg_location;
        pkg_location += package_create_pwr_soc_sensor_temp_record(snsr_temp_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH])
    {
        p_pwr_soc_record_die_mesh_t per_die_mesh_record = (p_pwr_soc_record_die_mesh_t)pkg_location;
        pkg_location += package_create_pwr_soc_die_mesh_record(per_die_mesh_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE])
    {
        p_pwr_soc_record_d2d_link_t die_to_die_link_state_record = (p_pwr_soc_record_d2d_link_t)pkg_location;
        pkg_location += package_create_pwr_soc_d2d_link_record(die_to_die_link_state_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE])
    {
        if (inband_die_id == 0)
        {
            // this record only exported on die 0.
            p_pwr_soc_record_max_soc_temp_t max_temp_record = (p_pwr_soc_record_max_soc_temp_t)pkg_location;
            pkg_location += package_create_pwr_soc_max_temp_record(max_temp_record);
            package_hdr->payload_header.number_of_records++;
        }
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER])
    {
        if (inband_die_id == 0)
        {
            p_pwr_soc_record_mpam_core_power_t mpam_record = (p_pwr_soc_record_mpam_core_power_t)pkg_location;
            pkg_location += package_create_pwr_mpam_core_pwr_record(mpam_record);
            package_hdr->payload_header.number_of_records++;
        }
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE])
    {
        if (inband_die_id == 0)
        {
            p_pwr_soc_record_mpam_throttle_t mpam_record = (p_pwr_soc_record_mpam_throttle_t)pkg_location;
            pkg_location += package_create_pwr_mpam_throttle_record(mpam_record);
            package_hdr->payload_header.number_of_records++;
        }
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER])
    {
        if (inband_die_id == 0)
        {
            p_pwr_soc_record_mpam_memory_power_t mpam_record = (p_pwr_soc_record_mpam_memory_power_t)pkg_location;
            pkg_location += package_create_pwr_mpam_memory_power_record(mpam_record);
            package_hdr->payload_header.number_of_records++;
        }
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

uint32_t package_create_24hr_pkg(uintptr_t pkg_location, size_t pkg_available_size)
{
    static uint32_t _24hr_package_number = 1;

    if (pkg_available_size < POWER_24HR_PKG_MAX_SIZE)
    {
        FPFW_ET_LOG(PkgCreatePwrPkgNotEnoughSpace, pkg_available_size);
        return 0;
    }

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)pkg_location;

    package_create_populate_hdr(package_hdr);
    package_hdr->payload_header.package_number = _24hr_package_number++;

    pkg_location += sizeof(telemetry_package_hdr_t);

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM])
    {
        p_pwr_core_record_histogram_t histogram_record = (p_pwr_core_record_histogram_t)pkg_location;
        pkg_location += package_create_pwr_core_histogram_record(histogram_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_AGING])
    {
        p_pwr_core_record_aging_t pkg_aging_record = (p_pwr_core_record_aging_t)pkg_location;
        pkg_location += package_create_pwr_core_aging_record(pkg_aging_record);
        package_hdr->payload_header.number_of_records++;
    }

    if (power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PKG_MON])
    {
        p_pwr_soc_record_pkg_monitor_t pkg_mon_record = (p_pwr_soc_record_pkg_monitor_t)pkg_location;
        pkg_location += package_create_pwr_soc_pkg_mon_record(pkg_mon_record);
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
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS])
    {
        p_inst_soc_record_rail_t rail_record = (p_inst_soc_record_rail_t)next_position;
        next_position += package_create_inst_soc_rail_record(rail_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_DIMM_RT])
    {
        p_inst_soc_record_dimm_runtime_t dimm_temp_record = (p_inst_soc_record_dimm_runtime_t)next_position;
        next_position += package_create_inst_soc_dimm_runtime_record(dimm_temp_record);
        pkg_hdr->payload_header.number_of_records++;
    }
    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP])
    {
        p_inst_soc_record_die_temp_t snsr_temp_record = (p_inst_soc_record_die_temp_t)next_position;
        next_position += package_create_inst_soc_sensor_temp_record(snsr_temp_record);
        pkg_hdr->payload_header.number_of_records++;
    }

    if (inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP])
    {
        if (inband_die_id == 0)
        {
            // this record only exported on die 0.
            p_inst_soc_record_max_temp_t max_temp_record = (p_inst_soc_record_max_temp_t)next_position;
            next_position += package_create_inst_soc_max_temp_record(max_temp_record);
            pkg_hdr->payload_header.number_of_records++;
        }
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    NUMBER_OF_THROTTLE_SOURCES,
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    NUMBER_OF_RACK_THROTTLE_PRIORITIES,
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    1,
                                    sizeof(pwr_core_collection_temperature_t));

        data_proc_tlm_cmpnt_get_pwr_core_temperature_data(core_id,
                                                          &temperature_record->temperature_collection[core_id].temperature_element);
    }
    return sizeof(pwr_core_record_temperature_t);
}

uint32_t package_create_pwr_core_power_record(p_pwr_core_record_power_t power_record)
{
    populate_record_hdr(&power_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_POWER],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_power_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&power_record->power_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_POWER,
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    1,
                                    sizeof(pwr_core_collection_power_t));

        data_proc_tlm_cmpnt_get_pwr_core_power_data(core_id, &power_record->power_collection[core_id].power_element);
    }
    return sizeof(pwr_core_record_power_t);
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
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES,
                                    sizeof(pwr_core_collection_histogram_t));

        data_proc_tlm_cmpnt_get_pwr_core_histogram_data(core_id,
                                                        &histogram_record->histogram_collection[core_id].histogram_element);
    }
    return sizeof(pwr_core_record_histogram_t);
}

uint32_t package_create_pwr_core_aging_record(p_pwr_core_record_aging_t aging_record)
{
    populate_record_hdr(&aging_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_AGING],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_aging_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&aging_record->aging_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_AGING,
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    NUMBER_OF_AGING_COUNTER_PAIRS,
                                    sizeof(pwr_core_collection_aging_t));

        data_proc_tlm_cmpnt_get_pwr_core_aging_data(core_id, &aging_record->aging_collection[core_id].aging_element);
    }
    return sizeof(pwr_core_record_aging_t);
}

uint32_t package_create_pwr_core_droop_count_record(p_pwr_core_record_droop_count_t droop_count_record)
{
    populate_record_hdr(&droop_count_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_CORE_DROOPS],
                        NUMBER_OF_CORES_PER_DIE,
                        sizeof(pwr_core_record_droop_count_t));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        populate_pwr_collection_hdr(&droop_count_record->droop_count_collection[core_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_CORE_DROOPS,
                                    CORE_ID_WITH_DIE_OFFSET(core_id),
                                    1,
                                    sizeof(pwr_core_collection_droop_count_t));

        data_proc_tlm_cmpnt_get_pwr_core_droop_count_data(core_id,
                                                          &droop_count_record->droop_count_collection[core_id].droop_count_element);
    }
    return sizeof(pwr_core_record_droop_count_t);
}

uint32_t package_create_pwr_soc_pkg_mon_record(p_pwr_soc_record_pkg_monitor_t pkg_mon_record)
{
    populate_record_hdr(&pkg_mon_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_PKG_MON],
                        1,
                        sizeof(pwr_soc_record_pkg_monitor_t));

    populate_pwr_collection_hdr(&pkg_mon_record->pkg_mon_collection.collection_header,
                                POWER_TELEMETRY_ELEMENT_SOC_PKG_MON,
                                1,
                                1,
                                sizeof(pwr_soc_collection_pkg_monitor_t));

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_record->pkg_mon_collection.pkg_mon_element);

    return sizeof(pwr_soc_record_pkg_monitor_t);
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
                                    VOLTAGE_RAIL_ID_WITH_DIE_OFFSET(rail_id),
                                    1,
                                    sizeof(pwr_soc_collection_vr_rail_t));

        data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(rail_id, &vr_rail_record->rail_collection[rail_id].rail_element);
    }

    return sizeof(pwr_soc_record_vr_rail_t);
}

uint32_t package_create_pwr_soc_hnf_record(p_pwr_soc_record_hnf_t hnf_record)
{
    populate_record_hdr(&hnf_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP],
                        NUMBER_OF_HNF_CHANNELS_PER_DIE,
                        sizeof(pwr_soc_record_hnf_t));

    for (uint16_t channel = 0; channel < NUMBER_OF_HNF_CHANNELS_PER_DIE; channel++)
    {
        populate_pwr_collection_hdr(&hnf_record->hnf_collection[channel].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP,
                                    HNF_ID_WITH_DIE_OFFSET(channel),
                                    1,
                                    sizeof(pwr_soc_collection_hnf_t));

        data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(channel, &hnf_record->hnf_collection[channel].hnf_element);
    }
    return sizeof(pwr_soc_record_hnf_t);
}

uint32_t package_create_pwr_soc_dimm_temp_record(p_pwr_soc_record_dimm_temp_t dimm_temp_record)
{
    populate_record_hdr(&dimm_temp_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE],
                        NUMBER_OF_DIMMS_PER_DIE,
                        sizeof(pwr_soc_record_dimm_temp_t));

    for (uint16_t dimm_idx = 0; dimm_idx < NUMBER_OF_DIMMS_PER_DIE; dimm_idx++)
    {
        populate_pwr_collection_hdr(&dimm_temp_record->dimm_collection[dimm_idx].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE,
                                    DIMM_ID_WITH_DIE_OFFSET(dimm_idx),
                                    1,
                                    sizeof(pwr_soc_collection_dimm_temp_t));

        dimm_temp_record->dimm_collection[dimm_idx].dimm_element.dimm_id = DIMM_ID_WITH_DIE_OFFSET(dimm_idx);

        data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(dimm_idx, &dimm_temp_record->dimm_collection[dimm_idx].dimm_element);
    }
    return sizeof(pwr_soc_record_dimm_temp_t);
}

uint32_t package_create_pwr_soc_dimm_power_record(p_pwr_soc_record_dimm_power_t dimm_power_record)
{
    populate_record_hdr(&dimm_power_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER],
                        NUMBER_OF_DIMMS_PER_DIE,
                        sizeof(pwr_soc_record_dimm_power_t));

    for (uint16_t dimm_idx = 0; dimm_idx < NUMBER_OF_DIMMS_PER_DIE; dimm_idx++)
    {
        populate_pwr_collection_hdr(&dimm_power_record->dimm_collection[dimm_idx].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER,
                                    DIMM_ID_WITH_DIE_OFFSET(dimm_idx),
                                    1,
                                    sizeof(pwr_soc_collection_dimm_power_t));

        dimm_power_record->dimm_collection[dimm_idx].dimm_element.dimm_id = DIMM_ID_WITH_DIE_OFFSET(dimm_idx);

        data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(dimm_idx,
                                                        &dimm_power_record->dimm_collection[dimm_idx].dimm_element);
    }
    return sizeof(pwr_soc_record_dimm_power_t);
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
                                    TEMP_ID_WITH_DIE_OFFSET(snsr_id),
                                    1,
                                    sizeof(pwr_soc_collection_sensor_temp_t));

        data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(snsr_id,
                                                       &snsr_temp_record->sensor_temp_collection[snsr_id].sensor_temp_element);
    }
    return sizeof(pwr_soc_record_sensor_temp_t);
}

uint32_t package_create_pwr_soc_die_mesh_record(p_pwr_soc_record_die_mesh_t die_mesh_record)
{
    populate_record_hdr(&die_mesh_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH],
                        1,
                        sizeof(pwr_soc_record_die_mesh_t));

    // This record is created per die, and each die has a single mesh topology
    populate_pwr_collection_hdr(&die_mesh_record->die_mesh_collection.collection_header,
                                POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH,
                                0,
                                1,
                                sizeof(pwr_soc_collection_die_mesh_t));

    data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data(&die_mesh_record->die_mesh_collection.die_mesh_element);

    return sizeof(pwr_soc_record_die_mesh_t);
}

uint32_t package_create_pwr_soc_d2d_link_record(p_pwr_soc_record_d2d_link_t d2d_link_record)
{
    populate_record_hdr(&d2d_link_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE],
                        1,
                        sizeof(pwr_soc_record_d2d_link_t));

    // This record is created per die, and each die has a single D2D link
    populate_pwr_collection_hdr(&d2d_link_record->d2d_link_collection.collection_header,
                                POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE,
                                0,
                                1,
                                sizeof(pwr_soc_collection_d2d_link_t));

    data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(&d2d_link_record->d2d_link_collection.d2d_link_element);

    return sizeof(pwr_soc_record_d2d_link_t);
}

uint32_t package_create_pwr_soc_max_temp_record(p_pwr_soc_record_max_soc_temp_t max_temp_record)
{
    populate_record_hdr(&max_temp_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE],
                        1,
                        sizeof(pwr_soc_record_max_soc_temp_t));

    populate_pwr_collection_hdr(&max_temp_record->max_soc_temp_collection.collection_header,
                                POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE,
                                1,
                                1,
                                sizeof(pwr_soc_collection_max_soc_temp_t));

    data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(&max_temp_record->max_soc_temp_collection.max_soc_temp_element);

    return sizeof(pwr_soc_record_max_soc_temp_t);
}

uint32_t package_create_pwr_mpam_core_pwr_record(p_pwr_soc_record_mpam_core_power_t mpam_record)
{
    populate_record_hdr(&mpam_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_soc_record_mpam_core_power_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_record->mpam_core_power_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER,
                                    mpam_id,
                                    1,
                                    sizeof(pwr_soc_collection_mpam_core_power_t));

        data_proc_tlm_cmpnt_get_pwr_soc_mpam_core_pwr_data(mpam_id,
                                                           &mpam_record->mpam_core_power_collection[mpam_id].mpam_core_power_element);
    }
    return sizeof(pwr_soc_record_mpam_core_power_t);
}

uint32_t package_create_pwr_mpam_throttle_record(p_pwr_soc_record_mpam_throttle_t mpam_throttle_record)
{
    populate_record_hdr(&mpam_throttle_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_soc_record_mpam_throttle_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_throttle_record->mpam_throttle_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE,
                                    mpam_id,
                                    1,
                                    sizeof(pwr_soc_collection_mpam_throttle_t));

        data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(
            mpam_id,
            &mpam_throttle_record->mpam_throttle_collection[mpam_id].mpam_throttle_element);
    }
    return sizeof(pwr_soc_record_mpam_throttle_t);
}

uint32_t package_create_pwr_mpam_memory_power_record(p_pwr_soc_record_mpam_memory_power_t mpam_memory_power_record)
{
    populate_record_hdr(&mpam_memory_power_record->record_header,
                        ++power_pkg_record_number[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER],
                        NUMBER_OF_MPAMS,
                        sizeof(pwr_soc_record_mpam_memory_power_t));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        populate_pwr_collection_hdr(&mpam_memory_power_record->mpam_memory_power_collection[mpam_id].collection_header,
                                    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER,
                                    mpam_id,
                                    1,
                                    sizeof(pwr_soc_collection_mpam_memory_power_t));

        data_proc_tlm_cmpnt_get_pwr_soc_mpam_memory_power_data(
            mpam_id,
            &mpam_memory_power_record->mpam_memory_power_collection[mpam_id].mpam_memory_power_element);
    }
    return sizeof(pwr_soc_record_mpam_memory_power_t);
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
                                     CORE_ID_WITH_DIE_OFFSET(core_id),
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
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS],
                        MAX_NUM_OF_VR_RAILS,
                        sizeof(inst_soc_record_rail_t));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        populate_inst_collection_hdr(&rail_record->rail_collection[rail_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS,
                                     VOLTAGE_RAIL_ID_WITH_DIE_OFFSET(rail_id),
                                     1,
                                     sizeof(inst_soc_collection_rail_t));

        data_proc_tlm_cmpnt_get_inst_soc_rail_data(rail_id, &rail_record->rail_collection[rail_id].rail_element);
    }
    return sizeof(inst_soc_record_rail_t);
}

uint32_t package_create_inst_soc_dimm_runtime_record(p_inst_soc_record_dimm_runtime_t dimm_temp_record)
{
    populate_record_hdr(&dimm_temp_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_DIMM_RT],
                        NUMBER_OF_DIMMS_PER_DIE,
                        sizeof(inst_soc_record_dimm_runtime_t));

    for (uint16_t module_id = 0; module_id < NUMBER_OF_DIMMS_PER_DIE; module_id++)
    {
        populate_inst_collection_hdr(&dimm_temp_record->dimm_collection[module_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_DIMM_RT,
                                     DIMM_ID_WITH_DIE_OFFSET(module_id),
                                     1,
                                     sizeof(inst_soc_collection_dimm_runtime_t));

        data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(module_id,
                                                           &dimm_temp_record->dimm_collection[module_id].dimm_rt_element);
    }

    return sizeof(inst_soc_record_dimm_runtime_t);
}

uint32_t package_create_inst_soc_sensor_temp_record(p_inst_soc_record_die_temp_t snsr_temp_record)
{
    populate_record_hdr(&snsr_temp_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP],
                        NUMBER_OF_SOC_TEMP_SENSORS,
                        sizeof(inst_soc_record_die_temp_t));

    for (uint16_t snsr_id = 0; snsr_id < NUMBER_OF_SOC_TEMP_SENSORS; snsr_id++)
    {
        populate_inst_collection_hdr(&snsr_temp_record->temperature_collection[snsr_id].collection_header,
                                     INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP,
                                     TEMP_ID_WITH_DIE_OFFSET(snsr_id),
                                     1,
                                     sizeof(inst_soc_collection_die_temp_t));

        snsr_temp_record->temperature_collection[snsr_id].temperature_element.sensor_id =
            TEMP_ID_WITH_DIE_OFFSET(snsr_id);
        data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(snsr_id,
                                                        &snsr_temp_record->temperature_collection[snsr_id].temperature_element);
    }

    return sizeof(inst_soc_record_die_temp_t);
}

uint32_t package_create_inst_soc_max_temp_record(p_inst_soc_record_max_temp_t max_temp_record)
{
    populate_record_hdr(&max_temp_record->record_header,
                        ++inst_pkg_record_number[INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP],
                        1,
                        sizeof(inst_soc_record_max_temp_t));

    populate_inst_collection_hdr(&max_temp_record->temperature_collection.collection_header,
                                 INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP,
                                 1, // only produced for die 0
                                 1,
                                 sizeof(inst_soc_collection_max_temp_t));

    data_proc_tlm_cmpnt_get_inst_soc_max_temp_data(&max_temp_record->temperature_collection.temperature_element);

    return sizeof(inst_soc_record_max_temp_t);
}
