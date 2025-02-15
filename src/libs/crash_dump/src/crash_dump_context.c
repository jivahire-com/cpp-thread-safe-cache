//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_context.c
 * Crash Dump Context management functions
 */

/*------------- Includes -----------------*/
#include "crash_dump_context.h"

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
