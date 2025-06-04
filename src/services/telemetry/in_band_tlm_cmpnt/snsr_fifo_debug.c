//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file snsr_fifo_debug.c
 * Debug mode where raw data is collected from the sensor fifo and stored in DDR
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "in_band_tlm_cmpnt.h"
#include "mts_manager_i.h"
#include "package_creation_i.h"
#include "snsr_fifo_debug_i.h"
#include "telemetry_events_i.h"
#include "telemetry_package_defs.h"

#include <FpFwAssert.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_RECORD_RESERVATION_SIZE (65 * 1024) // 64K sensor fifo plus package headers
#define MAX_BUFFER_ENTRIES          8

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t number_of_collections, uint32_t record_payload_size);
static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        uint16_t collection_id,
                                        uint16_t number_of_elements,
                                        uint32_t collection_payload_size);

/*-- Declarations (Statics and globals) --*/
uintptr_t sfd_pkg_start_location;
size_t sfd_pkg_available_size;
uint32_t sfd_pkg_used_size;
uint32_t sfd_next_record_number;
uint32_t sfd_package_number = 1;
/*------------- Functions ----------------*/

void in_band_tlm_cmpnt_begin_sensor_fifo_dbg_collection(void)
{
    ddr_manager_allocate_mem_for_snsr_fifo_dbg_pkg(&sfd_pkg_start_location, &sfd_pkg_available_size);

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;
    package_create_populate_hdr(package_hdr);
    package_hdr->payload_header.package_number = sfd_package_number++;

    sfd_pkg_used_size = sizeof(telemetry_package_hdr_t);
    sfd_next_record_number = 1;
}

void in_band_tlm_cmpnt_end_sensor_fifo_dbg_collection(void)
{
    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;
    package_hdr->payload_header.number_of_records = sfd_next_record_number - 1;
    package_hdr->payload_header.package_payload_size = sfd_pkg_used_size - sizeof(telemetry_package_hdr_t);

    mts_manager_queue_tlm_package(sfd_pkg_start_location, sfd_pkg_used_size);
}

void in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data(void)
{
    bool is_empty[SENSOR_FIFO_MAX_ID] = {false};
    sensor_fifo_svc_is_empty(&is_empty);

    bool all_empty = true;
    for (uint16_t fifo_id = 0; fifo_id < SENSOR_FIFO_MAX_ID; fifo_id++)
    {
        if (fifo_id == SENSOR_FIFO_SCP_MSG_TELEMETRY_HW)
        {
            // not reading from scp msg as it is needed by power control loop
            continue;
        }

        if (!is_empty[fifo_id])
        {
            all_empty = false;
            break;
        }
    }

    if (all_empty)
    {
        // Exit early if no FIFOs have data
        return;
    }

    if (sfd_pkg_available_size - sfd_pkg_used_size < MAX_RECORD_RESERVATION_SIZE)
    {
        // check for space for max fifo record size rather then check every addition
        // Not enough space for a max record, so end the current package
        in_band_tlm_cmpnt_end_sensor_fifo_dbg_collection();
        exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_DISABLED);
        return;
    }

    uint32_t num_collections = 0;
    p_telemetry_record_hdr_t record_hdr = (p_telemetry_record_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);
    sfd_pkg_used_size += sizeof(telemetry_record_hdr_t);
    uint32_t record_payload_start = sfd_pkg_used_size;

    if (!is_empty[SENSOR_FIFO_PSTATE_TELEMETRY_HW])
    {
        add_pstate_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW])
    {
        add_tile_temperature_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW])
    {
        add_tile_voltage_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW])
    {
        add_core_current_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_PVT_TEMP_FW])
    {
        add_soc_pvt_temperature_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_PVT_VOLTAGE_FW])
    {
        add_soc_pvt_voltage_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_DIMM_TEMP_FW])
    {
        add_dimm_info_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_VR_TEMP_FW])
    {
        add_vr_temperature_collection();
        num_collections++;
    }
    if (!is_empty[SENSOR_FIFO_VR_CURRENT_FW])
    {
        add_vr_current_collection();
        num_collections++;
    }

    populate_record_hdr(record_hdr, num_collections, sfd_pkg_used_size - record_payload_start);
}

static void populate_record_hdr(p_telemetry_record_hdr_t header, uint32_t number_of_collections, uint32_t record_payload_size)
{
    header->timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    header->record_number = sfd_next_record_number++;
    header->number_of_collections = number_of_collections;
    header->record_payload_size = record_payload_size;
}

