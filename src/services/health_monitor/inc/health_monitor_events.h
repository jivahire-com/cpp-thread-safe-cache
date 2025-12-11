//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_events.h
 * Defines Health Monitor Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define HM_ET_INFO(type)                 EventWriteHealthMonitorInfoNoParam((type))
#define HM_ET_INFO_PARAM(type, param)    EventWriteHealthMonitorInfoParam((param), (type))
#define HM_ET_ERROR(type)                EventWriteHealthMonitorErrorNoParam((type))
#define HM_ET_ERROR_PARAM(type, param)   EventWriteHealthMonitorErrorParam((param), (type))
#define HM_ET_WARNING(type)              EventWriteHealthMonitorWarningNoParam((type))
#define HM_ET_WARNING_PARAM(type, param) EventWriteHealthMonitorWarningParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the Health Monitor Provider
*/
typedef enum {
    HM_ET_ID_TYPE_INFO_NOPARAMS,
    HM_ET_ID_TYPE_INFO_PARAMS,
    HM_ET_ID_TYPE_ERROR_NOPARAMS,
    HM_ET_ID_TYPE_ERROR_PARAMS,
    HM_ET_ID_TYPE_WARNING_NOPARAMS,
    HM_ET_ID_TYPE_WARNING_PARAMS,

    HM_ET_ID_TYPE_COUNT
} HM_EVENT_ID;

typedef enum
{
    HM_ET_TYPE_MAIN_GET_CONFIG,
    HM_ET_TYPE_MAIN_INVALID_PARAMS,
    HM_ET_TYPE_MAIN_SYNC_DIES,
    HM_ET_TYPE_ACCEL_ICC_TRANSFER,
    HM_ET_TYPE_ACCEL_CPER_ERROR,
    HM_ET_TYPE_ACCEL_REGISTRATION,
    HM_ET_TYPE_HSP_ICC_TRANSFER,
    HM_ET_TYPE_HSP_CPER_ERROR,
    HM_ET_TYPE_ED_HSP_REGISTRATION,
    HM_ET_TYPE_MCP_ICC_TRANSFER,
    HM_ET_TYPE_MCP_CPER_ERROR,
    HM_ET_TYPE_MCP_REGISTRATION,
    HM_ET_TYPE_REGISTER_INVALID_PARAMS,
    HM_ET_TYPE_INJECTION_INVALID_ACCESS,
    HM_ET_TYPE_INJECTION_ICC_TRANSFER,
    HM_ET_TYPE_CPER_INVALID_PARAMS,
    HM_ET_TYPE_CPER_CACHE_ERROR,
    HM_ET_TYPE_GHES_CONSTRUCT_TBL,
    HM_ET_TYPE_GHES_GET_CONFIG,
    HM_ET_TYPE_GHES_INVALID_PARAMS,
    HM_ET_TYPE_UE_INVALID_PARAMS,

    HM_ET_TYPE_COUNT
} HM_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
    HealthMonitor_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | 
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | 
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_INFO_NOPARAMS,
                     HealthMonitorInfoNoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_INFO_PARAMS,
                     HealthMonitorInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_ERROR_NOPARAMS,
                     HealthMonitorErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_ERROR_PARAMS,
                     HealthMonitorErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_WARNING_NOPARAMS,
                     HealthMonitorWarningNoParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_HEALTH_MONITOR,
                     HM_ET_ID_TYPE_WARNING_PARAMS,
                     HealthMonitorWarningParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
