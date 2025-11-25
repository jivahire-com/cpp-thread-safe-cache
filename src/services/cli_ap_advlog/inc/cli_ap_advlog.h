//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_sys_info_int.h
 * Header file for system information init CLI
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <FpFwCli.h>            // for FpFwCliDisplayUsage, FPFW_C...

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Initialise cli_sys_info
 *
 * \b Description:
 * Registers the commands executed by cli_sys_info_init with FpFw CLI
 *
 * @param[in]
 * void
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
FPFW_CLI_STATUS cli_ap_advlog_pldm_init(void);
