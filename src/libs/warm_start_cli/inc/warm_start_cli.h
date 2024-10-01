//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_cli.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *
 *    Initializes the Warm Reset CLI Lib  
 * 
 *    @brief Called once for initialization
 *
 */
void warm_start_cli_init(psos_device_t p_device);

