//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_telemetry.c
 * Telemetry reporting
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_bwl.h"
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <gtimer_prodfw.h>
#include <idsw_kng.h>
#include <sensor_fifo_service.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static ddr_manager_i3c_temperature_t dimm_temps[NUM_DIMM_PER_DIE][NUM_DIMM_TEMP_SENSORS];
static TX_MUTEX ddr_telemetry_mutex;

/*------------- Functions ----------------*/
void ddr_init_telemetry()
{
    BUG_ASSERT(tx_mutex_create(&ddr_telemetry_mutex, "ddr_telemetry_mutex", TX_INHERIT) == TX_SUCCESS);
}

void ddr_telemetry_lock()
{
    tx_mutex_get(&ddr_telemetry_mutex, TX_WAIT_FOREVER);
}

void ddr_telemetry_unlock()
{
    tx_mutex_put(&ddr_telemetry_mutex);
}

void ddr_telemetry_update_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx, ddr_manager_i3c_temperature_t temp)
{
    BUG_ASSERT(dimm_idx < NUM_DIMM_PER_DIE);
    BUG_ASSERT(ts_idx < NUM_DIMM_TEMP_SENSORS);

    ddr_telemetry_lock();
    dimm_temps[dimm_idx][ts_idx] = temp;
    ddr_telemetry_unlock();
}

ddr_manager_i3c_temperature_t ddr_telemetry_get_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx)
{
    BUG_ASSERT(dimm_idx < NUM_DIMM_PER_DIE);
    BUG_ASSERT(ts_idx < NUM_DIMM_TEMP_SENSORS);

    ddr_manager_i3c_temperature_t temp;
    ddr_telemetry_lock();
    temp = dimm_temps[dimm_idx][ts_idx];
    ddr_telemetry_unlock();

    return temp;
}

void ddr_telemetry_report()
{
    sensor_ram_dimm_info_t dimm_info = {0};
    uint16_t power_mW = 0;

    dimm_info.timestamp = gtimer_prodfw_get_counter();
    dimm_info.dimm_throttling = ddr_manager_get_bwl_state();
    dimm_info.dimm_memory_frequency_id = (uint8_t)config_get_ddr_speed_grade();

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        dimm_info.dimm_id = dimm_idx;
        if (ddr_manager_power_mw_read(dimm_idx, &power_mW) != DDR_MANAGER_I3C_SUCCESS)
        {
            printf("Error reading DIMM %d power\n", dimm_idx);
            power_mW = 0;
        };
        dimm_info.dimm_power_mW = power_mW;

        ddr_manager_i3c_temperature_t temp0 = ddr_telemetry_get_dimm_temp(dimm_idx, 0);
        dimm_info.dimm_temp_s0_dC = (10 * temp0.temp_int) + temp0.temp_frac;

        ddr_manager_i3c_temperature_t temp1 = ddr_telemetry_get_dimm_temp(dimm_idx, 1);
        dimm_info.dimm_temp_s1_dC = (10 * temp1.temp_int) + temp1.temp_frac;

        // Send the telemetry data to the telemetry service
        sensor_fifo_svc_add_dimm_info(&dimm_info);
    }
}