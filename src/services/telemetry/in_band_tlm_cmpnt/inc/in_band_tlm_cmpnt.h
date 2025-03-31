//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt.h
 * Internal public interface for the in-band telemetry component
 */

#pragma once

/*----------- Nested includes ------------*/
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
 * @brief Handle incoming MTS messages. Called from executive telemetry component.
 */
void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void);