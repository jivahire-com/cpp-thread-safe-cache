//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio_cli.h
 * Header file for GPIO cli
 */

#pragma once
#include <gpio_lib.h>   // for gpio_config_entry_t

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void gpio_cli_init(pgpio_interface_t gpio_iface);
