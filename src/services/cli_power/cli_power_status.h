//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_status.h
 * Header file to support implementation of power status CLI commands.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "DfwkPtrTypes.h"     // for PDFWK_ASYNC_REQUEST_HEADER
#include "power_runconfig.h"  // for power_if_cmd_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief  Get the runconfig element ID for the sub-command. 
 * This is used to trigger the appropriate set function in the power service.
 * 
 *  @param[in] sub_command
 *     Pointer to the sub-command string
 * 
 *  @return
 *      The runconfig element ID for the sub-command
 */
power_if_cmd_t cli_power_status_get_cmd_id(char* sub_command);


/**
 * @brief Implementation of the power_status_completion CLI command.
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
void cli_power_status_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context);
