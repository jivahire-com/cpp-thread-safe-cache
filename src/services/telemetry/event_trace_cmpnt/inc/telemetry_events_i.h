//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_events_i.h
 * Event traces for the telemetry service
 */

#pragma once

/*----------- Nested includes ------------*/
#define FPFW_ET_NO_ALIGNMENT_CHECKS

#include <event_trace_providers.h>
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                           TelemetrySvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     10,
                     PwrPkgMemoryAllocationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     11,
                     InstPkgMemoryAllocationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     12,
                     PwrPkgGenerationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     13,
                     InstPkgGenerationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     14,
                     DeAllocateInvalidLocation,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     15,
                     DeAllocateQueueFreeFailed,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     16,
                     PkgCreatePwrPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, pkg_available_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     17,
                     PkgCreateInstPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, pkg_available_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     18,
                     DataCollectionEnabled,
                     FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     19,
                     DataCollectionDisabled,
                     FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     20,
                     TelemetryDataCleared,
                     FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     21,
                     DcsMgrClientFreeFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, block_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     22,
                     DcsMgrClientQueueFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, queue),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     23,
                     DcsMgrClientUnexpectedDcpMsg,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     24,
                     DcsMgrInvalidEventEnableDisable,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, provider_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, event_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     25,
                     DTSCoefficientReadFailedInit,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     26,
                     DIMMInfoInvalidDimmId,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     27,
                     DataPackagePWRrecordError,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, element_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     28,
                     DataPackageInstRecordError,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, element_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     29,
                     DcsMgrPkgFreeListEmpty,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    30,
                    TlmSvcDebugIncomingDcpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, seq_num))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    31,
                    TlmSvcDebugOutgoingDcpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, seq_num))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    32,
                    TlmSvcDebugIncomingTrpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_cpu_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_cpu_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dcp_client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT16, trp_msg_status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, init_cmd),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, source_seq_num))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    33,
                    TlmSvcDebugOutgoingTrpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_cpu_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_cpu_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dcp_client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT16, trp_msg_status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, init_cmd),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, source_seq_num))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    34,
                    DcsMgrNoMatchPkgComplete,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ddr_addr_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    35,
                    DcsMgrClientUnexpectedTrpMsg,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    36,
                    DcsMgrUnexpectedCallOnPrimary,
                    FPFW_ET_LEVEL_WARNING)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    37,
                    DcsMgrUnexpectedCallOnSecondary,
                    FPFW_ET_LEVEL_WARNING)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    38,
                    DdrMgrDbgAllocatePwrPkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    39,
                    DdrMgrDbgAllocateInstPkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    40,
                    DdrMgrDbgFreePkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    41,
                    DcsMgrAddPkgToActiveList,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_cpu_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    42,
                    DcsMgrRemovePkgFromActive,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_cpu_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    43,
                    DcsMgrResetMsgReceived,
                    FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     44,
                     DataUpdateMMAvgOverflow,
                     FPFW_ET_LEVEL_ERROR)
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     45,
                     DataUpdateMMAWrongInValidTimeStamp,
                     FPFW_ET_LEVEL_DEBUG)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     46,
                     PstatePWRUpdateMMAvgOverflow,
                     FPFW_ET_LEVEL_ERROR)

/*--------- Function Prototypes ----------*/

