//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_status.h
 * Crash dump status management functions
 */
#pragma once

/*----------- Nested includes ------------*/
#include <crash_dump.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Update crash dump region state.
 * 
 * @param state 1 in used, 0 not in use
 */
void crash_dump_update_state(crash_dump_state_t state);

/**
 * @brief Update crash dump state of core.
 * 
 * @param state Idle (0), progress (1) or completed (2).
 */
void crash_dump_update_core_state(crash_dump_core_state_t state);
