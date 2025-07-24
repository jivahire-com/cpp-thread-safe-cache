//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_tlm_cmpnt.h
 * Internal public interface for the out-of-band telemetry component
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h> // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the out of band telemetry component.
 * @param[in] die_id The die id.
 */
void out_of_band_tlm_cmpnt_init(uint8_t die_id);

/**
 * @brief Handle PLDM requests
 */
void out_of_band_tlm_cmpnt_handle_new_pldm_requests(void);

/**
 * @brief Print the sensors in the out of band telemetry component.
 */
void out_of_band_tlm_cmpnt_print_sensors(void);