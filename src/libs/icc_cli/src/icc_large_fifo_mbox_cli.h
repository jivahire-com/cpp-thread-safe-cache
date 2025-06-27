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
#include <icc_platform_defines.h>         // for large_fifo_mailbox_msg_header

/*-- Symbolic Constant Macros (defines) --*/

#define LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD (4)

/*-------------- Typedefs ----------------*/

typedef struct _large_fifo_cli_mailbox_msg
{
    large_fifo_mailbox_msg_header hdr;
    uint32_t data[LARGE_FIFO_MBOX_MAX_PAYLOAD_WORD];
} large_fifo_cli_mailbox_msg;

/*-------------Statics & Globals----------*/
extern fpfw_icc_base_ctx_t *icc_base_sdm_mbx_ctx;
extern fpfw_icc_base_ctx_t *icc_base_cded_mbx_ctx;

/*--------- Function Prototypes ----------*/
FPFW_CLI_STATUS large_fifo_mbox_echo(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_send(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_recv(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_loopback(int argc, const char** argv);