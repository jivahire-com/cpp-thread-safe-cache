//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hsp_firmware_headers.h
 * TODO: maintain HSP firmware headers until HSP publishes the headers as an artifact
 */

#pragma once

/*----------- Nested includes ------------*/
#include <kingsgate_hsp_boot_metadata.h>
#include <kingsgate_hsp_mailbox_commands.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef union HSP_BOOT_METADATA HSP_BOOT_METADATA;
typedef struct kng_hsp_mailbox_cmd_load_fw_req kng_hsp_mailbox_cmd_load_fw_req;

typedef enum _HSP_MAILBOX_RSP_STATUS {
	HSP_MAILBOX_RSP_STATUS_SUCCESS = 0,
	HSP_MAILBOX_RSP_STATUS_FAILURE = 1,
	HSP_MAILBOX_RSP_STATUS_MAX
} HSP_MAILBOX_RSP_STATUS;

/**
 * @brief Generic Mailbox message. Remove when HSP headers support it.
 * 
 */
typedef union _kng_hsp_mailbox_msg {
	struct kng_hsp_mailbox_msg_header header;	/**< incoming mailbox message from protocol to handler. */
	struct kng_hsp_mailbox_msg_rsp rsp;	        /**< outgoing mailbox message from handler to protocol. */
    uint32_t as_uint32[4];
} kng_hsp_mailbox_msg;

