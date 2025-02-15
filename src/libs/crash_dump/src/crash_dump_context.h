//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_context.h
 * Crash Dump Context management functions
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <crash_dump.h> // for crash_dump_context_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Internal function to set the Crash Dump Context object
 *
 * @param ctx Pointer to crash dump context.
 */
void set_crash_dump_context(crash_dump_context_t* ctx);
