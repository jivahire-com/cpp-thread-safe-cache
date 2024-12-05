//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_cli_i.h
 * Private header for icc mhu cli functionality.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <FpFwCli.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief API to list the indices of the icc base contexts
 * 
 * @param[in] argc number of arguments
 * @param[in] argv arguments
 * 
 * @return FPFW_CLI_STATUS CLI status
 */
FPFW_CLI_STATUS mhu_list_indices(int argc, const char** argv);

/**
 * @brief API to receive a message from the icc base context. Only one pending request is allowed.
 * 
 * @param[in] argc number of arguments
 * @param[in] argv arguments
 * 
 * @return FPFW_CLI_STATUS CLI status
 */
FPFW_CLI_STATUS mhu_recv(int argc, const char** argv);

/**
 * @brief API to send a message to the icc base context. Only one pending request is allowed.
 * 
 * @param[in] argc number of arguments
 * @param[in] argv arguments
 * 
 * @return FPFW_CLI_STATUS CLI status
 */
FPFW_CLI_STATUS mhu_send(int argc, const char** argv);

/**
 * @brief API to clear all pending messages.
 * 
 * @param[in] argc number of arguments
 * @param[in] argv arguments
 * 
 * @return FPFW_CLI_STATUS CLI status
 */
FPFW_CLI_STATUS mhu_clear(int argc, const char** argv);


/**
 * @brief API to list icc mhu properties.
 * 
 * @param[in] argc number of arguments
 * @param[in] argv arguments
 * 
 * @return FPFW_CLI_STATUS CLI status
 */
FPFW_CLI_STATUS mhu_props(int argc, const char** argv);
