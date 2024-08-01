/**
 * @file etc_etd_init.c
 * Instantiates Event Tracing Collector for the MCP and SCP cores
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h>                // for FPFW_KB
#include <IFpFwEventTracingBuffers.h> // for PFPFW_ET_MANIFEST_ID
#include <IFpFwEventTracingDefines.h> // for PFPFW_ET_EVENT_METADATA_HEADER
#include <IFpFwEventTracingEvents.h>  // for PFPFW_ET_PROVIDER_EVENT_FILTER
#include <build_data.h>               // for BUILD_ELF_SECTION_BINARY_METADATA
#include <event_trace_collector.h>    // for ETC_SERVICE_CORE_BUFFER_COUNT
#include <event_trace_decoder.h>      // for ETC_SERVICE_CORE_BUFFER_COUNT
#include <fpfw_init.h>                // for FPFW_INIT_STATUS_SUCCESS, FPFW...
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint8_t
#include <tx_api.h>                   // for TX_MINIMUM_STACK, TX_NO_TIME_S...

/*-- Symbolic Constant Macros (defines) --*/

// Thread Stack Sizes
#define ETD_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))
#define ETC_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

// Maximum number of single events to queue to the decoder
#define ETD_EVENT_QUOTA (30)

// Trace Buffer Sizes ==  (5 KB * Buffer Count (2)) + Reserved 6 KB == 10 KB + 6 KB == 16 KB
#define ETC_RESERVED         ((6) * (FPFW_KB))
#define ETC_CORE_BUFFER_SIZE ((5) * (FPFW_KB))
#define ETC_CORE_BUFFERS_MEMORY_SIZE \
    (((ETC_CORE_BUFFER_SIZE) * (ETC_SERVICE_CORE_BUFFER_COUNT)) + (ETC_RESERVED))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/**
 * Event Trace Decoder
 */
static etd_service_context_t s_etd_service_ctx = {0};
static etd_event_t s_etd_event_queue_memory[ETD_EVENT_QUOTA + FPFW_ET_BUFFER_COUNT];
static uint8_t s_etd_stack[ETD_STACK_SIZE];

// Metadata for decoding
extern uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
extern uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
extern uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
extern uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/**
 * Event Trace Collector
 */
static uint8_t s_etc_stack[ETC_STACK_SIZE];
static uint8_t s_etc_trace_buffer[ETC_CORE_BUFFERS_MEMORY_SIZE];
static etc_service_context_t s_etc_service_ctx = {0};

extern BUILD_ELF_SECTION_BINARY_METADATA g_BuildMetadata; // Per build version information
extern uint8_t _data_etproviderfilter_start; // Pointer to the start of the .ProviderFilter section
extern uint8_t _data_etproviderfilter_end;   // Pointer to the end   of the .ProviderFilter section

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(etd, FPFW_INIT_DEPENDENCIES("std_io"))
{
    etd_service_config_t config = {
        .event_queue_config = {.base_address = (uintptr_t)s_etd_event_queue_memory,
                               .byte_count = sizeof(s_etd_event_queue_memory),
                               .event_quota_size = ETD_EVENT_QUOTA},
        .thread_config =
            {
                .p_stack = s_etd_stack,
                .stack_size = sizeof(s_etd_stack),
                .priority = 15,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .event_manifest = {.provider_start = (PFPFW_ET_PROVIDER_METADATA_HEADER)&_ProviderMetadata_et_msdata_start,
                           .provider_end = (PFPFW_ET_PROVIDER_METADATA_HEADER)&_ProviderMetadata_et_msdata_end,
                           .event_start = (PFPFW_ET_EVENT_METADATA_HEADER)&_EventMetadata_et_msdata_start,
                           .event_end = (PFPFW_ET_EVENT_METADATA_HEADER)&_EventMetadata_et_msdata_end}};

    etd_initialize(&s_etd_service_ctx, &config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_etd_service_ctx};
}

FPFW_INIT_COMPONENT(etc, FPFW_INIT_DEPENDENCIES("etd"))
{

    etc_service_config_t config = {
        .mode = ETC_SERVICE_MODE_STDIO,
        .core_id = 0,
        .manifest_id = *(PFPFW_ET_MANIFEST_ID)&g_BuildMetadata.Id.Byte,
        .p_decoder_service = &s_etd_service_ctx,
        .trace_buffer_memory = {.p_pool = s_etc_trace_buffer, .byte_count = sizeof(s_etc_trace_buffer)},
        .thread_config =
            {
                .p_stack = s_etc_stack,
                .stack_size = sizeof(s_etc_stack),
                .priority = 10,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .provider_filters = {
            .start = (PFPFW_ET_PROVIDER_EVENT_FILTER)&_data_etproviderfilter_start,
            .end = (PFPFW_ET_PROVIDER_EVENT_FILTER)&_data_etproviderfilter_end,
        }};

    // Use the default request processing. Implement as needed.
    etc_initialize(&s_etc_service_ctx, &config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_etc_service_ctx};
}
