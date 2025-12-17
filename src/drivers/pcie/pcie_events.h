//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_events.h
 * Defines PCIe Driver Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PCIE_ET_INFO_PARAM(type, param) EventWritePcieInfoParam((param), (type))
#define PCIE_ET_ERROR_PARAM(type, param) EventWritePcieErrorParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the PCIe Driver Provider
*/
typedef enum {
    PCIE_ET_ID_TYPE_INFO_PARAMS,
    PCIE_ET_ID_TYPE_ERROR_PARAMS,

    PCIE_ET_ID_TYPE_COUNT
} PCIE_EVENT_ID;

typedef enum
{
    PCIE_ET_TYPE_BDAT_ATU_MAP_FAILED,
    PCIE_ET_TYPE_ENQUEUE_REQUEST_FAILED,
    PCIE_ET_TYPE_UNKNOWN_REQUEST,
    PCIE_ET_TYPE_INVALID_RP_RANGES,
    PCIE_ET_TYPE_CLI_REQ_NULL,
    PCIE_ET_TYPE_UNKNOWN_CLI_OP,
    PCIE_ET_TYPE_INVALID_RPSS_ID_CONFIG,
    PCIE_ET_TYPE_INVALID_RPSS_ID_REINIT,
    PCIE_ET_TYPE_INVALID_RPSS_ID_SHUTDOWN,
    PCIE_ET_TYPE_INVALID_RPSS_ID_WORKAROUND,
    PCIE_ET_TYPE_SYNC_REQUEST_OUT_OF_BOUNDS,
    PCIE_ET_TYPE_BAD_SYNC_REQUEST,
    PCIE_ET_TYPE_BDAT_PUBLISH_FAILED,
    PCIE_ET_TYPE_RPSS_IRQ_CALLBACK,
    PCIE_ET_TYPE_INT_GLOBAL_IDE,
    PCIE_ET_TYPE_INT_AES_HCFG,
    PCIE_ET_TYPE_INT_LINK_DOWN,
    PCIE_ET_TYPE_INT_LINK_UP,
    PCIE_ET_TYPE_INT_DTIM,
    PCIE_ET_TYPE_INT_LTIM,
    PCIE_ET_TYPE_INT_RASDP,
    PCIE_ET_TYPE_INT_DPC,
    PCIE_ET_TYPE_INT_UNEXPECTED,

    PCIE_ET_TYPE_COUNT
} PCIE_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_PCIE_DRIVER,
    Pcie_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_PCIE_DRIVER,
                     PCIE_ET_ID_TYPE_INFO_PARAMS,
                     PcieInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_PCIE_DRIVER,
                     PCIE_ET_ID_TYPE_ERROR_PARAMS,
                     PcieErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
