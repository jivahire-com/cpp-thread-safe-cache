//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_manager_events.h
 * Event trace helpers for the SEL manager service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SEL_MANAGER_ET_ERROR_PARAM(type, param) EventWriteSelManagerErrorParam((param), (type))

// clang-format off

/*-------------- Typedefs ----------------*/
typedef enum
{
    SEL_MANAGER_ET_ID_ERROR_PARAMS,

    SEL_MANAGER_ET_ID_COUNT
} SEL_MANAGER_EVENT_ID;

typedef enum
{
    SEL_MANAGER_ET_TYPE_INVALID_ICC_CTX,
    SEL_MANAGER_ET_TYPE_SEND_EVENT_FAIL,
    SEL_MANAGER_ET_TYPE_TRANSFER_FAIL,
    SEL_MANAGER_ET_TYPE_QUEUE_FULL,
    SEL_MANAGER_ET_TYPE_FLUSH_QUEUE_FAIL,
    SEL_MANAGER_ET_TYPE_RECEIVE_FAIL,
    SEL_MANAGER_ET_TYPE_REPORT_BMC_PLDM_CC_FAIL,
    SEL_MANAGER_ET_TYPE_REPORT_BMC_STATUS_FAIL,

    SEL_MANAGER_ET_TYPE_COUNT
} SEL_MANAGER_ET_TYPE_T;

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_SEL_MANAGER,
                           SelManagerSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
                               FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
                               FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_SEL_MANAGER,
                     SEL_MANAGER_ET_ID_ERROR_PARAMS,
                     SelManagerErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*--------- Function Prototypes ----------*/

// clang-format on
