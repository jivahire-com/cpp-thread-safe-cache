//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay.c
 *  This modules initializes and configures event trace relay for a die.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "etr_init_config_i.h"
#include "event_trace_relay_i.h"

#include <ErrorHandler.h>
#include <FpFwAssert.h>
#include <FpFwLock.h>
#include <IFpFwEventTracingStatus.h>
#include <atu_init.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <sdm_ext_cfg_regs.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef struct
{
    p_etr_service_context_t p_etr_context; // Pointer to the ETR service context
    mts_client_t mts_client;               // MTS client for Event Trace Relay
} event_trace_relay_mts_client_t;

/*--------------------------- Function Prototypes ---------------------------*/
static void etr_mts_rx_msg_handler(void);

/*------------------- Declarations (Statics and globals) --------------------*/

TX_EVENT_FLAGS_GROUP s_etr_mts_flags; // Event flags for synchronization

/* MTS Client Definition for Event Trace */
static event_trace_relay_mts_client_t s_event_trace_relay_mts_client = {
    .mts_client =
        {
            .notify_from_drv_frmwk = etr_mts_rx_msg_handler, // Callback for receiving MTS messages
        },
};

/* Message Queue Memory for the ET MTS Client */
static p_trp_msg_t g_etr_mts_client_queue_mem[ETR_MAX_MTS_CLIENT_MESSAGES];

/* Memory for the ET MTS Client Block Pool (used by the thread) */
static uint8_t g_etr_mts_client_pool_mem[ET_MTS_CLIENT_BLOCK_POOL_SIZE];

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

// TODO (ADO2686478): Revisit which failures are hard failures and which are soft failures
// At the moment, all failures are treated as soft failures in the ETR MTS Processing Path

// TODO (ADO2686478): Evaluate if and where we need Spinlocks in accessing/managing buffers
// The current implementation retains the old for DDR buffer handling with whatever spinlocks were used before.

/************************************************************************
 * Static Helper Functions to manage DDR ASIC buffers for the ETR service.
 ************************************************************************/

/**
 * @brief Initialize the DDR buffers for the ETR service.
 *
 * @param p_context -> Pointer to the ETR service context
 * @param p_config -> Pointer to the ETR service configuration
 * @return None
 */
