//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file i3c_controller_events.h
 * This file defines the event trace events for I3c Controller
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// clang-format off

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,                                                                       // ID
    COMMON_I3C_CONTROLLER,                                                                                               // Name
    FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS         // Logging Level Mask
)

/**
 * Define Event Trace events for the error domain
*/
typedef enum {
    I3C_CONTROLLER_EVENT_ID_PLATFORM_NOT_SUPPORT = 0,
    I3C_CONTROLLER_EVENT_ID_I3C_INIT_ERROR,
    I3C_CONTROLLER_EVENT_ID_I3C_SET_AASA_ERROR,
    I3C_CONTROLLER_EVENT_ID_I3C_READ_DIMM_ERROR,
    I3C_CONTROLLER_EVENT_ID_I3C_POWER_UP_PMIC_ERROR,
    I3C_CONTROLLER_EVENT_ID_DIMM_VERIFICATION_ERROR,

    I3C_CONTROLLER_EVENT_ID_MAX
} I3C_CONTROLLER_EVENT_ID;


FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_PLATFORM_NOT_SUPPORT,
    I3cControllerPlatformNotSupport,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_I3C_INIT_ERROR,
    I3cControllerInitError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, index),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_I3C_SET_AASA_ERROR,
    I3cControllerSetAASAError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_I3C_READ_DIMM_ERROR,
    I3cControllerReadError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_I3C_POWER_UP_PMIC_ERROR,
    I3cControllerPowerUpPmicError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_COMMON_I3C_CONTROLLER,
    I3C_CONTROLLER_EVENT_ID_DIMM_VERIFICATION_ERROR,
    I3cControllerDimmVerificationError,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, line)
)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// clang-format on
