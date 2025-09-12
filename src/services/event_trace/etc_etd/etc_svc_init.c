//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file etc_svc_init.c
 * Instantiates the the Event Trace Collection (ETC) Service.
 *
 */

/*-------------------------------- Includes ---------------------------------*/
#include <DbgPrint.h>      // for FPFW_DBGPRINT_INFO
#include <FpFwUtils.h>     // for FPFW_KB
#include <build_data.h>    // for BUILD_ELF_SECTION_BINARY_METADATA
#include <etc_etd_svc.h>   // for etc_svc_init
#include <gtimer_prodfw.h> // for gtimer_prodfw_get_counter
#include <idsw.h>          // for idsw_get_cpu_type
#include <stddef.h>        // for NULL
#include <stdint.h>        // for uint8_t
#include <tx_api.h>        // for TX_MINIMUM_STACK, TX_NO_TIME_SLICE

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define ETC_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint8_t s_etc_stack[ETC_STACK_SIZE];
static uint8_t s_etc_trace_buffer[ETC_CORE_BUFFERS_MEMORY_SIZE];
static etc_service_context_t s_etc_service_ctx = {0};

extern uint8_t _data_etproviderfilter_start; // Pointer to the start of the .ProviderFilter section
extern uint8_t _data_etproviderfilter_end;   // Pointer to the end   of the .ProviderFilter section

/*----------------------------- Static Functions ----------------------------*/
/*------------- Functions ----------------*/
uint64_t get_counter_ms()
{
    return 1000 * gtimer_prodfw_get_counter() / gtimer_prodfw_get_frequency();
}

/*----------------------------- Global Functions ----------------------------*/
void etc_svc_init(void)
{
    etc_service_config_t etc_config = {
        .mode = ETC_SERVICE_MODE_STDIO,
        .trace_buffer_memory = {.p_pool = s_etc_trace_buffer, .byte_count = sizeof(s_etc_trace_buffer)},
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
                .get_ticks_cb = get_counter_ms, // Use the system time in milliseconds
            },
    };

    etc_config.core_id = idsw_get_cpu_type();
    etc_config.p_decoder_service = get_etd_service_context();

    // the gnu build id is unique per core.  Use the first 16 bytes for the manifest id which needs to be
    // unique for the diagnostic decoder tool to decode the data
    memcpy((void*)&etc_config.manifest_id, (void*)g_note_gnu_build_id.BuildId, sizeof(etc_config.manifest_id));

    etc_initialize(&s_etc_service_ctx, &etc_config);
}

uint32_t get_etc_buffer_size(void)
{
    return (uint32_t)ETC_CORE_BUFFER_SIZE;
}

void* get_etc_buffer_address(void)
{
    return (uint8_t*)&s_etc_trace_buffer;
}

etc_service_context_t* get_etc_service_context(void)
{
    return &s_etc_service_ctx;
}