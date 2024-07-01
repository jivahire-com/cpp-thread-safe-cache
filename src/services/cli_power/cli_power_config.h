//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_config.h
 * Header file to support implementations of power config CLI commands.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>
#include <power_runconfig.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief  Implementation of the cli_power_config_async_print CLI command.
 * This function handles the print implementation for the power config CLI command, 
 * and is called though the driver framework after the data request has been processed by the 
 * power service.
 * 
 * It compares the sub-command string against the dictionary of sub-command strings 
 * and calls the corresponding print function.
 *
 *  @param[in] p_request
 *     Pointer to the request header
 * 
 * @param[in] completion_context
 *     Pointer to the completion context
 * 
 *  @return
 *      None
 */
void cli_power_config_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context);

/**
 *  @brief  Get the runconfig element ID for the sub-command. This is used to pass on the request to 
 *  fetch the appropriate data from the runconfig data structure within the power service.
 * 
 *  @param[in] sub_command
 *     Pointer to the sub-command string
 * 
 *  @return
 *      The runconfig element ID for the sub-command
 */
power_if_cmd_t cli_power_config_get_cmd_id(char* sub_command);