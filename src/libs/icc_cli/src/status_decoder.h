//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file status_decoder.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_status.h>
#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_STATUS

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
/**
 * @brief  Get the string representation of the error code
 * 
 * @param errorCode 
 * @return const char* 
 */
const char* get_fpfw_status_code_string(fpfw_status_t errorCode);

/**
 * @brief  Print the error string
 * 
 * @param argc 
 * @param argv 
 * @return FPFW_CLI_STATUS 
 */
FPFW_CLI_STATUS print_error_string(int argc, const char** argv);