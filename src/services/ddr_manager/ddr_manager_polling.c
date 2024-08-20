//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling.c
 * This file contains the DIMM polling functionality.
 * DDR I3C calls through DFWK
 * BWL/Throttling
 * Telemetry reporting
 *
 * New for KNG: Each die will have 2 I3C controllers, addressing 3 DIMMs each
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
// Check DIMM temperatures against thresholds and engage BWL if necessary
void check_dimm_temp_thresholds();

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
// TODO: Stubs for config_manager_get_ddr_dimm_temp_threshold_low,
// config_manager_get_ddr_dimm_temp_threshold_high, config_manager_get_ddr_dimm_temp_threshold_critical
// Replace with actual implementation once config_manager is implemented ADO: #:1831724
uint16_t config_manager_get_ddr_dimm_temp_threshold_low()
{
    return 80 << 8;
}

uint16_t config_manager_get_ddr_dimm_temp_threshold_high()
{
    return 100 << 8;
}

uint16_t config_manager_get_ddr_dimm_temp_threshold_critical()
{
    return 120 << 8;
}

void ddr_poll_dimms()
{
    if (!ddr_manager_platform_is_polling_supported())
    {
        printf("DDR polling not supported on this platform, skipping\n");
        return;
    }

    for (int dimm_idx = 0; dimm_idx < NUM_DIMM; dimm_idx++)
    {
        ddr_manager_i3c_temperature_t ts0_temp;
        if (ddr_manager_temperature_sensor_read(dimm_idx, 0, &ts0_temp) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_temp(dimm_idx, 0, ts0_temp);
        }
        else
        {
            printf("Failed to read temperature sensor 0 on DIMM %d\n", dimm_idx);
        }

        ddr_manager_i3c_temperature_t ts1_temp;
        if (ddr_manager_temperature_sensor_read(dimm_idx, 1, &ts1_temp) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_temp(dimm_idx, 1, ts0_temp);
        }
        else
        {
            printf("Failed to read temperature sensor 1 on DIMM %d\n", dimm_idx);
        }
    }

    check_dimm_temp_thresholds();
}

// Checks cached DIMM temperatures against thresholds initially read from config knobs
// Thresholds may be overridden via CLI at runtime (ddr_bwl command) but will not affect config knobs values
// Above 'high' -> Engage BWL - unless bwl_force_disabled == true
// Below 'low'  -> If BWL engaged, disengage - unless bwl_force_enabled == true
// Above 'crit' -> Blow things up
void check_dimm_temp_thresholds()
{
    uint16_t temp_threshold_low = config_manager_get_ddr_dimm_temp_threshold_low();
    uint16_t temp_threshold_high = config_manager_get_ddr_dimm_temp_threshold_high();
    uint16_t temp_threshold_critical = config_manager_get_ddr_dimm_temp_threshold_critical();

    uint16_t max_dimm_temp = 0;
    for (int dimm_idx = 0; dimm_idx < NUM_DIMM; dimm_idx++)
    {
        ddr_manager_i3c_temperature_t ts0_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 0);
        ddr_manager_i3c_temperature_t ts1_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 1);

        // Set the max_dimm_temp, accounting for negative temperatures
        if (ts0_temp.is_positive)
        {
            max_dimm_temp = ts0_temp.as_uint16 > max_dimm_temp ? ts0_temp.as_uint16 : max_dimm_temp;
        }
        else if (ts1_temp.is_positive)
        {
            max_dimm_temp = ts1_temp.as_uint16 > max_dimm_temp ? ts1_temp.as_uint16 : max_dimm_temp;
        }
        else if (!ts0_temp.is_positive && !ts1_temp.is_positive)
        {
            // If both temps are negative, set the max to 0
            max_dimm_temp = 0;
        }

        if ((ts0_temp.is_positive && ts0_temp.as_uint16 > temp_threshold_critical) ||
            (ts0_temp.is_positive && ts1_temp.as_uint16 > temp_threshold_critical))
        {
            printf("DIMM %d has exceeded critical temperature threshold\n", dimm_idx);

            // Blow things up
            ddr_manager_set_thermal_trip_gpio();

            // And BUG_CHECK here?
        }
    }

    // Engage BWL if the max DIMM temperature exceeds the high threshold
    if (max_dimm_temp > temp_threshold_high)
    {
        ddr_manager_engage_bwl();
    }

    // Disengage BWL if the max DIMM temperature falls below the low threshold
    if (max_dimm_temp < temp_threshold_low)
    {
        ddr_manager_disengage_bwl();
    }
}