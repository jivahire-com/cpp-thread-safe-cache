//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i.h
 * DDR Manager private API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <tx_api.h>
#include "ddr_manager_i3c.h"

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_WORK_QUEUE_NAME ("ddr-work-queue")
#define DDR_WORK_THREAD_NAME ("ddr-work_thread")
#define DDR_TIMER_NAME ("ddr-timer")
#define NUM_DIMM  (6) // Each die will address 6 DIMMs
#define NUM_DIMM_TEMP_SENSORS (2) // Each DIMM will have 2 temperature sensors

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
void ddr_worker_thread_func(ULONG pddr_service_ctx);
void ddr_timer_cb(ULONG pddr_service_ctx);

/**
 * @brief Initializes DDR telemetry.
 * 
 * This function initializes the DDR telemetry module.
 */
void ddr_init_telemetry(void);

/**
 * @brief Polls the DIMMs for telemetry data.
 * 
 * This function polls the DIMMs to retrieve telemetry data.
 */
void ddr_poll_dimms(void);

/**
 * @brief Updates the telemetry snapshot for DIMM temps
 * 
 * @param dimm_idx The index of the DIMM.
 * @param ts_idx The index of the temperature sensor.
 * @param temp The temperature value to update.
 */
void ddr_telemetry_update_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx, ddr_manager_i3c_temperature_t temp);

/**
 * @brief Retrieves the temperature of a DIMM from the telemetry snapshot.
 * 
 * @param dimm_idx The index of the DIMM.
 * @param ts_idx The index of the temperature sensor.
 * @return The temperature of the DIMM.
 */
ddr_manager_i3c_temperature_t ddr_telemetry_get_dimm_temp(uint8_t dimm_idx, uint8_t ts_idx);

void ddr_create_memory_map(void);
void ddr_process_i3c_data(void);
void ddr_create_bdat(void);
void ddr_create_smbios_tables(void);
void ddr_create_smbios_type_16(void);
void ddr_create_smbios_type_17(void);
void copy_type_16_to_ddr(void);
void copy_type_17_to_ddr(void);

uint16_t config_manager_get_ddr_dimm_temp_threshold_low();
uint16_t config_manager_get_ddr_dimm_temp_threshold_high();
uint16_t config_manager_get_ddr_dimm_temp_threshold_critical();

/**
 * Sets the thermal trip GPIO for the DDR manager.
 */
void ddr_manager_set_thermal_trip_gpio();

/**
 * Engages the bandwidth limiter for the DDR manager.
 */
void ddr_manager_engage_bwl();

/**
 * Disengages the bandwidth limiter for the DDR manager.
 */
void ddr_manager_disengage_bwl();

/**
 * Checks if polling is supported on the platform for the DDR manager.
 *
 * @return true if polling is supported, false otherwise.
 */
bool ddr_manager_platform_is_polling_supported();