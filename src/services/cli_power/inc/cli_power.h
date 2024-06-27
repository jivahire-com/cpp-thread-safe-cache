//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power.h
 * Header file for power cli
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>
#include <power_dfwk.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  API for registering the CLI commands for this module
 *
 *  @return status of initialization (CLI_SUCCESS?CLI_ERROR)
 */
FPFW_CLI_STATUS cli_power_init(ppower_service_interface_t p_interface);
