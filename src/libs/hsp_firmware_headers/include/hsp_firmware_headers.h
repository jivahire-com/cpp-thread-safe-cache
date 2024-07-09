//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hsp_firmware_headers.h
 * TODO: maintain HSP firmware headers until HSP publishes the headers as an artifact
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
enum {
	HSP_MAILBOX_MSG_UNKNOWN = 0x0,							/**< Unknown message */
	HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ = 0x1,			/**< Message to request fuses and 1SP load. */
	HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP = 0x2,			/**< Message to send response for fuses and 1SP load. */
	HSP_MAILBOX_MSG_GOTO_RECOVERY_REQ = 0x3,				/**< Message to request secondary die go to recovery. */
	HSP_MAILBOX_MSG_GOTO_RECOVERY_RSP = 0x4,				/**< Message to send response secondary die go to recovery. */
	HSP_MAILBOX_MSG_GOTO_HALT_REQ = 0x5,					/**< Message to request secondary die go to halt. */
	HSP_MAILBOX_MSG_GOTO_HALT_RSP = 0x6,					/**< Message to send response secondary die go to halt. */
	HSP_MAILBOX_MSG_GET_LOGS_REQ = 0x7,						/**< Message to request secondary die to write its logs to scratch RAM. */
	HSP_MAILBOX_MSG_GET_LOGS_RSP = 0x8,						/**< Message to send response for secondary die get logs request. */
	HSP_MAILBOX_MSG_LOAD_PROVISIONING_REQ = 0x9,			/**< Message to request secondary die to load provisionig blob from scratch RAM. */
	HSP_MAILBOX_MSG_LOAD_PROVISIONING_RSP = 0xA,			/**< Message to send response from secondary die after loading provisionig blob from scratch RAM. */
	HSP_MAILBOX_CMD_LOAD_FW_REQ = 0x80,						/**< SCP will request HSP to load different fw to respective destinations.*/
	HSP_MAILBOX_CMD_LOAD_FW_RSP = 0x81,						/**< HSP will respond back to SCP after loading the fw. */
	HSP_MAILBOX_CMD_START_CORE_REQ = 0x82,					/**< SCP will request HSP to bring MCP out of reset.*/
	HSP_MAILBOX_CMD_START_CORE_RSP = 0x83,					/**< HSP will respond back to SCP after bringing MCP out of reset. */
	HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_REQ = 0x84,			/**< SCP will request HSP to program SMMU into bypass mode. */
	HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_RSP = 0x85,			/**< HSP will respond back to SCP after completing SMMU programming. */
	HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY = 0x86,			/**< SCP will send a notifcation to HSP upon DDR init complete. */
	HSP_MAILBOX_CMD_DDR_INIT_DONE_NOTIFY_RSP = 0x87,		/**< HSP response to DDR init notification currently no action. */
	HSP_MAILBOX_CMD_DDR_ADDR_NOTIFY = 0x88,					/**< SCP will send a notification to HSP regarding the base address of the DDR region allocated to HSP. . */
	HSP_MAILBOX_CMD_DDR_ADDR_NOTIFY_RSP = 0x89,				/**< HSP will respond back to SCP. This response is important for SCP to capture. This will indicate that HSP has completed copying the required flash contents from flash to DDR */
	HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ = 0x8A,	/**< SCP will send a request to HSP to program CDESS and ddrss towers. */
	HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_RSP = 0x8B,	/**< HSP will notify SCP upon tower config complete. */
	HSP_MAILBOX_CMD_CML_READY_NOTIFY = 0x8C,				/**< SCP will send a notification to HSP regarding CMN link being ready. */
	HSP_MAILBOX_CMD_CML_READY_NOTIFY_RSP = 0x8D,			/**< HSP response. currently no action. */
	HSP_MAILBOX_CMD_CDESS_SETUP_DONE_NOTIFY = 0x8E,			/**< SCP will send a notification to HSP regarding CDEDSS set-up being done. */
	HSP_MAILBOX_CMD_CDESS_SETUP_DONE_NOTIFY_RSP = 0x8F,		/**< HSP response. currently no action. */
	HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY = 0x90,				/**< All cores will send back boot status to HSP. */
	HSP_MAILBOX_CMD_BOOT_STATUS_NOTIFY_RSP = 0x91,			/**< HSP response. Currently no action. */
	HSP_MAILBOX_CMD_MAX,
};

