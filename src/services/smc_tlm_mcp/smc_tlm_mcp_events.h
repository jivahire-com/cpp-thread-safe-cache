//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file smc_tlm_mcp_events.h
 * Event trace helpers for the SMC TLM MCP service.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace.h>
#include <event_trace_providers.h>
#include <IFpFwEventTraceProviders.h>

/*-- Symbolic Constant Macros (defines) --*/

#define AP_SMC_TELEMETRY_INFO(smc_fid, time_taken, core_id)     EventWriteApSmcTelemetry((smc_fid), (time_taken), (core_id))

/*-------------- Typedefs ----------------*/

/**
 * Event IDs for SMC TLM MCP service
 */
typedef enum {
    AP_SMC_TLM_EVENT = 0x0,  // SMC telemetry event with FID, core ID, and time
} AP_SMC_TLM_EVENT_ID;

/*-- Declarations (Statics and globals) --*/

/**
 * @brief Register the SMC TLM MCP event trace provider
 */
FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_SMC_TLM_MCP,
                           ApSmcTelemetrySvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
                               FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
                               FPFW_ET_LEVEL_MASK_ALWAYS);

/**
 * @brief Event trace for SMC telemetry - logs SMC FID, core ID, and execution time
 */
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_SMC_TLM_MCP,           // Provider ID
                     AP_SMC_TLM_EVENT,                                  // Event ID
                     ApSmcTelemetry,                                    // Event Name
                     FPFW_ET_LEVEL_INFO,                                // Log Level
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64_HEX, SMC_FID), // SMC Function ID
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, TIME_TAKEN),  // Time taken for SMC
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, CORE_ID))     // Core ID

/*--------- Function Prototypes ----------*/

// clang-format on