static void etr_initialize_ddr_buffers(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{

    /* Validate the base addresses are set */
    FPFW_RUNTIME_ASSERT(p_config->asic_ddr_config.base_addr != 0);
    FPFW_RUNTIME_ASSERT(p_config->hsp_ddr_config.base_addr != 0);

    /* Validate we can fit at least one buffer of each */
    FPFW_RUNTIME_ASSERT(p_config->asic_ddr_config.size_bytes <= IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE &&
                        p_config->asic_ddr_config.size_bytes >= ASIC_BUFFER_PAYLOAD_SIZE);
    FPFW_RUNTIME_ASSERT(p_config->hsp_ddr_config.size_bytes <= IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE &&
                        p_config->hsp_ddr_config.size_bytes >= HSP_BUFFER_PAYLOAD_SIZE);

    uint64_t asic_count = p_config->asic_ddr_config.size_bytes / ASIC_BUFFER_PAYLOAD_SIZE;
    uint64_t hsp_count = p_config->hsp_ddr_config.size_bytes / HSP_BUFFER_PAYLOAD_SIZE;

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_context->lock);

    /* Populate the buffers array with the pointers to the pool memory */
    for (uint32_t i = 0; i < asic_count; i++)
    {

        /* Update the buffer context */
        p_context->ddr_buffers[i] = (ddr_buffer_info_t){
            .state = ETR_DDR_BUFFER_STATE_FREE,
            .type = DIAG_PAYLOAD_PARSER_TRACE_DEVICE,
            .payload_management =
                {
                    .base_addr = p_config->asic_ddr_config.base_addr + (i * ASIC_BUFFER_PAYLOAD_SIZE),
                    .size_bytes = sizeof(asic_buffer_info_t),
                },
            .buffer.asic =
                {
                    .diag_header =
                        {
                            .payload_parser_version = DIAG_TRACE_PAYLOAD_PARSER_VERSION_DEVICE_PAYLOAD,
                            .payload_parser_type = DIAG_PAYLOAD_PARSER_TRACE_DEVICE,
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

    p_context->p_active_asic_buffer = &p_context->ddr_buffers[0];

    for (uint32_t i = 0; i < hsp_count; i++)
    {
        uint32_t buffer_index = i + ASIC_BUFFER_DDR_CAPACITY_MAX;

        /* Update the buffer context */
        p_context->ddr_buffers[buffer_index] = (ddr_buffer_info_t){
            .type = DIAG_PAYLOAD_PARSER_HSP_TRACE,
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
                            .payload_parser_version = DIAG_HSP_PAYLOAD_PARSER_VERSION,
                            .payload_parser_type = DIAG_PAYLOAD_PARSER_HSP_TRACE,
                        },
                },
        };
    }
    FpFwLockRelease(&p_context->lock, lock_state);
}

/**
 * @brief Get a new ASIC buffer from the DDR buffer pool.
 *
 * @param p_context -> Pointer to the ETR service context
 * @return None
 */
static void etr_get_new_asic_buffer(etr_service_context_t* p_context)
{
    bool free_buffer_found = false;

    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_context->lock);
    /* Walk the buffer array to find the next free buffer */
    for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
    {
        if (p_context->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_FREE &&
            p_context->ddr_buffers[i].type == DIAG_PAYLOAD_PARSER_TRACE_DEVICE)
        {
            free_buffer_found = true;
            p_context->p_active_asic_buffer = &p_context->ddr_buffers[i];
            break;
        }
    }

    /* If we didn't find a free buffer then we need to re-use one that is pending */
    if (!free_buffer_found)
    {
        for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
        {
            if (p_context->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING &&
                p_context->ddr_buffers[i].type == DIAG_PAYLOAD_PARSER_TRACE_DEVICE)
            {
                p_context->p_active_asic_buffer = &p_context->ddr_buffers[i];
                p_context->health_stats.asic_buffers_reused++;
                break;
            }
        }
    }

    p_context->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;
    p_context->p_active_asic_buffer->payload_management.size_bytes = sizeof(asic_buffer_info_t);
    p_context->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes = sizeof(FPFW_ET_ASIC_BUFFER_HEADER);
    FpFwLockRelease(&p_context->lock, lock_state);
}

/**
 * @brief Notify the specified core that the ASIC buffer is complete.
 *
 * @param p_buffer -> Pointer to the buffer that is complete
 * @param dest_core -> The core to notify (MCP or AP)
 * @return None
 */
static void etr_notify_asic_buffer_complete(ddr_buffer_info_t* p_buffer, KNG_CPU_TYPE dest_core)
{
    /* Create a TRP message to notify the MCP that the buffer is ready to be read */
    FPFW_DBGPRINT_VERBOSE("[ETR] Notifying %s on Die 0 for ETR Processing Complete", (dest_core == CPU_MCP) ? "MCP" : "AP");

    uint32_t crc = fpfw_crc32(0, (void*)(p_buffer->payload_management.base_addr), p_buffer->payload_management.size_bytes);

    trp_msg_t send_msg = {
        .hdr =
            {
                .src_node = {.core_id = CPU_MCP, .die_id = idsw_get_die_id()},
                .dest_node = {.core_id = dest_core, .die_id = DIE_0},
                .trp_msg_status = TRP_STATUS_SUCCESS,
                .broadcast_type = TRP_BROADCAST_NONE,
                .mts_client_id = MTS_CLIENT_ID_EVENT_TRACE,
                .trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION,
                .payload_size = sizeof(trp_msg_read_pkg_rsp_t),
            },
        .payload =
            {
                .package_notification = {.source_die_id = idsw_get_die_id(),
                                         .source_core_id = CPU_MCP,
                                         .local_mmap_addr = p_buffer->payload_management.base_addr,
                                         .pkg_size = p_buffer->payload_management.size_bytes,
                                         .crc = crc},
            },
    };

    mts_client_send_new_trp_msg(&send_msg);
}

/**
 * @brief Complete the current ASIC buffer and prepare for the next one.
 *
 * @param p_context -> Pointer to the ETR service context
 * @return None
 */
static void etr_complete_asic_buffer(etr_service_context_t* p_context)
{
    /* Update the current buffers state */
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_context->lock);
    p_context->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
    FpFwLockRelease(&p_context->lock, lock_state);

    p_ddr_buffer_info_t buf_to_send = p_context->p_active_asic_buffer;

    /* Copy the diag header and asic header into ddr */
    void* src = (void*)&buf_to_send->buffer.asic;
    void* dst = (void*)(buf_to_send->payload_management.base_addr);
    size_t size = sizeof(buf_to_send->buffer.asic);
    memcpy(dst, src, size);

    /* Get a new buffer */
    etr_get_new_asic_buffer(p_context);

    /*
     * If Die 0, notify host that the buffer is ready to be read
     * If Die 1, then notify Die 0 MCP that the buffer is ready to be read
     */
    KNG_CPU_TYPE dest_core = (idsw_get_die_id() == DIE_0) ? CPU_AP : CPU_MCP;
    etr_notify_asic_buffer_complete(buf_to_send, dest_core);
}

/************************************************************************
 * Static Helper Functions to manage MTS requests to the ETR service.
 ************************************************************************/
/**
 * @brief Notify the Event Trace Collector that a buffer has been processed.
 *
 * @param p_request Pointer to the service request containing the buffer information.
 * @return None
 */
static void etr_notify_etc(etr_service_request_t* p_request)
{
    FPFW_DBGPRINT_VERBOSE("[ETR] Notifying Event Trace Collector for ETR Processing Complete");
    mts_client_send_trp_response(p_request->p_trp_msg);
}

/**
 * @brief Handle a copy buffer request from any core via MTS.
 *
 * @param p_context -> Pointer to the ETR service context
 * @param p_request -> Pointer to the copy buffer request
 * @return None
 */
static void etr_handle_copy_buffer_request(etr_service_context_t* p_context, etr_service_request_t* p_request)
{
    /* Check if the buffer will fit in the current asic buffer */
    uint64_t size_left = p_context->p_active_asic_buffer->buffer.asic.asic_header.BufferSize -
                         p_context->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes;

    /* If it doesn't fit, complete the current asic buffer */
    if (size_left <= p_request->buffer_size_bytes)
    {
        FPFW_DBGPRINT_VERBOSE(
            "[ETR] Not enough space in current ASIC buffer, completing it and using a new one");
        etr_complete_asic_buffer(p_context);
    }
    else
    {
        FPFW_DBGPRINT_VERBOSE("[ETR] Buffer fits in current ASIC buffer. Size left: %zu", size_left);
    }

    /* Copy the buffer into the asic buffer */
    void* src = (void*)p_request->buffer_addr;
    void* dst = (void*)(p_context->p_active_asic_buffer->payload_management.base_addr) +
                p_context->p_active_asic_buffer->payload_management.size_bytes;
    size_t size = p_request->buffer_size_bytes;
    memcpy(dst, src, size);

    /* Update the asic buffer header and payload management */
    p_context->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes += size;
    p_context->p_active_asic_buffer->payload_management.size_bytes += size;

    /* Notify the owner of the core buffer that is has been copied */
    etr_notify_etc(p_request);

    FPFW_DBGPRINT_VERBOSE("[ETR] Handle Copy Buffer Request Completed");
}

/**
 * @brief Processes a host read complete request from the host.
 *
 * @param p_context -> Pointer to the ETR service context
 * @param p_request -> Pointer to the host read request
 * @return None
 */
static void etr_handle_host_read_complete_request(etr_service_context_t* p_context, etr_service_request_t* p_request)
{
    bool buffer_found = false;

    /* Find which buffer was read by the host */
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&p_context->lock);
    for (uint32_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        if (p_context->ddr_buffers[i].payload_management.base_addr ==
            p_request->p_trp_msg->payload.read_package_complete.phy_addr_offset)
        {
            buffer_found = true;

            /* Check if the buffer is still pending */
            if (p_context->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING)
            {
                /* If it is, set it's state to free now that we heard back from the host */
                p_context->ddr_buffers[i].state = ETR_DDR_BUFFER_STATE_FREE;
            }
            else
            {
                /* If it isn't, it's been re-used at some point, so we don't need to do anything */
                p_context->health_stats.delayed_host_reads++;
            }
            break;
        }
    }
    FpFwLockRelease(&p_context->lock, lock_state);

    if (!buffer_found)
    {
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG, 0, 0, 0, 0);
    }
}

