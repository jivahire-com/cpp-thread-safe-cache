//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_hsp_mbox_cli.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_STATUS
#include <fpfw_icc_base.h>                // for fpfw_icc_base_recv_req_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-------------Statics & Globals----------*/
extern const char* current_core_str;
extern fpfw_icc_base_ctx_t* icc_base_hsp_mbx_ctx;

/*--------- Function Prototypes ----------*/
FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv);
FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv);
FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv);
FPFW_CLI_STATUS hsp_mbox_echo(int argc, const char** argv);
FPFW_CLI_STATUS hsp_mbox_send(int argc, const char** argv);
FPFW_CLI_STATUS hsp_mbox_recv(int argc, const char** argv);