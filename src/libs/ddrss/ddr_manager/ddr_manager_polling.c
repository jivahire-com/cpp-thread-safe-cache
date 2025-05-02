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
 *  BWL = Bandwidth Limiting - this is a throttling mechanism implemented in the DDR controller
 *
 * New for KNG: Each die will have 2 I3C controllers, addressing 3 DIMMs each
 */

/*------------- Includes -----------------*/
#include "ddr_manager_bwl.h"
#include "ddr_manager_i.h"
#include "ddr_manager_i3c.h"

#include <bug_check.h>
#include <ddr_manager_events.h>
#include <fpfw_cfg_mgr.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void init_thresholds(ddr_dimm_temp_thresholds_t* thresholds);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void ddr_poll_dimms()
{
    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
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
}

// Checks cached DIMM temperatures against thresholds initially read from config knobs
// Thresholds may be overridden via CLI at runtime (ddr_bwl command) but will not affect config knobs values
// Above 'high' -> Engage BWL - unless bwl_force_disabled == true
// Below 'low'  -> If BWL engaged, disengage - unless bwl_force_enabled == true
// Above 'crit' -> Blow things up?  Task 2584104: Determine correct behavior when DDR exceeds critical temperature
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
    for (int dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        ddr_manager_i3c_temperature_t ts0_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 0);
        ddr_manager_i3c_temperature_t ts1_temp = ddr_telemetry_get_dimm_temp(dimm_idx, 1);

        // Set the max_dimm_temp, accounting for negative temperatures
        if (ts0_temp.is_positive)
        {
            max_dimm_temp = ts0_temp.temp_int > max_dimm_temp ? ts0_temp.temp_int : max_dimm_temp;
        }

        if (ts1_temp.is_positive)
        {
            max_dimm_temp = ts1_temp.temp_int > max_dimm_temp ? ts1_temp.temp_int : max_dimm_temp;
        }

        if (!ts0_temp.is_positive && !ts1_temp.is_positive)
        {
            // If both temps are negative, set the max to 0
            max_dimm_temp = 0;
        }

        if ((ts0_temp.is_positive && ts0_temp.temp_int > thresholds.crit) ||
            (ts1_temp.is_positive && ts1_temp.temp_int > thresholds.crit))
        {
            DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD, dimm_idx);

            // Blow things up
            ddr_manager_set_thermal_trip_gpio();

            // And BUG_CHECK here?
            // Task 2584104: Determine correct behavior when DDR exceeds critical temperature
        }
    }

    // Engage BWL if the max DIMM temperature exceeds the high threshold
    // BWL = Bandwidth Limiting - this is a throttling mechanism implemented in the DDR controller
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
    thresholds->low = config_get_ddrmanager_dimm_temp_low();
    thresholds->high = config_get_ddrmanager_dimm_temp_high();
    thresholds->crit = config_get_ddrmanager_dimm_temp_crit();
}