/************************************************************************
 * Static Helper Functions for Event Trace relay Initialization and state management.
 ************************************************************************/

/**
 * @brief Initialize the worker thread for the Event Trace relay service.
 *
 * @param p_context Pointer to the ETR service context
 * @param p_config Pointer to the ETR service configuration
 * @return None
 */
static void etr_initialize_worker_thread(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{
    ETR_CHECK_TX_STATUS(tx_thread_create(&p_context->worker_thread,
                                         ETR_WORKER_THREAD_NAME,
                                         etr_worker_thread_func,
                                         (ULONG)p_context,
                                         p_config->thread_config.p_stack,
                                         p_config->thread_config.stack_size,
                                         p_config->thread_config.priority,
                                         p_config->thread_config.priority,
                                         p_config->thread_config.time_slice_option,
                                         TX_AUTO_START));
}

/**
 * @brief Decode and validate the buffer metadata from the service request.
 *
 * This function reads the buffer address from the request, validates it, and extracts the buffer size.
 * It also checks the core ID and timestamps to ensure the buffer is valid.
 *
 * @param p_request Pointer to the service request
 * @return fpfw_status_t Returns FPFW_STATUS_SUCCESS if the buffer metadata is valid, otherwise returns FPFW_STATUS_FAIL.
 */
fpfw_status_t decode_validate_buffer_metadata(etr_service_request_t* p_request)
{
    /* Read the MTS buffer address from the TRP message */
    p_request->buffer_addr = p_request->p_trp_msg->payload.intercore_block_notification.addr_offset;

    switch (p_request->p_trp_msg->hdr.src_node.core_id)
    {
    /* For accelerators, the addr_offset is relative to DTCM Base.
    Hence add ATU Base Address for the accelerator as well as DTCM Offset from ATU Base Address */
    case CPU_SDM:
        p_request->buffer_addr += atu_svc_accel_atu_addr(ACCEL_ID_SDM) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
        break;

    case CPU_CDED_SDM:
        p_request->buffer_addr += atu_svc_accel_atu_addr(ACCEL_ID_CDED) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
        break;

    case CPU_SCP:
        // TODO (ADO2686478): For SCP address off set is relative to the MSCP Expansion RAM Base Address
        break;

    /* For MCP, the addr_offset is a local address, use as it is */
    case CPU_MCP:
        break;

    default:
        FPFW_DBGPRINT_ERROR("[ETR] Invalid Core ID: %d", p_request->p_trp_msg->hdr.src_node.core_id);
        return FPFW_STATUS_FAIL;
    }

    /* Extract Buffer Address and Size from the Core Buffer Header */
    PFPFW_ET_CORE_BUFFER_HEADER p_etc_header = (PFPFW_ET_CORE_BUFFER_HEADER)p_request->buffer_addr;
    p_request->buffer_size_bytes = p_etc_header->BufferSize;

    /* Print the Event Trace Buffer Header metadata */
    FPFW_DBGPRINT_WARNING("[ETR] Event Trace Buffer Header Metadata:\n \
        - Core ID: %d\n    \
        - Buffer ID: %d, \n    \
        - Buffer Size: %zu",
                          p_etc_header->CoreId,
                          p_etc_header->BufferId,
                          p_etc_header->BufferSize);

    /* Sanity Check the Buffer Header */
    if ((p_etc_header->CoreId != CPU_SCP && p_etc_header->CoreId != CPU_MCP &&
         p_etc_header->CoreId != CPU_SDM && p_etc_header->CoreId != CPU_CDED_SDM) ||
        (p_etc_header->BufferSize == 0))
    {
        FPFW_DBGPRINT_ERROR("[ETR] Invalid ET Buffer. Dropping request.");
        return FPFW_STATUS_FAIL;
    }

    return FPFW_STATUS_SUCCESS;
}

/**
 * @brief Driver Framework Callback to handle incoming MTS messages.
 *
 * @param None
 * @return None
 */
static void etr_mts_rx_msg_handler(void)
{
    /* Set the event flag to indicate a new MTS message is available */
    UINT txStatus = tx_event_flags_set(&s_etr_mts_flags, ETR_EVENT_FLAG_NEW_MTS_MSG, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

/**
 * @brief Initialize the MTS client for the Event Trace Relay.
 *
 * @param p_context Pointer to the ETR service context
 * @return None
 */
static void etr_mts_client_init(p_etr_service_context_t p_context, const etr_service_config_t* p_config)
{
    FPFW_UNUSED(p_config);
    s_event_trace_relay_mts_client.p_etr_context = p_context;

    /* Create a block pool for the MTS client to use for receiving messages */
    UINT tx_status = tx_block_pool_create(&s_event_trace_relay_mts_client.mts_client.rx_pool, // pool_ptr
                                          ETR_BLOCK_POOL_NAME,                                // name_ptr
                                          MAX_TRP_MSG_BLOCK_SIZE,                             // block_size
                                          g_etr_mts_client_pool_mem,                          // pool_start
                                          sizeof(g_etr_mts_client_pool_mem));                 // pool_size

    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS,
                            tx_status,
                            (uintptr_t)&s_event_trace_relay_mts_client.mts_client.rx_pool,
                            0,
                            0);

    /* Create a queue for receiving Event Trace MTS client messages */
    tx_status = tx_queue_create(&s_event_trace_relay_mts_client.mts_client.rx_queue, // queue_ptr
                                ETR_WORK_QUEUE_NAME,                                 // name_ptr
                                sizeof(p_trp_msg_t) / sizeof(uint32_t),              // queue_message_size
                                g_etr_mts_client_queue_mem,                          // queue_start
                                sizeof(g_etr_mts_client_queue_mem));                 // queue_size

    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS,
                            tx_status,
                            (uintptr_t)&s_event_trace_relay_mts_client.mts_client.rx_queue,
                            0,
                            0);

    /* Create a flag group for synchronization between MTS and the ETR worker thread */
    tx_status = tx_event_flags_create(&s_etr_mts_flags, "ETR MTS flags");
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_etr_mts_flags, 0, 0);

    /* Register the MTS client */
    mts_client_register(MTS_CLIENT_ID_EVENT_TRACE, &s_event_trace_relay_mts_client.mts_client);
}

