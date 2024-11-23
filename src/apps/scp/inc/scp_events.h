//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_events.h
 * Defines SCP App Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_MAIN, // ID
    SCP_MAIN,                         // Name
    ET_LEVEL_MASK_ALL                 // Logging Level Mask
)

/**
 * Define Event Trace events for the SCP Main Provider
*/
typedef enum {
    SCP_MAIN_EVENT_ID_HEARTBEAT = 0,
} SCP_MAIN_EVENT_ID;

// This event has no fields
FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_SCP_MAIN,
    SCP_MAIN_EVENT_ID_HEARTBEAT,
    ScpHeartBeat,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, rtos_ticks)
)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
