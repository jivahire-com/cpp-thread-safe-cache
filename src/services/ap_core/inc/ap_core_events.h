//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_events.h
 * Defines AP CORE Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define APCORE_ET_FW_LOAD_INFO(type, param) EventWriteApCoreFWLoadInfo((type), (param))
// clang-format off

/**
 * Define Event Trace events for the AP CORE
*/
typedef enum {
    APCORE_ET_ID_TYPE_FW_LOAD_INFO,
    APCORE_ET_ID_TYPE_MAX_COUNT
} APCORE_EVENT_ID;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_AP_CORE,
    ApCoreSvc_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | 
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | 
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_AP_CORE,
                     APCORE_ET_ID_TYPE_FW_LOAD_INFO, 
                     ApCoreFWLoadInfo,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, type), 
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
