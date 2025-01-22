//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_log.h
 * Header file to support implementations of power log CLI commands.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "DfwkPtrTypes.h"  // for PDFWK_ASYNC_REQUEST_HEADER

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief  Get the runlog element ID for the sub-command. 
 * This is used to trigger the appropriate set function in the power service.
 * 
 *  @param[in] sub_command
 *     Pointer to the sub-command string
 * 
 *  @return
 *      The runlog element ID for the sub-command
 */
power_if_cmd_t cli_power_log_get_cmd_id(char* sub_command);

/**
 *  @brief Implementation of the power_log_clear CLI command processor.
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
void cli_power_log_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context);