/*----------------------------- Global Functions ----------------------------*/
void etr_initialize(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{
    FPFW_DBGPRINT_INFO("[ETR] Initializing Event Trace Relay Service and the MTS Client");

    if (NULL == p_context || NULL == p_config)
    {
        FPFwErrorRaise(FPFW_ET_E_INVALIDARG, 0, 0, 0, 0);
    }

    /* Initialize the queue lock */
    FpFwLockInitialize(&p_context->lock);

    /* Initialize the ddr buffer management */
    etr_initialize_ddr_buffers(p_context, p_config);

    /* Initialize the hsp communication */
    etr_initialize_hsp_communication(p_context, p_config);

    /* Initialize the worker thread */
    etr_initialize_worker_thread(p_context, p_config);

    /* Initialize the MTS client for the Event Trace Relay */
    etr_mts_client_init(p_context, p_config);

    FPFW_DBGPRINT_INFO("[ETR] Event Trace Relay Service and MTS Client Initialized");
}

/**
 * @brief Worker thread function for processing Event Trace requests.
 * This function is a public function for unit testing purposes.
 *
 * @param thread_input Pointer to the ETR service context.
 * @return None -> This function runs in an infinite loop, so should never return
 */
void etr_worker_thread_func(ULONG thread_input)
{
    etr_service_context_t* p_context = (etr_service_context_t*)thread_input;

    etr_service_request_t etr_request = {0};

    /* Infinite loop that will decode and recycle buffers marked as completed */
    while (true)
    {
        ULONG event_flags = 0;

        /* Wait for a new message to be available. This will block until a new message is available from MTS */
        UINT status = tx_event_flags_get(&s_etr_mts_flags, ETR_EVENT_FLAG_ANY_VALID, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT_EXT(status == TX_SUCCESS, status, 0, 0, 0);

        /* Loop until all messages are processed and queue is empty */
        /** Error Handling for tx_queue_receive
         * 1. TX_DELETED - Should not happen in this context as the queue is created and used by the worker thread. TODO (ADO2686478): Evaluate if we need to handle this failure
         * 2. TX_QUEUE_EMPTY - This is expected when there are no messages in the queue, so we will exit the loop gracefully.
         * 3. TX_WAIT_ABORTED - This should not happen in this context as we are not using any wait options that can be aborted.
         * 4. TX_QUEUE_ERROR - This shouldn't happen since we are passing a valid queue pointer. This is caught statically in the unit tests.etr_request
         * 5. TX_PTR_ERROR - This should not happen since we are passing a valid pointer to the request structure. This is caught statically in the unit tests.
         * 6. TX_WAIT_ERROR - This should not happen since we are not using any wait options that can fail. This is caught statically in the unit tests.
         */
        while (tx_queue_receive(&s_event_trace_relay_mts_client.mts_client.rx_queue, &etr_request.p_trp_msg, TX_NO_WAIT) == TX_SUCCESS)
        {
            FPFW_DBGPRINT_INFO("[ETR-MTS] Processing new ET MTS msg with ID: %d, from core %d",
                               etr_request.p_trp_msg->hdr.trp_msg_id,
                               etr_request.p_trp_msg->hdr.src_node.core_id);

            switch (etr_request.p_trp_msg->hdr.trp_msg_id)
            {
            case TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION:
            case TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE:
                /* Update p_trp_msg and respond with a TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE request */
                etr_request.p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;

                if (decode_validate_buffer_metadata(&etr_request) != FPFW_STATUS_SUCCESS)
                {
                    FPFW_DBGPRINT_ERROR("[ETR] Buffer Metadata Invalid. Dropping request.");
                    etr_notify_etc(&etr_request);
                    break;
                }

                /* Handle the copy buffer request */
                etr_handle_copy_buffer_request(p_context, &etr_request);
                break;

            /* Handle the case where the host signals the MCP that ASIC buffer read is complete */
            case TRP_MSG_ID_READ_PACKAGE_COMPLETE:
                // TODO (ADO2686478): This flow is not fully complete - will be done in a future PR
                etr_handle_host_read_complete_request(p_context, &etr_request);
                break;

            // TODO (ADO2686478): Handle these messages when implementing D2D and MCP0 to Host interactions
            case TRP_MSG_ID_PACKAGE_NOTIFICATION: // Used by MCP Die 1 to indicate to MSCP Die 0 that a new ET Package is ready for forwarding (Push)
            case TRP_MSG_ID_READ_PACKAGE_RESPONSE: // Used by MCP Die 1 to indicate that the package is ready for
                                                   // reading (Pull) Update the destination core to be AP Die 0 p_trp_msg->hdr.dest_node.core_id
                                                   // = CPU_AP; p_trp_msg->hdr.dest_node.die_id = DIE_0;

            case TRP_MSG_ID_READ_PACKAGE: // Used by MCP Die 0 to indicate to MCP Die 1 that the read package operation is complete. Also used by the host to indicate to MCP Die 0 that the package has been read completely

            default:
                /* Soft failure. Report and move on */
                FPFW_DBGPRINT_ERROR("[ETR-MTS] Unsupported ET MTS ID: %d", etr_request.p_trp_msg->hdr.trp_msg_id);
                break;
            }

            /* Release the block back to the pool */
            status = tx_block_release(etr_request.p_trp_msg);
            FPFW_RUNTIME_ASSERT_EXT(status == TX_SUCCESS, status, 0, 0, 0);

#ifdef _WIN32
            /* For unit tests, break out of the loop after processing one message */
            break;
#endif
        }

/* For unit tests - break out of the loop */
#ifdef _WIN32
        break;
#endif
    }
}
