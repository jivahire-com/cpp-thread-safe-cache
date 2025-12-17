//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dma_events.h
 * Defines DMA Driver Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DMA_ET_ERROR(type)                EventWriteDmaErrorNoParam((type))
#define DMA_ET_ERROR_PARAM(type, param)   EventWriteDmaErrorParam((param), (type))
#define DMA_ET_WARNING(type)              EventWriteDmaWarningNoParam((type))
#define DMA_ET_WARNING_PARAM(type, param) EventWriteDmaWarningParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the DMA Driver Provider
*/
typedef enum {
    DMA_ET_ID_TYPE_ERROR_NOPARAMS,
    DMA_ET_ID_TYPE_ERROR_PARAMS,
    DMA_ET_ID_TYPE_WARNING_NOPARAMS,
    DMA_ET_ID_TYPE_WARNING_PARAMS,

    DMA_ET_ID_TYPE_COUNT

} DMA_EVENT_ID;

typedef enum
{
    DMA_ET_TYPE_LLI_INVALID,
    DMA_ET_TYPE_LLI_WR_SLV_ERR,
    DMA_ET_TYPE_LLI_RD_SLV_ERR,
    DMA_ET_TYPE_LLI_WR_DEC_ERR,
    DMA_ET_TYPE_LLI_RD_DEC_ERR,
    DMA_ET_TYPE_DST_SLV_ERR,
    DMA_ET_TYPE_SRC_SLV_ERR,
    DMA_ET_TYPE_CHANNEL_ABORTED,
    DMA_ET_TYPE_CHANNEL_DISABLED,
    DMA_ET_TYPE_CHANNEL_SUSPENDED,
    DMA_ET_TYPE_CHANNEL_LOCK_CLEARED,
    DMA_ET_TYPE_DST_TRANSCOMP,
    DMA_ET_TYPE_SRC_TRANSCOMP,
    DMA_ET_TYPE_BLOCK_TRANSCOMP,
    DMA_ET_TYPE_TRANSFER_COMPLETE,
    DMA_ET_TYPE_INVALID_INTERRUPT,

    DMA_ET_TYPE_COUNT
}   DMA_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_DMA_DRIVER,
    Dma_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_DMA_DRIVER,
                     DMA_ET_ID_TYPE_ERROR_NOPARAMS,
                     DmaErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_DMA_DRIVER,
                     DMA_ET_ID_TYPE_ERROR_PARAMS,
                     DmaErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_DMA_DRIVER,
                     DMA_ET_ID_TYPE_WARNING_NOPARAMS,
                     DmaWarningNoParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_DMA_DRIVER,
                     DMA_ET_ID_TYPE_WARNING_PARAMS,
                     DmaWarningParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
