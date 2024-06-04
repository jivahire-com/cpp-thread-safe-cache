//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utils.h
 * This file contains some common macros, variables and inline 
 * functions used across the project that is specific to
 * MSCP and accelerators firmware.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define KB                  (1024)
#define UNUSED(x)           (void)(x)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void dummy_function(void);

/**
 *
 * This function is used to sleep (blocking) in milliseconds.
 *
 * Credit to src\libs\boot_loader\kingsgate_boot.c author for the below API.
 * Since SCP is assumed to run at 1 GHz clock, this API is an approximate
 * estimation and not a clock accurate API for sleep in milliseconds.
 *
 *    @param[in] millisecond
 *              Sleep duration (blocking) in milliseconds.
 *
 *    @retval none
 */
void sleep_ms(uint32_t milliseconds);
