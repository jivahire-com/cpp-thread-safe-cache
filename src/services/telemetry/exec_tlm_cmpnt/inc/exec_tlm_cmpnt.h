//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_tlm_cmpnt.h
 * Internal public interface for the telemetry executive component
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the telemetry executive component.
 * param[in] pwr_pkg_period_ms The power package period in milliseconds.
 * param[in] inst_pkg_sample_period_ms The instantaneous package period in milliseconds.
 */
void exec_tlm_cmpnt_init(uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms);