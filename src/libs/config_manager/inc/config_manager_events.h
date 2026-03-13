//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file config_manager_events.h
 * This file defines the event trace events for Config Manager
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_CONFIG_MANAGER,                                                                            // ID
    COMMON_CONFIG_MANAGER,                                                                                                    // Name
    FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS              // Logging Level Mask
)

/**
 * Define Event Trace events for the common config manager
*/
typedef enum {
    COMMON_CONFIG_MANAGER_EVENT_ID_PLATFORM_NOT_SUPPORT = 0,
    COMMON_CONFIG_MANAGER_EVENT_ID_VAR_STORE_INVALID_DATA,
    COMMON_CONFIG_MANAGER_EVENT_ID_VAR_STORE_ACCESS_FAILED,

    COMMON_CONFIG_MANAGER_EVENT_ID_MAX
} COMMON_CONFIG_MANAGER_EVENT_ID;


FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_CONFIG_MANAGER,
    COMMON_CONFIG_MANAGER_EVENT_ID_PLATFORM_NOT_SUPPORT,
    ConfigManagerPlatformNotSupport,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_CONFIG_MANAGER,
    COMMON_CONFIG_MANAGER_EVENT_ID_VAR_STORE_INVALID_DATA,
    ConfigManagerVarStoreInvalidData,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, index),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_CONFIG_MANAGER,
    COMMON_CONFIG_MANAGER_EVENT_ID_VAR_STORE_ACCESS_FAILED,
    ConfigManagerVarStoreAccessFailed,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, index),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)


/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
