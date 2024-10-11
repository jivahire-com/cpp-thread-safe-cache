//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_large_fifo_mbox_cli.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_STATUS
#include <MboxPrimitives.h>
#include <icc_cli.h>
#include <fpfw_icc_base.h>                // for fpfw_icc_base_recv_req_t
#include <hsp_firmware_headers.h>
#include <icc_platform_defines.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

#define LARGE_FIFO_MSG_PAYLOAD_DEPTH ((sizeof(large_fifo_mailbox_msg) - sizeof(struct kng_hsp_mailbox_msg_header)) / MBOX_WORD_SIZE_BYTE)

/*-------------Statics & Globals----------*/
extern fpfw_icc_base_ctx_t *icc_base_sdm_mbx_ctx;

/*--------- Function Prototypes ----------*/
FPFW_CLI_STATUS large_fifo_mbox_echo(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_send(int argc, const char** argv);
FPFW_CLI_STATUS large_fifo_mbox_recv(int argc, const char** argv);