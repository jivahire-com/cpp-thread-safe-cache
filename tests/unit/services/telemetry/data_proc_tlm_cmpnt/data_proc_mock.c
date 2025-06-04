//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_mock.c
 * Provide mock functions for icc mhu transport driver tests
 */

/*------------- Includes -----------------*/
#include "telemetry_ut.h"

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_sampling_i.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t

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

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{
    FPFW_UNUSED(pvt_temperature);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info)
{
    FPFW_UNUSED(dimm_info);
    return *(sensor_ram_poll_status_t*)mock();
}

fpfw_status_t __wrap_platform_power_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    check_expected(count);
    check_expected_ptr(dts_coeff);
    return mock_type(int32_t);
}
void __wrap_comp_metrics_for_sample_period(void)
{
}
uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void)
{
    return mock_type(int64_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

bool __wrap_in_band_tlm_cmpnt_is_inst_record_enabled(uint32_t inst_record_id)
{
    FPFW_UNUSED(inst_record_id);
    return mock_type(bool);
}

void __wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg(void)
{
    function_called();
}
