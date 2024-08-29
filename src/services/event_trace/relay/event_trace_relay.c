//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay.c
 *  This modules initializes and configures event trace relay for a die.
 */

/*------------- Includes -----------------*/

#include "event_trace_relay_i.h"

#include <ErrorHandler.h>
#include <FpFwAssert.h>
#include <FpFwLock.h>
#include <FpFwUtils.h>
#include <IFpFwEventTracingBuffers.h>
#include <IFpFwEventTracingStatus.h>
#include <event_trace_relay.h>
#include <in_band_telemetry_ddr.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tx_api.h>
#include <tx_thread.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

// Initialize etr components
static void etr_initialize_ddr_buffers(etr_service_context_t* p_service, const etr_service_config_t* p_config);
static void etr_initialize_worker_thread(etr_service_context_t* p_service, const etr_service_config_t* p_config);

// Thread and request processing
static void etr_worker_thread_func(ULONG thread_input);
static void etr_handle_copy_buffer_request(etr_service_context_t* p_service, etr_service_request_t* p_request);
static void etr_handle_host_read_request(etr_service_context_t* p_service, etr_service_request_t* p_request);

// ICC
static void etr_notify_etdcc(ddr_buffer_info_t* p_buffer);
static void etr_notify_etc(etr_service_request_t* p_request);

// ASIC Buffer Management
static void etr_get_new_asic_buffer(etr_service_context_t* p_service);
static void etr_complete_asic_buffer(etr_service_context_t* p_service);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * Public Functions
 */
void etr_initialize(etr_service_context_t* p_service, const etr_service_config_t* p_config)
{
    if (NULL == p_service || NULL == p_config)
    {
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG, 0, 0, 0, 0);
    }

    // Initialize the queue lock
    FpFwLockInitialize(&p_service->lock);

    // Initialize the ddr buffer management
    etr_initialize_ddr_buffers(p_service, p_config);

    // Initialize the worker thread
    etr_initialize_worker_thread(p_service, p_config);
}

/**
 * Private Helper Functions
 */

//
// Helper Function: Sends a request to the Event Trace Data Collection Client that a buffer
//                  is ready to be read by the host.
//
static void etr_notify_etdcc(ddr_buffer_info_t* p_buffer)
{
    // @TODO: - Implement alongside the ETDCC. Requires D2D communication for ETR on DIE 1.
    //          Needs loopback to MCP for the ETR on DIE 0.
    //          https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1925374
    FPFW_UNUSED(p_buffer);
}

//
// Helper Function: Notify an Event Trace Collector that it's buffer has been copied
//
static void etr_notify_etc(etr_service_request_t* p_request)
{
    // @TODO: BPK - Implement once ICC to MCP/SCP/SDM/CDED is available with loopback to MCP
    //              https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1743379
    FPFW_UNUSED(p_request);
}

//
// Helper Function: Get a new ASIC buffer from the DDR buffer pool
//
static void etr_get_new_asic_buffer(etr_service_context_t* p_service)
{
    // Get a new buffer
    bool free_buffer_found = false;

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_service->lock);
    // walk the buffer array to find the next free buffer
    for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
    {
        if (p_service->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_FREE &&
            p_service->ddr_buffers[i].type == DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_DEVICE)
        {
            free_buffer_found = true;
            p_service->p_active_asic_buffer = &p_service->ddr_buffers[i];
            break;
        }
    }

    // If we didn't find a free buffer then we need to re-use one that is pending
    if (!free_buffer_found)
    {
        for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
        {
            if (p_service->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING &&
                p_service->ddr_buffers[i].type == DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_DEVICE)
            {
                p_service->p_active_asic_buffer = &p_service->ddr_buffers[i];
                p_service->health_stats.asic_buffers_reused++;
                break;
            }
        }
    }

    p_service->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;
    p_service->p_active_asic_buffer->payload_management.size_bytes = sizeof(asic_buffer_info_t);
    p_service->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes = sizeof(FPFW_ET_ASIC_BUFFER_HEADER);
    FpFwLockRelease(&p_service->lock, lock_state);
}

//
// Helper function: Fill in any missing header information and notify the ETDCC that the buffer
//                  is ready to be read by the host.
//
static void etr_complete_asic_buffer(etr_service_context_t* p_service)
{

    /**
     * When completing an ASIC buffer we need to do a few things:
     *
     * 1. Update the current buffers state
     * 2. Update the header information in DDR
     * 3. Grab a new asic buffer
     * 4. Notify the ETDCC that the finalized buffer is ready to be read
     */

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_service->lock);
    p_service->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
    FpFwLockRelease(&p_service->lock, lock_state);

    p_ddr_buffer_info_t buf_to_send = p_service->p_active_asic_buffer;

    // Copy the diag header and asic header into ddr
    void* src = (void*)&buf_to_send->buffer.asic;
    void* dst = (void*)(buf_to_send->payload_management.base_addr);
    size_t size = sizeof(buf_to_send->buffer.asic);
    memcpy(dst, src, size);

    // Get a new buffer
    etr_get_new_asic_buffer(p_service);

    // Notify the ETDCC
    etr_notify_etdcc(buf_to_send);
}

