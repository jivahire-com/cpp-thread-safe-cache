//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_context.h
 * Crash Dump Context management functions
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <atu_lib.h>    // for atu_map_entry_t
#include <crash_dump.h> // for crash_dump_context_t
#include <idsw_kng.h>   // for KNG_DIE_ID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Internal function to set the Crash Dump Context object
 *
 * @param ctx Pointer to crash dump context.
 */
void set_crash_dump_context(crash_dump_context_t* ctx);

/**
 * @brief Get the Crash Dump memory region address for a specific DIE and core.
 *        This helps to make mock for unit tests.
 *
 * @return Pointer to crash dump raw buffer.
 */
uint8_t* get_crash_dump_region_address(atu_map_entry_t* die1_entry, KNG_DIE_ID die_id, crash_dump_core_t core_id);