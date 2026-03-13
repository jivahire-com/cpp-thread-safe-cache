//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_events.h
 * This file defines the event trace events for SCP AVS
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_AVS,                                                                                                           // ID
    SCP_AVS,                                                                                                                                   // Name
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS     // Logging Level Mask
)

/**
 * Define Event Trace events for the mesh
*/
typedef enum {
    SCP_AVS_EVENT_ID_CRC_ERROR = 0,
    SCP_AVS_EVENT_ID_GET_CMD_RESP_READ_ERROR,
    SCP_AVS_EVENT_ID_GET_CMD_RESP_WRITE_ERROR,

    SCP_AVS_EVENT_ID_MAX
} SCP_AVS_EVENT_ID;

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_AVS,
    SCP_AVS_EVENT_ID_CRC_ERROR,
    ScpAvsCrcError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_AVS,
    SCP_AVS_EVENT_ID_GET_CMD_RESP_READ_ERROR,
    ScpAvsDriGetCmdRespReadError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_AVS,
    SCP_AVS_EVENT_ID_GET_CMD_RESP_WRITE_ERROR,
    ScpAvsDriGetCmdRespWriteError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
