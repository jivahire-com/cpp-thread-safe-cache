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
#include "ddr_manager_bwl.h"
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <ddr_manager_events.h>
#include <fpfw_cfg_mgr.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void init_thresholds(ddr_dimm_temp_thresholds_t* thresholds);
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
        // TODO: spam on SVP
        //  task https://azurecsi.visualstudio.com/Dev/_workitems/edit/2199194
        // DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_PLATFORM_NOT_SUPPORTED, ET_NOPARAM);
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
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_0, dimm_idx);
        }

        ddr_manager_i3c_temperature_t ts1_temp;
        if (ddr_manager_temperature_sensor_read(dimm_idx, 1, &ts1_temp) == DDR_MANAGER_I3C_SUCCESS)
        {
            ddr_telemetry_update_dimm_temp(dimm_idx, 1, ts0_temp);
        }
        else
        {
            DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_1, dimm_idx);
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
    static bool is_first_run = true;
    static ddr_dimm_temp_thresholds_t thresholds = {};

    if (is_first_run)
    {
        init_thresholds(&thresholds);
        is_first_run = false;
    }

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

        if ((ts0_temp.is_positive && ts0_temp.as_uint16 > thresholds.crit) ||
            (ts1_temp.is_positive && ts1_temp.as_uint16 > thresholds.crit))
        {
            DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD, dimm_idx);

            // Blow things up
            ddr_manager_set_thermal_trip_gpio();

            // And BUG_CHECK here?
        }
    }

    // Engage BWL if the max DIMM temperature exceeds the high threshold
    if (max_dimm_temp > thresholds.high)
    {
        if (config_get_ddrmanager_bwl_en())
        {
            ddr_manager_enable_bwl_i3c();
        }
        else
        {
            // May want to do something else here, like log an event
            DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_DIMM_TEMPERATURES_EXCEED_HIGH_THRESHOLD_BWL_DISABLE);
        }
    }

    // Disengage BWL if the max DIMM temperature falls below the low threshold
    if (max_dimm_temp < thresholds.low)
    {
        ddr_manager_disable_bwl_i3c();
    }
}

void init_thresholds(ddr_dimm_temp_thresholds_t* thresholds)
{
    thresholds->low = config_manager_get_ddr_dimm_temp_threshold_low();
    thresholds->high = config_manager_get_ddr_dimm_temp_threshold_high();
    thresholds->crit = config_manager_get_ddr_dimm_temp_threshold_critical();
}