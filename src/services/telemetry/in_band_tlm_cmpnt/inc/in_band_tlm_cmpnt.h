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
 */
void in_band_tlm_cmpnt_init(uint8_t die_id);

/**
 * @brief Generate performance report.
 */
void in_band_tlm_cmpnt_generate_inst_report(void);

/**
 * @brief Generate power report.
 */
void in_band_tlm_cmpnt_generate_pwr_report(void);