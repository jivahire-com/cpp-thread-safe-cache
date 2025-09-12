//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hsp_firmware_headers.h
 * TODO: maintain HSP firmware headers until HSP publishes the headers as an artifact
 */

#pragma once

/*----------- Nested includes ------------*/
#include <boot_status_codes.h>
#include <kingsgate_hsp_boot_metadata.h>
#include <kingsgate_hsp_mailbox_commands.h>

/*-- Symbolic Constant Macros (defines) --*/
#define HSP_MBOX_FIFO_DEPTH						(4U)

/*-------------- Typedefs ----------------*/
typedef union HSP_BOOT_METADATA HSP_BOOT_METADATA;
typedef struct kng_hsp_mailbox_cmd_load_fw_req kng_hsp_mailbox_cmd_load_fw_req;
typedef struct kng_hsp_mailbox_cmd_set_variable kng_hsp_mailbox_cmd_set_variable;
typedef struct kng_hsp_mailbox_cmd_get_variable kng_hsp_mailbox_cmd_get_variable;
typedef struct kng_hsp_mailbox_cmd_load_fw_64bit_req kng_hsp_mailbox_cmd_load_fw_64bit_req;
typedef struct kng_hsp_mailbox_cmd_prepare_for_core_reset_req kng_hsp_mailbox_cmd_prepare_for_core_reset_req;

typedef enum _HSP_MAILBOX_RSP_STATUS {
	HSP_MAILBOX_RSP_STATUS_SUCCESS = 0,
	HSP_MAILBOX_RSP_STATUS_LOAD_ERROR = 0x01,
	HSP_MAILBOX_RSP_STATUS_INVALID_PARAMETER = 0x02,
	HSP_MAILBOX_RSP_STATUS_UNSUPPORTED = 0x03,
	HSP_MAILBOX_RSP_STATUS_BAD_BUFFER_SIZE = 0x04,
	HSP_MAILBOX_RSP_STATUS_BUFFER_TOO_SMALL = 0x05,
	HSP_MAILBOX_RSP_STATUS_NOT_READY = 0x06,
	HSP_MAILBOX_RSP_STATUS_DEVICE_ERROR = 0x07,
	HSP_MAILBOX_RSP_STATUS_WRITE_PROTECTED = 0x08,
	HSP_MAILBOX_RSP_STATUS_OUT_OF_RESOURCES = 0x09,
	HSP_MAILBOX_RSP_STATUS_VOLUME_CORRUPTED = 0x0A,
	HSP_MAILBOX_RSP_STATUS_VOLUME_FULL = 0x0B,
	HSP_MAILBOX_RSP_STATUS_NOT_FOUND = 0x0E,
	HSP_MAILBOX_RSP_STATUS_SECURITY_VIOLATION = 0x1A,
} HSP_MAILBOX_RSP_STATUS;

/**
 * @brief Generic Mailbox message. Remove when HSP headers support it.
 * 
 */
typedef union _kng_hsp_mailbox_msg {
	struct kng_hsp_mailbox_msg_header header;	/**< incoming mailbox message from protocol to handler. */
	struct kng_hsp_mailbox_boot_status_notify boot_stat_notif; //! Initiator MSCP, Receiver HSP
	struct kng_hsp_mailbox_boot_status_extd_notify boot_stat_extd_notif; //! Initiator MSCP, Receiver HSP
	struct kng_hsp_mailbox_msg_rsp rsp;	        /**< outgoing mailbox message from handler to protocol. */
	struct kng_hsp_mailbox_cmd_get_security_state_rsp policy_status_rsp; /**<Security status of the HSP. */
	struct kng_hsp_mailbox_cmd_telemetry_ddr_addr_notify telm_ddr_addr_notify; /**< SCP will send a notification to HSP regarding the base address of the DDR region allocated to HSP. */
	struct kng_hsp_mailbox_cmd_prepare_for_core_reset_req prepare_for_core_reset_req; /**< outgoing mailbox message to notify other cores to do quiscing */
	struct kng_hsp_mailbox_msg_cmd_enable_smmu_access_req smmu_access_req;
	struct kng_hsp_mailbox_msg_cmd_post_scp_init_tower_config_req tower_config_req;
	struct kng_hsp_mailbox_cmd_ddrss_fips_key_test_status_notify fips_key_test_status_notify; /**< outgoing mailbox message to notify the status of the FIPS key test. */
	uint32_t as_uint32[HSP_MBOX_FIFO_DEPTH];
} kng_hsp_mailbox_msg;

/*--------- Function Prototypes ----------*/