//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_fuses_events_i.h
 * Event traces for the telemetry fuse library
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                           TlmFusesLib,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     10,
                     TlmFusesReadFailedY_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     11,
                     TlmFusesReadFailedK_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     12,
                     TlmFusesDTScoeffyValIsZeroAt,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, y_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     13,
                     TlmFusesDTScoeffkValIsZeroAt,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, k_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                    14,
                    TlmFusesRequestedFuseBitReadSizeInValid,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, fuse_bit_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     15,
                     TlmFusesNotSupported,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     16,
                     TlmFusesReadTileCoeffDefaultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     17,
                     TlmFusesReadSoCTopCoeffDefaultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     18,
                     TlmFusesReadFailedtileCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     19,
                     TlmFusesReadFailedSocTopCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

/*--------- Function Prototypes ----------*/