static void populate_pwr_collection_hdr(p_telemetry_collection_hdr_t header,
                                        uint16_t collection_id,
                                        uint16_t number_of_elements,
                                        uint32_t collection_payload_size)
{
    header->provider_id = UINT16_MAX;
    header->element_id = UINT16_MAX;
    header->collection_id = collection_id;
    header->number_of_elements = number_of_elements;
    header->collection_payload_size = collection_payload_size;
}

void add_pstate_collection(void)
{
    sensor_ram_poll_status_t status;

    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    pstate_telem_t* state_data = (pstate_telem_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_core_pstate(state_data);
        if (status.curr_data_is_valid == true)
        {
            state_data++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)state_data - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(pstate_telem_t);
    sfd_pkg_used_size += num_elements * sizeof(pstate_telem_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_PSTATE_TELEMETRY_HW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_tile_temperature_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    tile_temp_t* temperature_data = (tile_temp_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_tile_temperature(temperature_data, NULL);
        if (status.curr_data_is_valid == true)
        {
            temperature_data++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)temperature_data - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(tile_temp_t);
    sfd_pkg_used_size += num_elements * sizeof(tile_temp_t);

    populate_pwr_collection_hdr(collection_hdr,
                                SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW,
                                num_elements,
                                sfd_pkg_used_size - collection_payload_start);
}

void add_tile_voltage_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    tile_voltage_t* voltage_data = (tile_voltage_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_tile_voltage(voltage_data, NULL);
        if (status.curr_data_is_valid == true)
        {
            voltage_data++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)voltage_data - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(tile_voltage_t);
    sfd_pkg_used_size += num_elements * sizeof(tile_voltage_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_core_current_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    core_current_t* current_data = (core_current_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_core_current(current_data, NULL);
        if (status.curr_data_is_valid == true)
        {
            current_data++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)current_data - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(core_current_t);
    sfd_pkg_used_size += num_elements * sizeof(core_current_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_soc_pvt_temperature_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    soc_pvt_temp_t* pvt_temperature = (soc_pvt_temp_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_soc_pvt_temperature(pvt_temperature);
        if (status.curr_data_is_valid == true)
        {
            pvt_temperature++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)pvt_temperature - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(soc_pvt_temp_t);
    sfd_pkg_used_size += num_elements * sizeof(soc_pvt_temp_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_PVT_TEMP_FW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_soc_pvt_voltage_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    soc_pvt_voltage_t* pvt_voltage = (soc_pvt_voltage_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_soc_pvt_voltage(pvt_voltage);
        if (status.curr_data_is_valid == true)
        {
            pvt_voltage++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)pvt_voltage - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(soc_pvt_voltage_t);
    sfd_pkg_used_size += num_elements * sizeof(soc_pvt_voltage_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_PVT_VOLTAGE_FW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_dimm_info_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    sensor_ram_dimm_info_t* dimm_info = (sensor_ram_dimm_info_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_dimm_info(dimm_info);
        if (status.curr_data_is_valid == true)
        {
            dimm_info++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)dimm_info - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(sensor_ram_dimm_info_t);
    sfd_pkg_used_size += num_elements * sizeof(sensor_ram_dimm_info_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_DIMM_TEMP_FW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_vr_temperature_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    vr_temp_t* vr_temperature = (vr_temp_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_vr_temperature(vr_temperature);
        if (status.curr_data_is_valid == true)
        {
            vr_temperature++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)vr_temperature - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(vr_temp_t);
    sfd_pkg_used_size += num_elements * sizeof(vr_temp_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_VR_TEMP_FW, num_elements, sfd_pkg_used_size - collection_payload_start);
}

void add_vr_current_collection(void)
{
    sensor_ram_poll_status_t status;
    p_telemetry_collection_hdr_t collection_hdr =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    sfd_pkg_used_size += sizeof(telemetry_collection_hdr_t);
    uint32_t collection_payload_start = sfd_pkg_used_size;

    vr_current_t* vr_current = (vr_current_t*)(sfd_pkg_start_location + sfd_pkg_used_size);
    do
    {
        status = sensor_fifo_svc_poll_vr_current(vr_current);
        if (status.curr_data_is_valid == true)
        {
            vr_current++;
        }
    } while (status.more_entries == true);

    uint16_t num_elements =
        (uint16_t)((uintptr_t)vr_current - (uintptr_t)(sfd_pkg_start_location + collection_payload_start)) /
        sizeof(vr_current_t);
    sfd_pkg_used_size += num_elements * sizeof(vr_current_t);

    populate_pwr_collection_hdr(collection_hdr, SENSOR_FIFO_VR_CURRENT_FW, num_elements, sfd_pkg_used_size - collection_payload_start);
}