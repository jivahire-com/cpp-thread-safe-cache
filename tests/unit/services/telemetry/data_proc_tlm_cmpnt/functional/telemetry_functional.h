/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file telemetry_functional.h
 *
 */

#pragma once

#ifndef TELEMETRY_FUNCTIONAL_H
#define TELEMETRY_FUNCTIONAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <data_proc_tlm_cmpnt.h>
#include <sensor_fifo_service.h>
#include <event_trace_providers.h>
#include <telemetry_package_defs.h>
#include <tlm_logger_i.h>
#include <package_creation_i.h>


// External declarations for runtime info structures
extern core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_info;


// Wrapper declarations for sensor services
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info);


// Test helper functions
void reset_pwr_tlm_data(void);
extern tile_temp_t g_stored_temperatures[];

// Mock function declarations
uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void);

#ifdef __cplusplus
}
#endif

#endif
