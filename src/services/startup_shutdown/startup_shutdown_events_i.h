//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_events_i.h
 * Event traces for the startup and shutdown service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                           StartupShutdownSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

//
// Info Events
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     0,
                     LocalCoreSyncStageStart,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     1,
                     LocalCoreSyncStageRemoteEnd,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, iterations))


//
// Verbose Events - Offset by 10
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     10,
                     LocalCoreSyncStageRemoteRead,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, remote_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, iteration))

/*--------- Function Prototypes ----------*/
