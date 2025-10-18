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
static ddr_manager_i3c_temperature_t dimm_temps[NUM_DIMM_PER_DIE][NUM_DIMM_TEMP_SENSORS] = {0};
static uint16_t dimm_powers[NUM_DIMM_PER_DIE] = {0};
static TX_MUTEX ddr_telemetry_mutex;

// BWL residency tracking
static uint64_t bwl_residency_ticks = 0;
static uint16_t throttle_count = 0; // number of times BWL has engaged in this telemetry period
static uint64_t last_gtimer_count = 0;
static TX_MUTEX bwl_residency_mutex;

/*------------- Functions ----------------*/
void ddr_init_telemetry()
{
    BUG_ASSERT(tx_mutex_create(&ddr_telemetry_mutex, "ddr_telemetry_mutex", TX_INHERIT) == TX_SUCCESS);
    BUG_ASSERT(tx_mutex_create(&bwl_residency_mutex, "ddr_bwl_residency_mutex", TX_INHERIT) == TX_SUCCESS);
}

void ddr_telemetry_lock()
{
    tx_mutex_get(&ddr_telemetry_mutex, TX_WAIT_FOREVER);
}

void ddr_telemetry_unlock()
{
    tx_mutex_put(&ddr_telemetry_mutex);
}

void ddr_bwl_residency_lock()
{
    tx_mutex_get(&bwl_residency_mutex, TX_WAIT_FOREVER);
}

void ddr_bwl_residency_unlock()
{
    tx_mutex_put(&bwl_residency_mutex);
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

void ddr_telemetry_update_dimm_power(uint8_t dimm_idx, uint16_t power_mW)
{
    BUG_ASSERT(dimm_idx < NUM_DIMM_PER_DIE);

    ddr_telemetry_lock();
    dimm_powers[dimm_idx] = power_mW;
    ddr_telemetry_unlock();
}

uint16_t ddr_telemetry_get_dimm_power(uint8_t dimm_idx)
{
    BUG_ASSERT(dimm_idx < NUM_DIMM_PER_DIE);

    uint16_t power_mW;
    ddr_telemetry_lock();
    power_mW = dimm_powers[dimm_idx];
    ddr_telemetry_unlock();

    return power_mW;
}

void ddr_telemetry_increment_throttle_count()
{
    ddr_bwl_residency_lock();
    throttle_count == UINT16_MAX ? throttle_count = UINT16_MAX : throttle_count++;
    ddr_bwl_residency_unlock();
}

uint16_t ddr_telemetry_get_throttle_count()
{
    return throttle_count;
}

void ddr_telemetry_reset_throttle_count()
{
    ddr_bwl_residency_lock();
    throttle_count = 0;
    ddr_bwl_residency_unlock();
}

void ddr_bwl_residency_add_ticks(uint32_t ticks)
{
    ddr_bwl_residency_lock();
    bwl_residency_ticks += ticks;
    ddr_bwl_residency_unlock();
}

uint32_t ddr_bwl_residency_get_ticks()
{
    uint32_t ticks;

    ddr_bwl_residency_lock();
    ticks = bwl_residency_ticks;
    ddr_bwl_residency_unlock();

    return ticks;
}

void ddr_bwl_residency_reset()
{
    ddr_bwl_residency_lock();
    bwl_residency_ticks = 0;
    ddr_bwl_residency_unlock();
}

uint16_t gtimer_ticks_to_ms(uint32_t ticks)
{
    const uint32_t ticks_per_ms = gtimer_prodfw_get_frequency() / 1000;

    // Avoid divide by zero
    if (ticks_per_ms == 0)
    {
        return 0;
    }

    // Convert ticks to milliseconds
    return (uint16_t)(ticks / ticks_per_ms);
}

void set_last_gtimer_count(uint64_t count)
{
    ddr_bwl_residency_lock();
    last_gtimer_count = count;
    ddr_bwl_residency_unlock();
}

uint64_t get_last_gtimer_count(void)
{
    uint64_t count;

    ddr_bwl_residency_lock();
    count = last_gtimer_count;
    ddr_bwl_residency_unlock();

    return count;
}

void ddr_telemetry_report()
{
    sensor_ram_dimm_info_t dimm_info = {0};
    uint16_t power_mW = 0;

    dimm_info.timestamp = gtimer_prodfw_get_counter();
    dimm_info.dimm_throttling = ddr_manager_get_bwl_state();
    dimm_info.dimm_throttle_count = ddr_telemetry_get_throttle_count();
    dimm_info.dimm_memory_frequency_id = (uint8_t)config_get_ddr_speed_grade();

    // Report BWL residency time in ms
    uint32_t residency_ticks = ddr_bwl_residency_get_ticks();
    if (dimm_info.dimm_throttling != DIMM_THROTTLE_SOURCE_NONE)
    {
        // If BWL is currently engaged, add the time since last engagement to the residency counter
        uint64_t current_gtimer_count = gtimer_prodfw_get_counter();

        ddr_bwl_residency_lock();
        if ((current_gtimer_count < last_gtimer_count) || (last_gtimer_count == 0))
        {
            last_gtimer_count = current_gtimer_count;
        }
        uint32_t delta_ticks = (uint32_t)(current_gtimer_count - last_gtimer_count);
        residency_ticks += delta_ticks;

        // Update the last_gtimer_count for next calculation
        last_gtimer_count = current_gtimer_count;
        ddr_bwl_residency_unlock();
    }
    else
    {
        last_gtimer_count = 0; // Reset the last_gtimer_count when BWL is not engaged
    }

    uint16_t residency_ms = gtimer_ticks_to_ms(residency_ticks);

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        dimm_info.dimm_id = dimm_idx;
        if (ddr_manager_power_mw_read(dimm_idx, &power_mW) != DDR_MANAGER_I3C_SUCCESS)
        {
            DDR_LOG_WARN("Error reading DIMM %d power\n", dimm_idx);
            power_mW = 0;
        };
        dimm_info.dimm_power_mW = power_mW;

        ddr_manager_i3c_temperature_t temp0 = ddr_telemetry_get_dimm_temp(dimm_idx, 0);
        // Franctional temperatures may be {0,25,50,75}
        dimm_info.dimm_temp_s0_dC = (10 * temp0.temp_int) + (temp0.temp_frac + 5) / 10;

        ddr_manager_i3c_temperature_t temp1 = ddr_telemetry_get_dimm_temp(dimm_idx, 1);
        // Franctional temperatures may be {0,25,50,75}
        dimm_info.dimm_temp_s1_dC = (10 * temp1.temp_int) + (temp1.temp_frac + 5) / 10;

        dimm_info.dimm_throttle_duration_ms = (uint8_t)residency_ms;

        // Send the telemetry data to the telemetry service
        sensor_fifo_svc_add_dimm_info(&dimm_info);
    }

    // Reset BWL residency counter after reporting
    ddr_bwl_residency_reset();

    // Reset throttle count after reporting
    ddr_telemetry_reset_throttle_count();
}