//
// Helper function: Handles copying a buffer from the core to the ASIC buffer
//
static void etr_handle_copy_buffer_request(etr_service_context_t* p_service, etr_service_request_t* p_request)
{

    /**
     * When handling a copy buffer request we need to do a few things:
     *
     * 1. Check if the buffer will fit in the current asic buffer
     * 2. If it fits, copy the buffer into the asic buffer and notify the owner
     * 3. If it doesn't fit, complete the current asic buffer and start a new one, and requeue the request
     *
     */

    bool requeue = true;

    uint64_t size_left = p_service->p_active_asic_buffer->buffer.asic.asic_header.BufferSize -
                         p_service->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes;

    if (size_left >= p_request->request.copy_buffer.buffer_header.UsedBytes)
    {
        requeue = false;

        // Copy the buffer into the asic buffer
        void* src = (void*)p_request->request.copy_buffer.buffer_addr;
        void* dst = (void*)(p_service->p_active_asic_buffer->payload_management.base_addr) +
                    p_service->p_active_asic_buffer->payload_management.size_bytes;
        size_t size = p_request->request.copy_buffer.buffer_header.UsedBytes;
        memcpy(dst, src, size);

        // Update the asic buffer header and payload management
        p_service->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes += size;
        p_service->p_active_asic_buffer->payload_management.size_bytes += size;

        // Notify the owner of the core buffer that is has been copied
        etr_notify_etc(p_request);
    }
    else
    {
        // We don't have enough space in the current asic buffer, complete it.
        etr_complete_asic_buffer(p_service);
    }

    if (requeue)
    {
        UINT status = tx_queue_front_send(&p_service->request_queue.queue, p_request, TX_WAIT_FOREVER);
        ETR_CHECK_TX_STATUS(status);
    }
    else
    {
        // Return the request to the pool
        UINT status = tx_block_release(p_request);
        ETR_CHECK_TX_STATUS(status);
    }
}

//
// Helper function: Handles a host read request
//
static void etr_handle_host_read_request(etr_service_context_t* p_service, etr_service_request_t* p_request)
{

    /**
     * When handling a host read request we need to do a few things:
     *
     * 1. Find which buffer was read by the host
     * 2. Check if the buffer is still pending
     * 3. If it is, set it's state to free now that we heard back from the host
     * 4. If it isn't, it's been re-used at some point, so we don't need to do anything
     */

    bool buffer_found = false;

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_service->lock);
    for (uint32_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        if (p_service->ddr_buffers[i].payload_management.base_addr ==
            p_request->request.host_read.payload_management.base_addr)
        {
            buffer_found = true;
            if (p_service->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING)
            {
                p_service->ddr_buffers[i].state = ETR_DDR_BUFFER_STATE_FREE;
            }
            else
            {
                p_service->health_stats.delayed_host_reads++;
            }
            break;
        }
    }
    FpFwLockRelease(&p_service->lock, lock_state);

    if (!buffer_found)
    {
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG, 0, 0, 0, 0);
    }
}

static void etr_worker_thread_func(ULONG thread_input)
{
    etr_service_context_t* p_service = (etr_service_context_t*)thread_input;

    // Infinite loop that will decode and recycle buffers marked as completed
    while (true)
    {
        etr_service_request_t* p_request;
        (void)tx_queue_receive(&p_service->request_queue.queue, &p_request, TX_WAIT_FOREVER);
        etr_process_request(p_service, p_request);
    }
}

/**
 * Private Header Functions
 */

void etr_process_request(etr_service_context_t* p_service, etr_service_request_t* p_request)
{
    switch (p_request->type)
    {
    case ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER:
        etr_handle_copy_buffer_request(p_service, p_request);
        break;
    case ETR_SERVICE_REQUEST_TYPE_HOST_READ:
        etr_handle_host_read_request(p_service, p_request);
        break;
    default:
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG, 0, 0, 0, 0);
        break;
    }
}

void etr_icc_handle_etc()
{
    // @TODO: Implement once ICC to MCP/SCP/SDM/CDED is available (with loopback to MCP)
    //        https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1743379
    __asm__("nop");
}

void etr_icc_handle_etdcc()
{
    // @TODO: Implement once ICC to MCP (and die-to-die MCP) is available (with loopback to MCP)
    //        https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1925374
    __asm__("nop");
}

