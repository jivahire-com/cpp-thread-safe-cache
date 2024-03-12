//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_event_trace_collector_events_i.h
 * Defines SCP ETC Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

// clang-format off

/**
 * Define Event Trace Providers for SCP
*/
FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_ETC, // ID
    SCP_ETC,                         // Name
    ET_MASK                          // Logging Level Mask
)

/**
 * Define Event Trace events for the SCP Main Provider
*/
typedef enum {
    SCP_ETC_EVENT_ID_COLLECTOR_INIT = 0,
} SCP_ETC_EVENT_ID;

// This event has no fields
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_ETC, // Provider ID
    SCP_ETC_EVENT_ID_COLLECTOR_INIT, // Event ID (per provider)
    ScpEtcInit,                      // Event Name
    FPFW_ET_LEVEL_INFO               // Event Logging Level
)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
