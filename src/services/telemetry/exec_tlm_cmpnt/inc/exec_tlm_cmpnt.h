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
 * param[in] pwr_rpt_period_ms The power report period in milliseconds.
 * param[in] perf_rpt_period_ms The performance report period in milliseconds.
 */
void exec_tlm_cmpnt_init(uint32_t pwr_rpt_period_ms, uint32_t perf_rpt_period_ms);