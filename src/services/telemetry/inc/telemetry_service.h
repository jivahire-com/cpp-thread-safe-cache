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
 * @param[in] pwr_pkg_period_ms The power package period in milliseconds.
 * @param[in] inst_pkg_sample_period_ms The instantaneous package sample period in milliseconds.
 * @param[in] inst_samples_per_pkg The number of samples per instantaneous package.
 */
void telemetry_service_init(uint8_t die_id, uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms, uint16_t inst_samples_per_pkg);