void etr_initialize_ddr_buffers(etr_service_context_t* p_service, const etr_service_config_t* p_config)
{

    // Validate the base addresses are set
    FPFW_RUNTIME_ASSERT(p_config->asic_ddr_config.base_addr != 0);
    FPFW_RUNTIME_ASSERT(p_config->hsp_ddr_config.base_addr != 0);

    // Validate we can fit at least one buffer of each
    FPFW_RUNTIME_ASSERT(p_config->asic_ddr_config.size_bytes <= IB_TELEMETRY_DDR_DIE_TRACE_ASIC_SIZE &&
                        p_config->asic_ddr_config.size_bytes >= ASIC_BUFFER_PAYLOAD_SIZE);
    FPFW_RUNTIME_ASSERT(p_config->hsp_ddr_config.size_bytes <= IB_TELEMETRY_DDR_DIE_TRACE_HSP_SIZE &&
                        p_config->hsp_ddr_config.size_bytes >= HSP_BUFFER_PAYLOAD_SIZE);

    uint64_t asic_count = p_config->asic_ddr_config.size_bytes / ASIC_BUFFER_PAYLOAD_SIZE;
    uint64_t hsp_count = p_config->hsp_ddr_config.size_bytes / HSP_BUFFER_PAYLOAD_SIZE;

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_service->lock);
    // populate the buffers array with the pointers to the pool memory
    for (uint32_t i = 0; i < asic_count; i++)
    {

        // Update the buffer context
        p_service->ddr_buffers[i] = (ddr_buffer_info_t){
            .state = ETR_DDR_BUFFER_STATE_FREE,
            .type = DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_DEVICE,
            .payload_management =
                {
                    .base_addr = p_config->asic_ddr_config.base_addr + (i * ASIC_BUFFER_PAYLOAD_SIZE),
                    .size_bytes = sizeof(asic_buffer_info_t),
                },
            .buffer.asic =
                {
                    .diag_header =
                        {
                            .encoder_version = TRACE_ENCODER_VERSION_DEVICE_PAYLOAD,
                            .strategy_id = DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_DEVICE,
                        },
                    .asic_header =
                        {
                            .SocId = p_config->soc_info.soc_id,
                            .FwVersion = {0},
                            .BufferSize = ASIC_BUFFER_PAYLOAD_SIZE - sizeof(diag_decoder_payload_header_t),
                            .UsedBytes = sizeof(FPFW_ET_ASIC_BUFFER_HEADER),
                        },
                },
        };
    }

    p_service->p_active_asic_buffer = &p_service->ddr_buffers[0];

    for (uint32_t i = 0; i < hsp_count; i++)
    {
        uint32_t buffer_index = i + ASIC_BUFFER_DDR_CAPACITY_MAX;

        // Update the buffer context
        p_service->ddr_buffers[buffer_index] = (ddr_buffer_info_t){
            .type = DIAGNOSTIC_DECODER_STRATEGY_ID_HSP_TRACE,
            .state = ETR_DDR_BUFFER_STATE_FREE,
            .payload_management =
                {
                    .base_addr = p_config->hsp_ddr_config.base_addr + (i * HSP_BUFFER_PAYLOAD_SIZE),
                    .size_bytes = sizeof(diag_decoder_payload_header_t),
                },
            .buffer.hsp =
                {
                    .diag_header =
                        {
                            .encoder_version = HSP_ENCODER_VERSION,
                            .strategy_id = DIAGNOSTIC_DECODER_STRATEGY_ID_HSP_TRACE,
                        },
                },
        };
    }
    FpFwLockRelease(&p_service->lock, lock_state);
}

void etr_initialize_worker_thread(etr_service_context_t* p_service, const etr_service_config_t* p_config)
{
    // Initialize the worker thread and it's dependencies
    UINT status = tx_block_pool_create(&p_service->request_queue.pool,
                                       ETR_WORK_POOL_NAME,
                                       sizeof(p_service->request_queue.pool_memory[0]),
                                       p_service->request_queue.pool_memory,
                                       sizeof(p_service->request_queue.pool_memory));
    ETR_CHECK_TX_STATUS(status);

    status = tx_queue_create(&p_service->request_queue.queue,
                             ETR_WORK_QUEUE_NAME,
                             sizeof(p_service->request_queue.queue_memory[0]) / sizeof(uint32_t),
                             p_service->request_queue.queue_memory,
                             sizeof(p_service->request_queue.queue_memory));
    ETR_CHECK_TX_STATUS(status);

    status = tx_thread_create(&p_service->worker_thread,
                              ETR_WORKER_THREAD_NAME,
                              etr_worker_thread_func,
                              (ULONG)p_service,
                              p_config->thread_config.p_stack,
                              p_config->thread_config.stack_size,
                              p_config->thread_config.priority,
                              p_config->thread_config.priority,
                              p_config->thread_config.time_slice_option,
                              TX_AUTO_START);
    ETR_CHECK_TX_STATUS(status);
}
