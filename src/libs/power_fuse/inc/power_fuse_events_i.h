//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuse_events_i.h
 * Event traces for the power_fuse library
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                           PowerFuseLib,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     10,
                     PowerFuseReadFailedY_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     11,
                     PowerFuseReadFailedK_offset,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     12,
                     DTScoeffyValIsZeroAt,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, y_offset))
                
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,      
                    13,                  
                    RequestedFuseBitReadSizeInValid,                        
                    FPFW_ET_LEVEL_ERROR,                            
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT32, fuse_bit_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     14,
                     PlatformPowrFuseNotSupported,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     15,
                     PowerFuseReadTileCoeffDeafultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     16,
                     PowerFuseReadSoCTopCoeffDeafultValues,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     17,
                     PowerFuseReadFailedtileCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_POWER_FUSE_LIB,
                     18,
                     PowerFuseReadFailedSocTopCoeff,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))



/*--------- Function Prototypes ----------*/