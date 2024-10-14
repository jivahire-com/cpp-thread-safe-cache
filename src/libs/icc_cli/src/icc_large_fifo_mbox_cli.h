//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_large_fifo_mbox_cli.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_STATUS
#include <fpfw_icc_base.h>                // for fpfw_icc_base_recv_req_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-------------Statics & Globals----------*/
extern fpfw_icc_base_ctx_t *icc_base_sdm_mbx_ctx;

/*--------- Function Prototypes ----------*/
FPFW_CLI_STATUS large_fifo_mbox_echo(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_send(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_recv(int argc, const char** argv);