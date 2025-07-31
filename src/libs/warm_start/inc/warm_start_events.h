//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_events.h
 * Defines Warm Start Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define WS_ET_INFO(type)                 EventWriteWarmStartInfoNoParam((type))
#define WS_ET_INFO_PARAM(type, param)    EventWriteWarmStartInfoParam((param), (type))
#define WS_ET_ERROR(type)                EventWriteWarmStartErrorNoParam((type))
#define WS_ET_ERROR_PARAM(type, param)   EventWriteWarmStartErrorParam((param), (type))
#define WS_ET_WARNING(type)              EventWriteWarmStartWarningNoParam((type))
#define WS_ET_WARNING_PARAM(type, param) EventWriteWarmStartWarningParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the Warm Start Provider
*/
typedef enum {
    WS_ET_ID_TYPE_INFO_NOPARAMS,
    WS_ET_ID_TYPE_INFO_PARAMS,
    WS_ET_ID_TYPE_ERROR_NOPARAMS,
    WS_ET_ID_TYPE_ERROR_PARAMS,
    WS_ET_ID_TYPE_WARNING_NOPARAMS,
    WS_ET_ID_TYPE_WARNING_PARAMS,

    WS_ET_ID_TYPE_COUNT
} WS_EVENT_ID;

typedef enum
{
    WS_ET_TYPE_INIT_DONE,
    WS_ET_TYPE_DATA_PUT_ID,
    WS_ET_TYPE_DATA_GET_ID,
    WS_ET_TYPE_DATA_NO_SPACE_AVAILABLE,
    WS_ET_TYPE_DATA_FOUND,
    WS_ET_TYPE_DATA_REUSED,
    WS_ET_TYPE_LIST_INIT,
    WS_ET_TYPE_DATA_INSERT,

    WS_ET_TYPE_COUNT
} WS_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
    WarmStart_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | 
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | 
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_INFO_NOPARAMS,
                     WarmStartInfoNoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_INFO_PARAMS,
                     WarmStartInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_ERROR_NOPARAMS,
                     WarmStartErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_ERROR_PARAMS,
                     WarmStartErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_WARNING_NOPARAMS,
                     WarmStartWarningNoParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_WARM_START,
                     WS_ET_ID_TYPE_WARNING_PARAMS,
                     WarmStartWarningParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
