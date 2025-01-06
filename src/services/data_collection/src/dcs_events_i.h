//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_events_i.h
 * Event traces for the data collection service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h> // for EVENT_TRACE_PROVIDER_ID_SCP_FUSE
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                           DcsSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     10,
                     TrpUnmappedRoute,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, die_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cpu_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     11,
                     TrpIccReceiveFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     12,
                     TrpIccUnexpectedCmdCode,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cmd_code))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     13,
                     TrpIccInvalidCallbackEndpoint,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, endpoint))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     14,
                     TrpIccReceiveRegisterFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, endpoint))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     15,
                     DcsInvalidClientId,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, client_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     16,
                     DcsMultipleRegistrationNotAllowed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, client_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     17,
                     DcsSvcClientFreeFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, block_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     18,
                     DcsSvcClientQueueFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, queue),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     19,
                     DcsSvcClientUnexpectedMsg,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     20,
                     DcsSvcClientAllocateFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pool),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     21,
                     TrpIccSendFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, icc_endpoint))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     22,
                     TrpIccNoRoute,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, dest_die_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, dest_cpu_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_DCS_SERVICE,
                     23,
                     TrpInvalidBroadcastType,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, broadcast_type))

/*--------- Function Prototypes ----------*/