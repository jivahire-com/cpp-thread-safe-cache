//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implementation of the sensor fifo service public API's.
 */

/*------------- Includes -----------------*/

#include "sensor_fifo_driver_interface.h" // for sensor_fifo_driver_interfa...
#include "sensor_fifo_service.h"          // for sensor_ram_poll_status_t
#include "telemetry_data_struct.h"        // IWYU pragma: keep

#include <DfwkClient.h> // for DfwkClientInterfaceOpen
#include <assert.h>     // IWYU pragma: keep
#include <stdint.h>     // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sensor_fifo_driver_interface_t* s_pSensor_fifo_driver_inf;
/*------------- Functions ----------------*/
// Firmware writes the following entries to fifo buffers. The hardware as a constraint in that writes have to
// be in multiples of QUADWORDS, or 8 bytes.  These asserts will enforce the entries are correctly sized with
// padding bytes. Without out padding, garbage data following the structure would be copied into the telemetry
// buffer. Or the service would have to double buffer the incoming entry to the QUADWORD multiple which is a
// waste of runtime.
static_assert((sizeof(soc_pvt_temp_t) % QUADWORD_SIZE) == 0, "soc_pvt_temp_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(soc_pvt_voltage_t) % QUADWORD_SIZE) == 0, "soc_pvt_voltage_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(vr_temp_t) % QUADWORD_SIZE) == 0, "vr_temp_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(vr_current_t) % QUADWORD_SIZE) == 0, "vr_current_t needs to be multiple of QUADWORD_SIZE");
static_assert((sizeof(sensor_ram_dimm_info_t) % QUADWORD_SIZE) == 0,
              "sensor_ram_dimm_info_t needs to be multiple of QUADWORD_SIZE");

void sensor_fifo_svc_initialize(sensor_fifo_driver_interface_t* driver_interface)
{
    s_pSensor_fifo_driver_inf = driver_interface;

    DfwkClientInterfaceOpen(&driver_interface->base_interface);
}

sensor_ram_poll_status_t sensor_fifo_svc_poll_tile_temperature(sensor_telem_t* temperature_data, uint8_t* tile_index)
{
    (void)temperature_data;
    (void)tile_index;

    sensor_ram_poll_status_t retVal = {0};

    return retVal;
}