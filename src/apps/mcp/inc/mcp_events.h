//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mcp_events.h
 * Defines MCP App Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_MCP_MAIN, // ID
    MCP_MAIN,                         // Name
    ET_LEVEL_MASK_ALL                 // Logging Level Mask
)

/**
 * Define Event Trace events for the mcp Main Provider
*/
typedef enum {
    MCP_MAIN_EVENT_ID_HEARTBEAT = 0,
    MCP_MAIN_EVENT_ID_RT_INIT_COMPLETE,
} MCP_MAIN_EVENT_ID;


// This event has no fields
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_MAIN,
    MCP_MAIN_EVENT_ID_HEARTBEAT,
    McpHeartBeat,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, rtos_ticks)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_MAIN,
    MCP_MAIN_EVENT_ID_RT_INIT_COMPLETE,
    InitCompleteEvent,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, thread_input)
)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
