//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ddr.h
 * Header file for power cli
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern const bool g_cli_ddr_dev_mode;

/*--------- Function Prototypes ----------*/
void cli_ddr_init(void);
