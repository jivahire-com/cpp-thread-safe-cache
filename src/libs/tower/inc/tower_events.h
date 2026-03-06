//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_events.h
 * This file defines the event trace events for Tower
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,                                                                                                      // ID
    SCP_TOWER,                                                                                                                                 // Name
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS     // Logging Level Mask
)

/**
 * Define Event Trace events for the mesh
*/
typedef enum {
    TOWER_EVENT_ID_PLATFORM_NOT_SUPPORT = 0,
    TOWER_EVENT_ID_RAS_RECORD_TO_CPER_ERROR,
    TOWER_EVENT_ID_TOWER_RECORD_HANDLE_ERROR,
    TOWER_EVENT_ID_INVALID_TOWER_RECORD,
    TOWER_EVENT_ID_INVALID_ACCESS,
    TOWER_EVENT_ID_SDMSS_TOWER_ISOLATION_ENABLE,
    TOWER_EVENT_ID_SDMSS_TOWER_ISOLATION_DISABLE,
    TOWER_EVENT_ID_CDEDSS_TOWER_ISOLATION_ENABLE,
    TOWER_EVENT_ID_CDEDSS_TOWER_ISOLATION_DISABLE,

    TOWER_EVENT_ID_MAX
} TOWER_EVENT_ID;

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_PLATFORM_NOT_SUPPORT,
    TowerPlatformNotSupport,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_RAS_RECORD_TO_CPER_ERROR,
    TowerRasRecordToCperError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_TOWER_RECORD_HANDLE_ERROR,
    TowerTowerRecordHandleError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_INVALID_TOWER_RECORD,
    TowerInvalidTowerRecord,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_SDMSS_TOWER_ISOLATION_ENABLE,
    TowerSdmssIsolationEn,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_SDMSS_TOWER_ISOLATION_DISABLE,
    TowerSdmssIsolationDisable,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_CDEDSS_TOWER_ISOLATION_ENABLE,
    TowerCdedssIsolationEn,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_CDEDSS_TOWER_ISOLATION_DISABLE,
    TowerCdedssIsolationDisable,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_TOWER,
    TOWER_EVENT_ID_INVALID_ACCESS,
    TowerISRInvalidAccess,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
