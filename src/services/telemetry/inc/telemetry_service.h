//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_service.h
 * Public telemetry service interface.
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the telemetry service.
 *
 * @param[in] die_id The die id.
 * @param[in] pwr_rpt_period_ms The power report period in milliseconds.
 * @param[in] inst_rpt_period_ms The performance report period in milliseconds.
 */
void telemetry_service_init(uint8_t die_id, uint32_t pwr_rpt_period_ms, uint32_t inst_rpt_period_ms);