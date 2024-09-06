//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_logger_i.h
 *  Private header
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_BUFFER_ENTRIES 		 8
#define MAX_NUMBER_POWER_SAMPLE  100
#define DOUT2MILLIVOLTS(voltage) (voltage * 1000)


// The current conversion factor is set by default as 26.5 per bit.
#ifndef CORE_CURRENT_CONVERSION_FACTOR
    #define CORE_CURRENT_CONVERSION_FACTOR 26.5F
#endif


/*----------- Nested includes ------------*/

#include <fpfw_status.h>
#include <sensor_fifo_service.h> // for sensor ram data structures
#include <stdint.h>


/*--------- Function Prototypes ----------*/


/**
 * @brief Internal API to log tile/core/hnf temperature parameters
 *
 * @param temperature_data - SCF RAM formatted resource for temperature packets
 * @param tile_index - index to the tile id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_tile_temperature(tile_temp_t* temperature_data, uint8_t tile_index);

/**
 * @brief Internal API to log tile/core voltages
 *
 * @param voltage_data - SCF RAM formatted resource for voltage packets
 * @param tile_index - index to the tile id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_tile_voltage(tile_voltage_t* voltage_data, uint8_t tile_index);

/**
 * @brief Internal API to log core currents and power
 *
 * @param current_data - SCF RAM formatted resource for core current packets
 * @param core_index - index to the core id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_core_current(core_current_t* current_data, uint8_t core_index);

/**
 * @brief helper function to update the pstate runtime timestamp
 *
 * @param core_id - core that is referenced to that owns this timestamp
 * @param pstate - pstate that is reference to where it needs to be updated
 * @param timestamp - timestamp used for the update
 *
 * @return fpfw_status_t
 */
fpfw_status_t update_core_pstate_timestamps(uint8_t core_id, uint8_t pstate, uint64_t timestamp);

/**
 * @brief Internal API to log voltage regulator (VR) temperatures
 *
 * @param vr_temperature - SCF RAM formatted resource for VR Temperatures
 *
 * @return None
 */
void tlm_logger_log_vr_temp(vr_temp_t* vr_temperature);

/**
 * @brief Internal API to log voltage regulator (VR) current and voltage
 *
 * @param vr_current - SCF RAM formatted resource for VR Currents (and voltages)
 *
 * @return None
 */
void tlm_logger_log_vr_current(vr_current_t* vr_current);

/**
 * @brief Internal API to log pstate telemetry packets, for pstate, cstate and throttling info
 *
 * @param pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        NOTE: The Pstate packet contains the core id reference internally.
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_core_pstate(pstate_telem_t* pstate_telemetry);


#ifdef __cplusplus
}
#endif