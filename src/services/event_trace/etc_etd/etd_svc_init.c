//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file etd_svc_init.c
 * Instantiates the the Event Trace Decoder (ETD) Service.
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwUtils.h>                // for FPFW_KB
#include <IFpFwEventTracingDefines.h> // for PFPFW_ET_EVENT_METADATA_HEADER
#include <IFpFwEventTracingEvents.h>  // for PFPFW_ET_PROVIDER_EVENT_FILTER
#include <etc_etd_svc.h>              // for etd_svc_init
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint8_t
#include <tx_api.h>                   // for TX_MINIMUM_STACK, TX_NO_TIME_S...

/*------------------- Symbolic Constant Macros (defines) --------------------*/

// Thread Stack Sizes
#define ETD_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

// Maximum number of single events to queue to the decoder
#define ETD_EVENT_QUOTA (30)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static etd_service_context_t s_etd_service_ctx = {0};
static etd_event_t s_etd_event_queue_memory[ETD_EVENT_QUOTA + FPFW_ET_BUFFER_COUNT];
static uint8_t s_etd_stack[ETD_STACK_SIZE];

// Metadata for decoding
extern uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
extern uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
extern uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
extern uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
void etd_svc_init(void)
{
    etd_service_config_t etd_config = {
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

    etd_initialize(&s_etd_service_ctx, &etd_config);
}

etd_service_context_t* get_etd_service_context(void)
{
    return &s_etd_service_ctx;
}