//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_set.h
 * Header file to support implementations of power set CLI commands.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Implementation of the cli_power_set_completion CLI command.
 *
 * @param[in] p_request
 *     Pointer to the request header
 * 
 * @param[in] completion_context
 *     Pointer to the completion context
 * 
 *  @return
 *      None
 */
void cli_power_set_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context);
