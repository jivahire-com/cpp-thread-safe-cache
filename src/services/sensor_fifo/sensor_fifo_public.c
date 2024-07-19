//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_public.c
 * Implementation of the sensor fifo service public API's.
 */

/*------------- Includes -----------------*/
#include "device_fifo_id.h"               // for DEVICE_FIFO_PVT_TEMP_TLM_F...
#include "fpfw_status.h"                  // for FPFW_STATUS_SUCCEEDED, fpf...
#include "sensor_fifo_driver_interface.h" // for sensor_fifo_driver_read_entry
#include "sensor_fifo_service.h"          // for QUADWORD_SIZE, sensor_ram_...
#include "sensor_fifo_service_i.h"        // for pSensor_fifo_driver_inf

#include <assert.h>  // IWYU pragma: keep for static_assert
#include <stdbool.h> // for false, true
#include <stdint.h>  // for uint16_t, uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
// Firmware writes the following entries to fifo buffers. The hardware as a constraint in that writes have to
// be in multiples of QUADWORDS, or 8 bytes.  These asserts will enforce the entries are correctly sized with
// padding bytes. Without out padding, garbage data following the structure would be copied into the telemetry
// buffer. Or the service would have to double buffer the incoming entry to the QUADWORD multiple which is a
// waste of runtime.
static_assert((sizeof(core_current_t) % QUADWORD_SIZE) == 0, "core_current_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(tile_temp_t) % QUADWORD_SIZE) == 0, "tile_temp_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(tile_voltage_t) % QUADWORD_SIZE) == 0, "tile_voltage_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(soc_pvt_temp_t) % QUADWORD_SIZE) == 0, "soc_pvt_temp_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(soc_pvt_voltage_t) % QUADWORD_SIZE) == 0, "soc_pvt_voltage_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(vr_temp_t) % QUADWORD_SIZE) == 0, "vr_temp_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(vr_current_t) % QUADWORD_SIZE) == 0, "vr_current_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(sensor_ram_dimm_info_t) % QUADWORD_SIZE) == 0,
              "sensor_ram_dimm_info_t needs to be multiple of QUADWORD_SIZE");

sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index)
{
    sensor_ram_poll_status_t retVal = {0};

    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;

    fpfw_status_t status = sensor_fifo_driver_read_entry(
        pSensor_fifo_driver_inf,
        pSensor_fifo_driver_inf->device->fifo_property_table[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW].device_fifo_id,
        sizeof(tile_temp_t),
        (uint8_t*)temperature_data,
        &num_entries_read,
        &num_entries_remaining,
        tile_index);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        retVal.curr_data_is_valid = num_entries_read > 0 ? true : false;
        retVal.more_entries = num_entries_remaining > 0 ? true : false;
    }

    // no event trace on poll failures due to polling rate can overrun even trace
    // health status is used to track error rate

    return retVal;
}

sensor_ram_poll_status_t sensor_fifo_svc_poll_scp_message(plimit_msg_telem_t* plimit_msg)
{
    sensor_ram_poll_status_t retVal = {0};

    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index = 0; // not used

    fpfw_status_t status = sensor_fifo_driver_read_entry(
        pSensor_fifo_driver_inf,
        pSensor_fifo_driver_inf->device->fifo_property_table[DEVICE_FIFO_SCP_MSG_TLM_HW_PROD].device_fifo_id,
        sizeof(plimit_msg_telem_t),
        (uint8_t*)plimit_msg,
        &num_entries_read,
        &num_entries_remaining,
        &stride_index);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        retVal.curr_data_is_valid = num_entries_read > 0 ? true : false;
        retVal.more_entries = num_entries_remaining > 0 ? true : false;
    }

    // no event trace on poll failures due to polling rate can overrun even trace
    // health status is used to track error rate

    return retVal;
}

void sensor_fifo_svc_add_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{
    sensor_fifo_driver_write_entries(
        pSensor_fifo_driver_inf,
        pSensor_fifo_driver_inf->device->fifo_property_table[DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD].device_fifo_id,
        (uint8_t*)pvt_temperature,
        sizeof(soc_pvt_temp_t),
        1,
        0);
}

sensor_ram_poll_status_t sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{
    sensor_ram_poll_status_t retVal = {0};

    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index = 0; // not used

    fpfw_status_t status = sensor_fifo_driver_read_entry(
        pSensor_fifo_driver_inf,
        pSensor_fifo_driver_inf->device->fifo_property_table[DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD].device_fifo_id,
        sizeof(soc_pvt_temp_t),
        (uint8_t*)pvt_temperature,
        &num_entries_read,
        &num_entries_remaining,
        &stride_index);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
        retVal.curr_data_is_valid = num_entries_read > 0 ? true : false;
        retVal.more_entries = num_entries_remaining > 0 ? true : false;
    }

    // no event trace on poll failures due to polling rate can overrun even trace
    // health status is used to track error rate

    return retVal;
}