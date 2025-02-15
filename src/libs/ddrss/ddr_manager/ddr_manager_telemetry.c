//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_telemetry.c
 * Telemetry reporting
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <bug_check.h>
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