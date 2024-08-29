//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay.h
 *  Defines public types and methods for the event trace relay service.
 */

#pragma once

/*--------------- Includes ---------------*/

#include <ErrorHandler.h>
#include <FpFwLock.h>
#include <FpFwUtils.h>
#include <IFpFwEventTracingBuffers.h>
#include <in_band_telemetry_ddr.h>
#include <stddef.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

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
#define ASIC_BUFFER_DDR_CAPACITY_MAX (IB_TELEMETRY_DDR_DIE_TRACE_ASIC_SIZE / ASIC_BUFFER_PAYLOAD_SIZE)

/**
 * Sizing and capacity for HSP Buffers
 * - How big, including the Diag Header and any HSP Headers, the HSP Buffer Payload is.
 * - How many HSP Buffers we can fit into DDR.
 * @TODO: - This is a placeholder value, the actual size is TBD.
 *          https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1506825
*/
#define HSP_BUFFER_PAYLOAD_SIZE (16 * FPFW_KB)
#define HSP_BUFFER_DDR_CAPACITY_MAX (IB_TELEMETRY_DDR_DIE_TRACE_HSP_SIZE / HSP_BUFFER_PAYLOAD_SIZE)

#define ETR_CORE_BUFFERS_CAPACITY (8)
#define ETR_DDR_BUFFERS_CAPACITY_MAX (ASIC_BUFFER_DDR_CAPACITY_MAX + HSP_BUFFER_DDR_CAPACITY_MAX)

#define ETR_WORK_QUEUE_CAPACITY (ETR_CORE_BUFFERS_CAPACITY + ETR_DDR_BUFFERS_CAPACITY_MAX)

#define ETR_WORKER_THREAD_NAME ("etr-worker_thread")
#define ETR_WORK_POOL_NAME ("etr-work_pool")
#define ETR_WORK_QUEUE_NAME ("etr-work_queue")

/*-------------- Typedefs ----------------*/

typedef struct 
{
    uint64_t soc_id; // Wafer Lot Number and Wafer X/Y Coordinates
    uint32_t die_id;
} soc_info_t;

//
// Structures for managing DDR buffers
//

typedef enum {
    ETR_DDR_BUFFER_STATE_FREE,
    ETR_DDR_BUFFER_STATE_ACTIVE,
    ETR_DDR_BUFFER_STATE_PENDING,
    ETR_DDR_BUFFER_STATE_INVALID
} etr_ddr_buffer_state_t;

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
    diag_decoder_strategy_id_t type;
    etr_ddr_buffer_state_t state;
    payload_management_t payload_management;
    union
    {
        asic_buffer_info_t asic;
        hsp_buffer_info_t hsp;
    } buffer;
} ddr_buffer_info_t, *p_ddr_buffer_info_t;

//
// Types of requests the ETR Worker Thread Processes
//

typedef enum
{
    ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER,
    ETR_SERVICE_REQUEST_TYPE_HOST_READ,
    ETR_SERVICE_REQUEST_TYPE_INVALID
} ETR_SERVICE_REQUEST_TYPE;

typedef struct {
    uint32_t core_id;
    uintptr_t buffer_addr;
    FPFW_ET_CORE_BUFFER_HEADER buffer_header;
} etr_service_copy_buffer_request_t, *p_etr_service_copy_buffer_request_t;

typedef struct {
    uint32_t die_id;
    payload_management_t payload_management;
} etr_service_host_read_request_t, *p_etr_service_host_read_request_t;

typedef struct {
    ETR_SERVICE_REQUEST_TYPE type;
    union {
        etr_service_copy_buffer_request_t copy_buffer;
        etr_service_host_read_request_t host_read;
    } request;
} etr_service_request_t, *p_etr_service_request_t;

//
// ETR Service Context and Configuration
//

typedef struct 
{
    soc_info_t soc_info;
    p_ddr_buffer_info_t p_active_asic_buffer;
    FPFW_LOCK lock;
    /**
     * We keep track of all buffers in DDR
    */
    ddr_buffer_info_t ddr_buffers[ETR_DDR_BUFFERS_CAPACITY_MAX];
    /**
     * To utilize the threadx queue for thread processing we need a
     * block pool to allocate requests from, to then queue them. Due
     * to threadx queues having a limit on the queue entry size.
    */
    struct {
        TX_BLOCK_POOL pool;
        etr_service_request_t pool_memory[ETR_WORK_QUEUE_CAPACITY];
        TX_QUEUE queue;
        void* queue_memory[ETR_WORK_QUEUE_CAPACITY];
    } request_queue;
    TX_THREAD worker_thread;
    /**
     * Keep track of unexpected but possible states
     */
    struct {
        uint64_t asic_buffers_reused;
        uint64_t delayed_host_reads;
    } health_stats;
} etr_service_context_t, *p_etr_service_context_t;



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
} etr_service_config_t;

/*--------- Function Prototypes ----------*/

/**
 *  @brief  Initializes the Event Trace Relay service. Enabling core traces buffers to be copied to DDR.
 * 
 *  @param[in]  p_service    This is the service context where the internal state will be stored for the module
 *                           NOTE! The provided memory is used only for lifetime of the program
 * 
 *  @param[in]  p_config     This is the user configuration. Parameters are described in the documentation of the struct.
 *                           NOTE! The provided memory is used only for the lifetime of this function call.
 * 
 *  @retval     None. This will attempt to create the threadx resources, the module will raise an error if any of these fail.
*/
void etr_initialize(etr_service_context_t* p_service, const etr_service_config_t* p_config);
