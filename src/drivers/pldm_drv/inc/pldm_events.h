//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pldm_events.h
 * Defines PLDM Driver Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PLDM_ET_ERROR_PARAM(type, param) EventWritePldmErrorParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the PLDM Driver Provider
*/
typedef enum {
    PLDM_ET_ID_TYPE_ERROR_PARAMS,

    PLDM_ET_ID_TYPE_COUNT
} PLDM_EVENT_ID;

typedef enum
{
    PLDM_ET_TYPE_RAISE_PLATFORM_EVENT_FAILED,
    PLDM_ET_TYPE_UNKNOWN_REQUEST_TYPE,

    PLDM_ET_TYPE_COUNT
} PLDM_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_PLDM_DRIVER,
    Pldm_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_PLDM_DRIVER,
                     PLDM_ET_ID_TYPE_ERROR_PARAMS,
                     PldmErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on