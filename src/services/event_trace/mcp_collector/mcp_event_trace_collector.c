//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mcp_event_trace_collector.c
 *  This modules initializes and configures the ETC for the mcp Core.
 */

/*------------- Includes -----------------*/

#include "IFpFwEventTracingGeneration.h"        // for FPFW_ET_LOG
#include "mcp_event_trace_collector_events_i.h" // IWYU pragma: keep
#include "mcp_event_trace_collector_events_i.h" // for EventWriteMcpEtcInit

#include <IFpFwEventTracingBuffers.h>  // for PFPFW_ET_MANIFEST_ID
#include <build_data.h>                // for BUILD_ELF_SECTION_BI...
#include <event_trace_collector.h>     // for ETC_SERVICE_CORE_BUF...
#include <mcp_event_trace_collector.h> // for mcp_etc_initialize
#include <stdint.h>                    // for uintptr_t, uint8_t
#include <tx_api.h>                    // for TX_MINIMUM_STACK

/*-- Symbolic Constant Macros (defines) --*/

#define KB (1024)

// 200 B + 4 KB
#define MCP_ETC_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (KB)))

// (5 KB * Buffer Count (2)) + Reserved 6 KB == 10 KB + 6 KB == 16 KB (in DTCM)
#define MCP_ETC_RESERVED         ((6) * (KB))
#define MCP_ETC_CORE_BUFFER_SIZE ((5) * (KB))
#define MCP_ETC_CORE_BUFFERS_MEMORY_SIZE \
    (((MCP_ETC_CORE_BUFFER_SIZE) * (ETC_SERVICE_CORE_BUFFER_COUNT)) + (MCP_ETC_RESERVED))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern BUILD_ELF_SECTION_BINARY_METADATA g_BuildMetadata; // Per build version information

extern uint8_t _data_etproviderfilter_start; // Pointer to the start of the .ProviderFilter section
extern uint8_t _data_etproviderfilter_end;   // Pointer to the end   of the .ProviderFilter section

extern uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
extern uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
extern uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
extern uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

// Thread Stack
static uint8_t s_etc_stack[MCP_ETC_STACK_SIZE];

// Core Buffer Memory
static uint8_t s_trace_buffer[MCP_ETC_CORE_BUFFERS_MEMORY_SIZE];

static etc_service_context_t s_service_ctx = {0};

/*------------- Functions ----------------*/

void mcp_etc_initialize()
{
    // clang-format off
    etc_service_config_t config = {
        .mode = ETC_SERVICE_MODE_STDIO,
        .core_id = 0,
        .manifest_id = *(PFPFW_ET_MANIFEST_ID)&g_BuildMetadata.Id.Byte,
        .trace_buffer_memory = {
            .p_pool     = s_trace_buffer,
            .byte_count = sizeof(s_trace_buffer)
        },
        .thread_config = {
            .p_stack           = s_etc_stack,
            .stack_size        = sizeof(s_etc_stack),
            .priority          = 15,
            .time_slice_option = TX_NO_TIME_SLICE,
        },
        .event_manifest = {
            .provider_start = (uintptr_t)&_ProviderMetadata_et_msdata_start,
            .provider_end   = (uintptr_t)&_ProviderMetadata_et_msdata_end,
            .event_start    = (uintptr_t)&_EventMetadata_et_msdata_start,
            .event_end      = (uintptr_t)&_EventMetadata_et_msdata_end
        },
        .provider_filters = {
            .start = (uintptr_t)&_data_etproviderfilter_start,
            .end   = (uintptr_t)&_data_etproviderfilter_end,
        }
    };
    // clang-format on

    // Use the default request processing. Implement as needed.
    etc_initialize(&s_service_ctx, &config);

    FPFW_ET_LOG(McpEtcInit);
}
