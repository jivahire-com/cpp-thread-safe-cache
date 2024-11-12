//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_mock.c
 * Provide mock functions for icc mhu transport driver tests
 */

/*------------- Includes -----------------*/
#include "telemetry_ut.h"

#include <FpFwCMocka.h>          // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
#include <tlm_logger_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index)
{
    FPFW_UNUSED(temperature_data);
    FPFW_UNUSED(tile_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index)
{
    FPFW_UNUSED(voltage_data);
    FPFW_UNUSED(tile_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index)
{
    FPFW_UNUSED(current_data);
    FPFW_UNUSED(core_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data)
{
    FPFW_UNUSED(state_data);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature)
{
    FPFW_UNUSED(vr_temperature);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current)
{
    FPFW_UNUSED(vr_current);
    return *(sensor_ram_poll_status_t*)mock();
}
