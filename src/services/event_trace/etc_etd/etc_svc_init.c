//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file etc_svc_init.c
 * Instantiates the the Event Trace Collection (ETC) Service.
 *
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwUtils.h>                // for FPFW_KB
#include <IFpFwEventTracing.h>        // for FPFwETGetController, FPFwETControllerFlushBuffer
#include <bug_check.h>                // for BUG_ASSERT
#include <build_data.h>               // for BUILD_ELF_SECTION_BINARY_METADATA
#include <crash_dump.h>               // for crash_dump_register_address32
#include <et_mts_client.h>            // for et_mts_notify_buffer_complete
#include <etc_etd_svc.h>              // for etc_svc_init
#include <gtimer_prodfw.h>            // for gtimer_prodfw_get_counter
#include <idsw.h>                     // for idsw_get_cpu_type
#include <idsw_kng.h>                 // for CPU_MCP, CPU_SCP
#include <mscp_exp_rmss_memory_map.h> // for ETC_CORE_BUFFERS_MEMORY_SIZE
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint8_t
#include <tx_api.h>                   // for TX_MINIMUM_STACK, TX_NO_TIME_SLICE

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define ETC_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint8_t s_etc_stack[ETC_STACK_SIZE];
static etc_service_context_t s_etc_service_ctx = {0};

extern uint8_t _data_etproviderfilter_start; // Pointer to the start of the .ProviderFilter section
extern uint8_t _data_etproviderfilter_end;   // Pointer to the end   of the .ProviderFilter section

static etc_service_config_t etc_config = {
    .mode = ETC_SERVICE_MODE_STDIO,
    .p_process_request = {.func = (ETC_PROCESS_REQUEST_FUNC)et_mts_notify_buffer_complete},
    .process_request_type = ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER,
    .thread_config =
        {
            .p_stack = s_etc_stack,
            .stack_size = sizeof(s_etc_stack),
            .priority = 10,
            .time_slice_option = TX_NO_TIME_SLICE,
        },
    .provider_filters =
        {
            .start = (PFPFW_ET_PROVIDER_EVENT_FILTER)&_data_etproviderfilter_start,
            .end = (PFPFW_ET_PROVIDER_EVENT_FILTER)&_data_etproviderfilter_end,
        },
    .callbacks =
        {
            .get_ticks_cb = gtimer_get_timestamp_ms, // Use the system time in milliseconds
        },
};

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
void etc_svc_init(void)
{
    uint8_t cpu_type = idsw_get_cpu_type();

    if (cpu_type == CPU_MCP)
    {
        etc_config.trace_buffer_memory.p_pool = (void*)SCP_EXP_MCP_TRACE_BUFFER_BASE;
        etc_config.trace_buffer_memory.byte_count = SCP_EXP_MCP_TRACE_BUFFER_SIZE;
    }
    else if (cpu_type == CPU_SCP)
    {
        etc_config.trace_buffer_memory.p_pool = (void*)SCP_EXP_SCP_TRACE_BUFFER_BASE;
        etc_config.trace_buffer_memory.byte_count = SCP_EXP_SCP_TRACE_BUFFER_SIZE;
    }
    else
    {
        BUG_ASSERT(false);
    }

    etc_config.core_id = cpu_type;
    etc_config.p_decoder_service = get_etd_service_context();

    // the gnu build id is unique per core.  Use the first 16 bytes for the manifest id which needs to be
    // unique for the diagnostic decoder tool to decode the data
    memcpy((void*)&etc_config.manifest_id, (void*)g_note_gnu_build_id.BuildId, sizeof(etc_config.manifest_id));

    etc_initialize(&s_etc_service_ctx, &etc_config);
}

uint32_t get_etc_buffer_size(void)
{
    return (uint32_t)(etc_config.trace_buffer_memory.byte_count / ETC_SERVICE_CORE_BUFFER_COUNT);
}

uint8_t* get_etc_buffer_byte_pool_address(void)
{
    return (uint8_t*)etc_config.trace_buffer_memory.p_pool;
}

etc_service_context_t* get_etc_service_context(void)
{
    return &s_etc_service_ctx;
}