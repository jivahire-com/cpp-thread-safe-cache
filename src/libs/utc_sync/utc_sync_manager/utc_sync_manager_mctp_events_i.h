//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_manager_mctp_events_i.h
 * Event traces for the UTC Sync Manager MCTP library
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                           UtcSyncManagerMctpLib,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

// Indexing of IDs: 1x for FATAL, 2x for ERROR, 3x for WARNING, 4x for INFO, 5x for VERBOSE, 6x for DEBUG

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    20,
                    UtcSyncManagerMctpSendMsgFailed,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_info),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_ptr),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_size),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, dest_eid),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, msg_tag),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, tag_owner))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    21,
                    UtcSyncManagerMctpRecvMsgFailed,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_info),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_ptr),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_size),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, msg_type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    22,
                    UtcSyncManagerMctpInitFailed,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    23,
                    UtcSyncManagerMctpMsgSizeTooLarge,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64_HEX, msg_size),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64_HEX, buffer_size_max))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    24,
                    UtcSyncManagerRequestMtsUtcStampFailed,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    25,
                    UtcSyncManagerMctpRecvMsgInvalidMsg,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, mctp_recv_ptr),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, mctp_msg_ptr),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, cmd_set),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, cmd),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, completion_code))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    50,
                    UtcSyncManagerMctpSendMsgComplete,
                    FPFW_ET_LEVEL_VERBOSE,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_ptr),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, msg_size),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, dest_eid),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, msg_tag),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, tag_owner))
    
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    51,
                    UtcSyncManagerMctpRequestTimestampComplete,
                    FPFW_ET_LEVEL_VERBOSE,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_UTC_SYNC_MANAGER,
                    52,
                    UtcSyncManagerMctpRecvMsgAlreadyPending,
                    FPFW_ET_LEVEL_VERBOSE)

/*--------- Function Prototypes ----------*/