typedef enum _HSP_FIRMWARE_ID {
	HspFirmwareIdOwnerKeyManifest = 1,
	HspFirmwareIdSp1,
	HspFirmwareIdFirmwareKeyManifest,
	HspFirmwareIdTableOfContents,
	HspFirmwareIdFirmwareDescriptor,
	HspFirmwareIdSprt,
	HspFirmwareIdScp,
	HspFirmwareIdMcp,
	HspFirmwareIdFuseOverrideDie0,
	HspFirmwareIdFuseOverrideDie1,
	HspFirmwareIdDdrPhy,
	HspFirmwareIdDiagnosticDdrPhy,
	HspFirmwareIdDiagnosticDdrMicroCode,
	HspFirmwareIdPciePhy,
	HspFirmwareIdUsbPhy,
	HspFirmwareIdMeshConfig,
	HspFirmwareIdPeSecurityMonitor,
	HspFirmwareIdPeManagementMode,
	HspFirmwareIdPeUefi,
	HspFirmwareIdPeSpm,
	HspFirmwareIdRmm,
	HspFirmwareIdSpmcManifest,
	HspFirmwareIdSdm,
	HspFirmwareIdCded,
	HspFirmwareIdKmp,
	HspFirmwareIdMscpTrace,
	HspFirmwareIdMetaData,
	HspFirmwareIdIftVector,
	HspFirmwareIdMax,
	HspFirmwareIdForceUint32 = 0x7FFFFFFF,
} HSP_FIRMWARE_ID;

typedef struct _kng_hsp_mailbox_msg_header {
	uint32_t cmd:16;	/**< request command. */
	uint32_t seq:8;		/**< request sequence. */
	uint32_t context:4;	/**< optional context. */
	uint32_t flags:4;	/**< set to HSP_MAILBOX_FLAGS_RESPONSE_REQUESTED if response required from HSP. */
} kng_hsp_mailbox_msg_header;

typedef struct _kng_hsp_mailbox_msg_rsp {
	kng_hsp_mailbox_msg_header header;	/**< msg header conataining cmd,seq ,context and flags. */
	uint32_t status;							/**< status of the processed incoming mailbox messages. */
	uint32_t status_ex;							/**< status additional information or actions like goto recovery could be appended. */
} kng_hsp_mailbox_msg_rsp;

typedef struct _kng_hsp_mailbox_cmd_load_fw_req {
	kng_hsp_mailbox_msg_header header;	/**< msg header conataining cmd,seq ,context and flags. */
	HSP_FIRMWARE_ID id;							/**< FW id of the FW to be loaded. */
	uint32_t address;							/**< destination address.  */
	uint32_t size;								/**< size of the FW to be loaded. */
} kng_hsp_mailbox_cmd_load_fw_req;

typedef union _kng_hsp_mailbox_msg {
	kng_hsp_mailbox_msg_header header;	/**< incoming mailbox message from protocol to handler. */
	kng_hsp_mailbox_msg_rsp rsp;	        /**< outgoing mailbox message from handler to protocol. */
    uint32_t as_uint32[4];
} kng_hsp_mailbox_msg;

typedef union _HSP_BOOT_METADATA
{
    struct
    {
        uint32_t MetadataVersion : 8;   /**< 8 bits for MetadataVersion. */
        uint32_t BootMode : 4;      /**< 4 bits for BootMode. */
        uint32_t BoardId : 4;       /**< 4 bits for BoardId. */
        uint32_t ResetReason : 4;   /**< 4 bits for ResetReason. */
        uint32_t SingleDieEn : 1;   /**< 1 bit for SingleDieEn. */
        uint32_t CurrentDie : 1;    /**< 1 bit for CurrentDie. */
        uint32_t Reserved : 10;     /**< 10 bits reserved for future use. */
    };

    uint32_t AsUint32;
} HSP_BOOT_METADATA;

/*--------- Function Prototypes ----------*/