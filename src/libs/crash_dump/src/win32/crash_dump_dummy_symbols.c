//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump_dummy_symbols.c
 *  Provides symbols for win32 build that are given by the ARM linker script
 */

/*--------------- Includes ---------------*/
#include "../crash_dump_payload.h" // for GNU_BUILD_ID

#include <stdint.h> // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
uint32_t __stack_start__;
uint32_t __stack_end__;
uint8_t _build_id_msdata_start[sizeof(GNU_BUILD_ID)];

/*------------- Functions ----------------*/
