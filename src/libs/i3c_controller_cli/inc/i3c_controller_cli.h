//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file i3c_controller_cli.h
 * i3c_controller CLI APIs
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkClient.h>
#include <dw_i3c.h>
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *
 *    Initializes the i3c_controller CLI interface  
 *
 *    @param[in]  Interface
 * 
 *    @brief Function call to initialize the i3c_controller CLI interface
 *
 */
void i3c_controller_cli_initialize(void);

