//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file vab_events.h
 * Defines VAB Driver Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define VAB_ET_ERROR(type)                EventWriteVabErrorNoParam((type))
#define VAB_ET_ERROR_PARAM(type, param)   EventWriteVabErrorParam((param), (type))
#define VAB_ET_WARNING(type)              EventWriteVabWarningNoParam((type))
#define VAB_ET_WARNING_PARAM(type, param) EventWriteVabWarningParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the VAB Driver Provider
*/
typedef enum {
    VAB_ET_ID_TYPE_ERROR_NOPARAMS,
    VAB_ET_ID_TYPE_ERROR_PARAMS,
    VAB_ET_ID_TYPE_WARNING_NOPARAMS,
    VAB_ET_ID_TYPE_WARNING_PARAMS,

    VAB_ET_ID_TYPE_COUNT
} VAB_EVENT_ID;

typedef enum
{
    VAB_ET_TYPE_RAS_NODE_HANDLER_ERROR,
    VAB_ET_TYPE_RAS_NODE_INVALID_RECORD,
    VAB_ET_TYPE_TBU_RECORD_HANDLER_ERROR,
    VAB_ET_TYPE_TBU_INVALID_RECORD,
    VAB_ET_TYPE_RPSS_RAS_NODE_HANDLER_ERROR,
    VAB_ET_TYPE_RPSS_RAS_NODE_INVALID_RECORD,
    VAB_ET_TYPE_RPSS_NO_VALID_INTERRUPT_SOURCE,
    VAB_ET_TYPE_RPSS_INTU0_UNSUPPORTED,
    VAB_ET_TYPE_RPSS_INTU1_UNSUPPORTED,
    VAB_ET_TYPE_SDMSS_INTU0_UNSUPPORTED,
    VAB_ET_TYPE_SDMSS_INTU1_UNSUPPORTED,
    VAB_ET_TYPE_CDEDSS_INTU0_UNSUPPORTED,
    VAB_ET_TYPE_CDEDSS_INTU1_UNSUPPORTED,
    VAB_ET_TYPE_INVALID_VAB_INSTANCE,
    VAB_ET_TYPE_NULL_INJECTION_PAYLOAD,
    VAB_ET_TYPE_UNEXPECTED_ERROR_DOMAIN,
    VAB_ET_TYPE_INVALID_COMPONENT_INSTANCE,
    VAB_ET_TYPE_FABRIC_PARITY_INJECTION_FAILED,
    VAB_ET_TYPE_PARITY_ERROR_ARMED,
    VAB_ET_TYPE_RAS_NODE_SPOOF_FAILED,
    VAB_ET_TYPE_INVALID_INJECTION_TARGET,

    VAB_ET_TYPE_COUNT
} VAB_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_VAB_DRIVER,
    Vab_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_VAB_DRIVER,
                     VAB_ET_ID_TYPE_ERROR_NOPARAMS,
                     VabErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_VAB_DRIVER,
                     VAB_ET_ID_TYPE_ERROR_PARAMS,
                     VabErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_VAB_DRIVER,
                     VAB_ET_ID_TYPE_WARNING_NOPARAMS,
                     VabWarningNoParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_VAB_DRIVER,
                     VAB_ET_ID_TYPE_WARNING_PARAMS,
                     VabWarningParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
