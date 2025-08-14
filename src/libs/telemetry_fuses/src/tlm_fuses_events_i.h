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
#include <tlm_fuses.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                           TlmFusesLib,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);


//
// ALWAYS
//
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     0,
                     TlmFusesReadEcid,
                     FPFW_ET_LEVEL_ALWAYS,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_0),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_1),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_2),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_3),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_4),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_5),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_6),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_7),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_8),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, wafer_num),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, x_coord),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, y_coord))

//
// ERRORS
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     10,
                     TlmFusesReadFailedY_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     11,
                     TlmFusesReadFailedK_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     12,
                     TlmFusesDTScoeffyValIsZeroAt,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, y_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     13,
                     TlmFusesDTScoeffkValIsZeroAt,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, k_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                    14,
                    TlmFusesRequestedFuseBitReadSizeInValid,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, fuse_bit_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     15,
                     TlmFusesNotSupported,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     16,
                     TlmFusesReadTileCoeffDefaultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     17,
                     TlmFusesReadSoCTopCoeffDefaultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     18,
                     TlmFusesReadFailedtileCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     19,
                     TlmFusesReadFailedSocTopCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     20,
                     TlmFusesReadEcidFuseServiceNotUp,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     21,
                     TlmFusesReadEcidFailed,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     22,
                     TlmFusesReadEcidWaferLotNum,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, char_index))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     23,
                     TlmFusesReadEcidWaferNum,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     24,
                     TlmFusesReadEcidXCoord,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_FUSES_LIB,
                     25,
                     TlmFusesReadEcidYCoord,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

/*--------- Function Prototypes ----------*/