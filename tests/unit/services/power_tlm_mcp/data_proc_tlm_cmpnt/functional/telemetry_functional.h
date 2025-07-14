//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_functional.h
 * This file contains the definitions of structures used for packages for in band telemetry.
 */

#pragma once

/*----------- Nested includes ------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <data_proc_tlm_cmpnt.h>
#include <sensor_fifo_service.h>
#include <event_trace_providers.h>
#include <telemetry_package_defs.h>
#include <data_sampling_i.h>
#include <package_creation_i.h>
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    uint16_t min;
    uint16_t max;
    uint16_t avg;
    uint16_t count;
} stats_t;

/*-- Declarations (Statics and globals) --*/

extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;


/*--------- Function Prototypes ----------*/

// Wrapper declarations for sensor services
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature);
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info);

void reset_pwr_tlm_data(void);
void update_stats(stats_t* stats, uint16_t latest_value);
extern tile_temp_t g_stored_temperatures[];

// Mock function declarations
uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void);

#ifdef __cplusplus
}
#endif


