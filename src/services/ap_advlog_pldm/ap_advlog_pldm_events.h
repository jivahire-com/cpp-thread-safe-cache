//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_advlog_pldm_events.h
 * Event trace helpers for the AP Advlog PLDM service.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/

#define AP_ADVLOG_PLDM_ET_INFO(type)                  EventWriteApAdvlogPldmInfoNoParam((type))
#define AP_ADVLOG_PLDM_ET_INFO_PARAM(type, param)     EventWriteApAdvlogPldmInfoParam((param), (type))
#define AP_ADVLOG_PLDM_ET_ERROR(type)                 EventWriteApAdvlogPldmErrorNoParam((type))
#define AP_ADVLOG_PLDM_ET_ERROR_PARAM(type, param)    EventWriteApAdvlogPldmErrorParam((param), (type))

// clang-format off

/*-------------- Typedefs ----------------*/

typedef enum
{
    AP_ADVLOG_PLDM_ET_ID_INFO_NOPARAMS,
    AP_ADVLOG_PLDM_ET_ID_INFO_PARAMS,
    AP_ADVLOG_PLDM_ET_ID_ERROR_NOPARAMS,
    AP_ADVLOG_PLDM_ET_ID_ERROR_PARAMS,

    AP_ADVLOG_PLDM_ET_ID_COUNT
} AP_ADVLOG_PLDM_EVENT_ID;

typedef enum
{
    AP_ADVLOG_PLDM_ET_TYPE_SIGNATURE_MISMATCH,
    AP_ADVLOG_PLDM_ET_TYPE_SIZE_EXCEEDED,
    AP_ADVLOG_PLDM_ET_TYPE_CORRUPTED,
    AP_ADVLOG_PLDM_ET_TYPE_DUMP_TRANSFER_FAIL,
    AP_ADVLOG_PLDM_ET_TYPE_EFFECTER_COMPLETE_FAIL,
    AP_ADVLOG_PLDM_ET_TYPE_INFO_RETRIEVE_FAIL,
    AP_ADVLOG_PLDM_ET_TYPE_LOGDUMP_IN_PROGRESS,
    AP_ADVLOG_PLDM_ET_TYPE_RAISE_PPE_FAIL,
    AP_ADVLOG_PLDM_ET_TYPE_READY,
    AP_ADVLOG_PLDM_ET_TYPE_START_TRANSFER,
    AP_ADVLOG_PLDM_ET_TYPE_EFFECTER_COMPLETE,
    AP_ADVLOG_PLDM_ET_TYPE_TRANSFER_COMPLETE,

    AP_ADVLOG_PLDM_ET_TYPE_COUNT
} AP_ADVLOG_PLDM_ET_TYPE_T;

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_AP_ADVLOG_PLDM,
                           ApAdvlogPldmSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
                               FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
                               FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_AP_ADVLOG_PLDM,
                     AP_ADVLOG_PLDM_ET_ID_INFO_NOPARAMS,
                     ApAdvlogPldmInfoNoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_AP_ADVLOG_PLDM,
                     AP_ADVLOG_PLDM_ET_ID_INFO_PARAMS,
                     ApAdvlogPldmInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_AP_ADVLOG_PLDM,
                     AP_ADVLOG_PLDM_ET_ID_ERROR_NOPARAMS,
                     ApAdvlogPldmErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_AP_ADVLOG_PLDM,
                     AP_ADVLOG_PLDM_ET_ID_ERROR_PARAMS,
                     ApAdvlogPldmErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*--------- Function Prototypes ----------*/

// clang-format on
