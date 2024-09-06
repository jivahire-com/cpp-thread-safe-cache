//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt.h
 * Internal public interface for the in-band telemetry component
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the in band telemetry component.
 */
void in_band_tlm_cmpnt_init(void);

/**
 * @brief Generate performance report.
 */
void in_band_tlm_cmpnt_generate_perf_report(void);

/**
 * @brief Generate power report.
 */
void in_band_tlm_cmpnt_generate_pwr_report(void);