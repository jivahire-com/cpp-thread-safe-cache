//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm_events.h
 * Event trace helpers for the Power PLDM service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_PLDM_ET_ERROR(type) EventWritePowerPldmErrorNoParam((type))

// clang-format off

/*-------------- Typedefs ----------------*/

typedef enum
{
    POWER_PLDM_ET_ID_ERROR_NOPARAMS,

    POWER_PLDM_ET_ID_COUNT
} POWER_PLDM_EVENT_ID;

typedef enum
{
    POWER_PLDM_ET_TYPE_PWR_CAP_REQUEST_ACTIVE,
    POWER_PLDM_ET_TYPE_PWR_THROT_REQUEST_ACTIVE,

    POWER_PLDM_ET_TYPE_COUNT
} POWER_PLDM_ET_TYPE_T;

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_POWER_PLDM,
                           PowerPldmSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
                               FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
                               FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_POWER_PLDM,
                     POWER_PLDM_ET_ID_ERROR_NOPARAMS,
                     PowerPldmErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*--------- Function Prototypes ----------*/

// clang-format on
