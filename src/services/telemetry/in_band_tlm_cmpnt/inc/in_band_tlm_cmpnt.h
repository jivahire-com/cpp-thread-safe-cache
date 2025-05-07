//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt.h
 * Internal public interface for the in-band telemetry component
 */

#pragma once

/*----------- Nested includes ------------*/
#include <exec_tlm_cmpnt.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the in band telemetry component.
 * param[in] die_id The die id.
 * param[in] inst_samples_per_pkg The number of samples per instantaneous package.
 */
void in_band_tlm_cmpnt_init(uint8_t die_id, uint16_t inst_samples_per_pkg);

/**
 * @brief Generate instantaneous package.
 */
void in_band_tlm_cmpnt_add_inst_sample(void);

/**
 * @brief Generate power package.
 */
void in_band_tlm_cmpnt_generate_pwr_pkg(void);

/**
 * @brief Generate 24 hour package.
 */
void in_band_tlm_cmpnt_generate_24hr_pkg(void);

/**
 * @brief Handle incoming MTS messages. Called from executive telemetry component.
 */
void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void);

/**
 * @brief Execute in-band telemetry component exit actions for the given telemetry mode.
 * @param[in] exiting_mode  current telemetry mode
 * @return None
 *
 */
void in_band_tlm_cmpnt_tlm_mode_exit_actions(tlm_operating_mode_t exiting_mode);

/**
 * @brief Execute in-band telemetry component enter actions for the given telemetry mode.
 * @param[in] entering_mode  new telemetry mode
 * @return None
 *
 */
void in_band_tlm_cmpnt_tlm_mode_enter_actions(tlm_operating_mode_t entering_mode);

/**
 * @brief Check if any instantaneous telemetry record is enabled. If not inst packages will not be generated.
 * @return true if enabled, false if disabled.
 */
bool in_band_tlm_cmpnt_is_instantaneous_enabled(void);

/**
 * @brief Collect raw sensor fifo data for debug packages.
 */
void in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data(void);