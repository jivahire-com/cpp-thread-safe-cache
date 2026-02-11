//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_context.c
 * Crash Dump Context management functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_context.h"

#include <crash_dump_memory.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static crash_dump_context_t* g_crash_dump_ctx = NULL;

/*------------- Functions ----------------*/
/**
 * @brief Get the Crash Dump Context object
 *
 * @return Pointer to static crash dump context.
 */
crash_dump_context_t* crash_dump_context()
{
    return g_crash_dump_ctx;
}

/**
 * @brief Internal function to set the Crash Dump Context object
 *
 * @param ctx Pointer to crash dump context.
 */
void set_crash_dump_context(crash_dump_context_t* ctx)
{
    g_crash_dump_ctx = ctx;
}

/**
 * @brief Get the Crash Dump memory region address for a specific DIE and core.
 *        This helps to make mock for unit tests.
 *
 * @return Pointer to crash dump raw buffer.
 */
uint8_t* get_crash_dump_region_address(KNG_DIE_ID die_id, crash_dump_core_t core_id)
{
    return CRASH_DUMP_CORE_ADDRESS(die_id, core_id);
}
