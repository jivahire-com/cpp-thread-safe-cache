//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_events.h
 * This file defines the event trace events for Variable Services
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_VARIABLE_SERVICES,                                                                                          // ID
    COMMON_VARIABLE_SERVICES,                                                                                                                  // Name
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS     // Logging Level Mask
)

/**
 * Define Event Trace events for the mesh
*/
typedef enum {
    VARIABLE_SERVICES_EVENT_ID_ICC_SEND_ERROR = 0,

    VARIABLE_SERVICES_EVENT_ID_MAX
} VARIABLE_SERVICES_EVENT_ID;

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_VARIABLE_SERVICES,
    VARIABLE_SERVICES_EVENT_ID_ICC_SEND_ERROR,
    VariableServicesAsyncIccSendError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, icc_status),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)


/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
