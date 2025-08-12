//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay.h
 *  Defines public types and methods for the event trace relay service.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <FpFwUtils.h>
#include <diag_decoder.h>
#include <in_band_telemetry_ddr.h>
#include <transfer_relay_protocol.h>
#include <tx_api.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/**
 * The Trace DDR Buffers in DDR are split into two types: ASIC and HSP. Each type contains the same
 * diagnostic decoder header, followed by type specific payloads.
*/

/**
 * Sizing and capacity for ASIC Buffers
 * - How big, including the Diag Header and the ASIC Header, the ASIC Buffer Payload is.
 * - How many ASIC Buffers we can fit into DDR.
*/
#define ASIC_BUFFER_PAYLOAD_SIZE (64 * FPFW_KB)
#define ASIC_BUFFER_DDR_CAPACITY_MAX (IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE / ASIC_BUFFER_PAYLOAD_SIZE)

/**
 * Sizing and capacity for HSP Buffers
 * - How big, including the Diag Header and any HSP Headers, the HSP Buffer Payload is.
 * - How many HSP Buffers we can fit into DDR.
 * @TODO: - This is a placeholder value, the actual size is TBD.
 *          https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1506825
*/
#define HSP_BUFFER_PAYLOAD_SIZE (16 * FPFW_KB)
#define HSP_BUFFER_DDR_CAPACITY_MAX (IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE / HSP_BUFFER_PAYLOAD_SIZE)

/**
 * Sizing and capacity for DDR Buffers
 * - How big, including the Diag Header and any HSP Headers, the DDR Buffer Payload is.
 * - How many DDR Buffers we can fit into DDR.
*/
#define ETR_CORE_BUFFERS_CAPACITY (8)
#define ETR_DDR_BUFFERS_CAPACITY_MAX (ASIC_BUFFER_DDR_CAPACITY_MAX + HSP_BUFFER_DDR_CAPACITY_MAX)

#define ETR_WORK_QUEUE_CAPACITY (ETR_CORE_BUFFERS_CAPACITY + ETR_DDR_BUFFERS_CAPACITY_MAX)

/*-------------------------------- Typedefs ---------------------------------*/

typedef struct
{
    uint64_t soc_id; // Wafer Lot Number and Wafer X/Y Coordinates
    uint32_t die_id;
} soc_info_t;

/* States for managing DDR buffers */
typedef enum {
    ETR_DDR_BUFFER_STATE_FREE,
    ETR_DDR_BUFFER_STATE_ACTIVE,
    ETR_DDR_BUFFER_STATE_PENDING,
    ETR_DDR_BUFFER_STATE_INVALID
} etr_ddr_buffer_state_t;

/* Payload data for managing DDR buffers */
typedef struct {
    uintptr_t base_addr;
    uint64_t size_bytes;
} payload_management_t, *p_payload_management_t;

typedef struct {
    diag_decoder_payload_header_t diag_header;
    FPFW_ET_ASIC_BUFFER_HEADER asic_header;
} asic_buffer_info_t, *p_asic_buffer_info_t;

typedef struct {
    diag_decoder_payload_header_t diag_header;
    uint8_t data[0];
} hsp_buffer_info_t, *p_hsp_buffer_info_t;

typedef struct {
    diag_payload_parser_type_t type;
    etr_ddr_buffer_state_t state;
    payload_management_t payload_management;
    union
    {
        asic_buffer_info_t asic;
        hsp_buffer_info_t hsp;
    } buffer;
} ddr_buffer_info_t, *p_ddr_buffer_info_t;

/* Types of requests the ETR Worker Thread Processes */
typedef enum
{
    ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER,
    ETR_SERVICE_REQUEST_TYPE_HOST_READ,
    ETR_SERVICE_REQUEST_TYPE_INVALID
} ETR_SERVICE_REQUEST_TYPE;

typedef struct {
    p_trp_msg_t p_trp_msg; // Pointer to the TRP message to process
    uintptr_t buffer_addr;
    uint32_t buffer_size_bytes; // Size of the buffer to copy
} etr_service_request_t, *p_etr_service_request_t;

/* ETR Service Context Structures */
typedef struct
{
    soc_info_t soc_info;
    p_ddr_buffer_info_t p_active_asic_buffer;
    FPFW_LOCK lock;
    /**
     * We keep track of all buffers in DDR
    */
    ddr_buffer_info_t ddr_buffers[ETR_DDR_BUFFERS_CAPACITY_MAX];
    TX_THREAD worker_thread;
    /**
     * Keep track of ICC Contexts not managed by MTS
     */
    struct {
        fpfw_icc_base_ctx_t* p_hsp_icc_ctx;
    } icc;
    /**
     * Keep track of unexpected but possible states
     */
    struct {
        uint64_t asic_buffers_reused;
        uint64_t delayed_host_reads;
    } health_stats;
} etr_service_context_t, *p_etr_service_context_t;

/* ETR Service Configuration Structure */
typedef struct
{
    soc_info_t soc_info;
    struct {
        uint64_t base_addr;
        uint64_t size_bytes;
    } asic_ddr_config;
    struct {
        uint64_t base_addr;
        uint64_t size_bytes;
    } hsp_ddr_config;
    struct
    {
        void* p_stack;              // Memory used to the worker thread stack
        size_t stack_size;          // Size of the provided memory
        uint32_t priority;          // ThreadX Thread priority
        uint32_t time_slice_option; // ThreadX Thread slicing option
    } thread_config;
    struct
    {
        fpfw_icc_base_ctx_t* p_hsp_icc_ctx; 
    } icc_config;
} etr_service_config_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 *  @brief  Initializes the Event Trace Relay service. Enabling core traces buffers to be copied to DDR.
 *
 *  @param[in]  p_context    This is the service context where the internal state will be stored for the module
 *                           NOTE! The provided memory is used only for lifetime of the program
 *
 *  @param[in]  p_config     This is the user configuration. Parameters are described in the documentation of the struct.
 *                           NOTE! The provided memory is used only for the lifetime of this function call.
 *
 *  @retval     None. This will attempt to create the threadx resources, the module will raise an error if any of these fail.
*/
void etr_initialize(etr_service_context_t* p_context, const etr_service_config_t* p_config);
