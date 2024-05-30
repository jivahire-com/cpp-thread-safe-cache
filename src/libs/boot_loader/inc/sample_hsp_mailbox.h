//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sample_hsp_mailbox.h
 * Temporary hsp header file containing macro and structure definitions , will be removed and actual
 * headers to be used once available. Current definitions taken from Pioneer.
 */

/*----------- Nested includes ------------*/

#pragma once

/*-------------- Typedefs ----------------*/

// TODO: Replace the  mail box data and response with proper structure once it has been defined on HSP
// side ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1793271


typedef enum _HSP_FIRMWARE_ID
{
	HspFirmwareIdBootManifest = 1,
	HspFirmwareIdSp1,
	HspFirmwareIdFirmwareManifest,
	HspFirmwareIdSprt,
	HspFirmwareIdScp,
	HspFirmwareIdMcp,
	HspFirmwareIdDdr,
	HspFirmwareIdPeSecurityMonitor,
	HspFirmwareIdPeManagement,
	HspFirmwareIdUefi,
	HspFirmwareIdDiagnosticDdr,
	HspFirmwareIdPcie,
	HspFirmwareIdUsb,
	HspFirmwareIdFuseOverride,
	HspFirmwareIdMeshConfig,
	HspFirmwareIdMetaData,
	HspFirmwareIdMax,
	HspFirmwareIdForceUint32 = 0x7FFFFFFF,
} HSP_FIRMWARE_ID;

typedef enum _HSP_MBOX_STATUS_EX
{
	HSP_MBOX_STATUS_NOT_FATAL = 0,
	HSP_MBOX_STATUS_FATAL,
	HSP_MBOX_STATUS_COMPLETE,
} HSP_MBOX_STATUS_EX;

typedef struct _HSP_MBOX_FIRMWARE_BOOT_STATUS_CMD
{
	HSP_FIRMWARE_ID Id;
	uint32_t Status;
	HSP_MBOX_STATUS_EX StatusEx;
} HSP_MBOX_FIRMWARE_BOOT_STATUS_CMD, *PHSP_MBOX_FIRMWARE_BOOT_STATUS_CMD,
	HSP_MBOX_FIRMWARE_UPDATE_STATUS_CMD,
	*PHSP_MBOX_FIRMWARE_UPDATE_STATUS_CMD;

typedef enum _HSP_MAILBOX_CMD
{
	// variable service
	HspMailboxCmdGetVariable = 1,
	// Data structure is growing, so temporarily
	// move the HspMailboxCmdGetNextVariableName value
	// Once the clients all move to the new one, the
	// depricated one can be removed
	HspMailboxCmdGetNextVariableName_DEPRECATED,
	HspMailboxCmdSetVariable,
	HspMailboxCmdQueryVariable,

	// variable policy
	HspMailboxCmdDisableVarPolicy,
	HspMailboxCmdIsVarPolicyEnabled,
	HspMailboxCmdLockVarPolicy,
	HspMailboxCmdDumpVarPolicy,
	HspMailboxCmdRegisterVarPolicy,

	// runtime service
	HspMailboxCmdExitBootService,

	// tpm
	HspMailboxCmdTpmStart,

	// boot flow
	HspMailboxCmdLoadAndStartCore,
	HspMailboxCmdLoadFirmware,
	HspMailboxCmdCoreResetComplete,
	HspMailboxCmdFirmwareBootStatus,

	// update
	HspMailboxCmdGetFirmwareMetadata,
	HspMailboxCmdFimwareUpdate,
	HspMailboxCmdPrepareForCoreReset,
	HspMailboxCmdFimwareUpdateStatus,
	HspMailboxCmdCopySlotAToSlotB,
	HspMailboxCmdCopySlotBToSlotA,

	// runtime
	HspMailBoxCmdHeartBeat,
	HspMailboxCmdCrashDump,

	// reset
	HspMailboxCmdShutdown,
	HspMailboxCmdColdReset,
	HspMailboxCmdWarmReset,

	// Jtag Mailbox commands
	HspMailboxCmdJtagDebugNonSecure,

	// HspVersion AP NS
	HspMailboxCmdGetHspSprtVersion,

	// Verify firmware image in active slot
	HspMailboxCmdVerifyFwImages,

	// error handling
	HspMailboxCmdError,

	// Temporary value during transition
	// to new GetNextVariableName structure
	HspMailboxCmdGetNextVariableName,

	// DRAM availablility
	HspMailboxCmdDramInitialized,
	HspMailboxCmdDramRegionAllocated,

	// debug trace log
	HspMailBoxCmdGetLogInfo,
	HspMailBoxCmdGetLog,
	HspMailBoxCmdSetLogLevel,
	HspMailBoxCmdClearLog,
	HspMailBoxCmdSetUtc,

	// impactless update
	HspMailboxCmdFimwareErase,
	HspMailboxCmdResetCore,
	HspMailboxCmdGetStagedFirmware,

	HspMailboxCmdGetSecurityStatus,
	HspMailboxCmdGetMetadataVersion,

	// mcp buffer log
	HspMailboxCmdSendLog,
	HspMailboxCmdSendLogComplete,

	// Svn
	HspMailboxCmdGetSvnFuseData,
	HspMailboxCmdUpdateSvnFuse,

	HspMailboxCmdHostLoadFirmware,

	// XCP SEL
	HspMailboxCmdXcpSelLog,

	//
	// max
	HspMailBoxCmdMax,

} HSP_MAILBOX_CMD;

typedef union _HSP_MAILBOX_MSG
{
	struct
	{
		uint32_t Cmd     : 16;
		uint32_t Context : 16;
		union
		{
			HSP_MBOX_FIRMWARE_BOOT_STATUS_CMD BootStatus;
			uint32_t Msg[3];
		};
	};

	uint32_t AsUint32[4];
} HSP_MAILBOX_MSG, *PHSP_MAILBOX_MSG;

/*--------- Function Prototypes ----------*/