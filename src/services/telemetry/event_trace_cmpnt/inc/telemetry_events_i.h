//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_events_i.h
 * Event traces for the telemetry service
 */

#pragma once

/*----------- Nested includes ------------*/

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
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     11,
                     InstPkgMemoryAllocationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     12,
                     PwrPkgGenerationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     13,
                     InstPkgGenerationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     14,
                     DeAllocateInvalidLocation,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     15,
                     DeAllocateQueueFreeFailed,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     16,
                     PkgCreatePwrPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pkg_available_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     17,
                     PkgCreateInstPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pkg_available_size))

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
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, block_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     22,
                     DcsMgrClientQueueFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, queue),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     23,
                     DcsMgrClientUnexpectedMsg,
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
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     26,
                     DIMMInfoInvalidDimmId,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))
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


/*--------- Function Prototypes ----------*/