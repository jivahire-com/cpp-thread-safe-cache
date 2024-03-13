//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mcp_event_trace_collector_events_i.h
 * Defines mcp ETC Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

// clang-format off

/**
 * Define Event Trace Providers for mcp
*/
FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_MCP_ETC, // ID
    MCP_ETC,                         // Name
    ET_MASK                          // Logging Level Mask
)

/**
 * Define Event Trace events for the MCP Main Provider
*/
typedef enum {
    MCP_ETC_EVENT_ID_COLLECTOR_INIT = 0,
} MCP_ETC_EVENT_ID;

// This event has no fields
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETC, // Provider ID
    MCP_ETC_EVENT_ID_COLLECTOR_INIT, // Event ID (per provider)
    McpEtcInit,                      // Event Name
    FPFW_ET_LEVEL_INFO               // Event Logging Level
)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
