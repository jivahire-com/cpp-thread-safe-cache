//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay.c
 *  This modules initializes and configures event trace relay for a die.
 */

/**
 * Important Note: The ETR service accesses DDR memory that is accessible across the entire MSCP firmware.
 * Normally, such access would require a Spinlock before access. However, the segment used by
 * Event Trace Telemetry is dedicated and not used by other services based on static allocation.
 *
 * Hence, to simplify the design, no spinlock is used for DDR access for non-HSP telemetry, since these
 * buffers are only used by the ETR service on MCP. Concurrent access is only necessary if there is
 * re-entrancy within the service itself, which is not the case. The service operates on a queue mechanism,
 * ensuring that only one request is processed at a time.
 *
 * In DDR allocation, it is necessary to ensure that there is no overlapping memory pools between the ETR
 * service and other services. Otherwise, we can expect data corruption.
 *
 * In case there are other services that require access to the same DDR memory regions, it is the responsibility
 * of those services to implement their own synchronization mechanisms to prevent data corruption.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "etr_init_config_i.h"
#include "event_trace_relay_i.h"

#include <ErrorHandler.h>
#include <IFpFwEventTracingStatus.h>
#include <atu_init.h>
#include <bug_check.h>
#include <data_collection_protocol.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <mts_platform_specialization.h>
#include <sdm_ext_cfg_regs.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define EVT_TELEMETRY_GET_DDR_OFFSET(die_id, mscp_atu_mapped_addr) \
    ((die_id * IB_TELEMETRY_DDR_PER_DIE_SIZE) + (mscp_atu_mapped_addr - MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR))

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
    BUG_ASSERT(p_config->asic_ddr_config.base_addr != 0);
    BUG_ASSERT(p_config->hsp_ddr_config.base_addr != 0);

    /* Validate we can fit at least one buffer of each */
    BUG_ASSERT_PARAM(p_config->asic_ddr_config.size_bytes <= IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE &&
                         p_config->asic_ddr_config.size_bytes >= ASIC_BUFFER_PAYLOAD_SIZE,
                     p_config->asic_ddr_config.size_bytes,
                     0);
    BUG_ASSERT_PARAM(p_config->hsp_ddr_config.size_bytes <= IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE &&
                         p_config->hsp_ddr_config.size_bytes >= HSP_BUFFER_PAYLOAD_SIZE,
                     p_config->hsp_ddr_config.size_bytes,
                     0);

    uint64_t asic_count = p_config->asic_ddr_config.size_bytes / ASIC_BUFFER_PAYLOAD_SIZE;
    uint64_t hsp_count = p_config->hsp_ddr_config.size_bytes / HSP_BUFFER_PAYLOAD_SIZE;

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
}

/**
 * @brief Notify MCP Die 0 that the ASIC buffer is complete.
 *
 * @param p_buffer -> Pointer to the buffer that is complete
 * @param dest_core -> The core to notify (MCP or AP)
 * @return None
 */
static void etr_notify_primary_mcp_asic_buffer_complete(ddr_buffer_info_t* p_buffer)
{
    /* Create a TRP message to notify the MCP that the buffer is ready to be read */
    FPFW_DBGPRINT_VERBOSE("[ETR] Notifying MCP on Die 0 for ETR Processing Complete");

    uint32_t crc = fpfw_crc32(0, (void*)(p_buffer->payload_management.base_addr), p_buffer->payload_management.size_bytes);

    trp_msg_t send_msg = {
        .hdr =
            {
                .src_node = {.core_id = CPU_MCP, .die_id = mts_get_this_die_id()},
                .dest_node = {.core_id = CPU_MCP, .die_id = DIE_0},
                .trp_msg_status = TRP_STATUS_SUCCESS,
                .broadcast_type = TRP_BROADCAST_NONE,
                .mts_client_id = MTS_CLIENT_ID_EVENT_TRACE,
                .trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION,
                .payload_size = sizeof(trp_msg_read_pkg_rsp_t),
            },
        .payload =
            {
                .package_notification = {.source_die_id = mts_get_this_die_id(),
                                         .source_core_id = mts_get_this_core_id(),
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
    p_context->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    p_ddr_buffer_info_t buf_to_send = p_context->p_active_asic_buffer;

    /* Copy the diag header and asic header into ddr */
    void* src = (void*)&buf_to_send->buffer.asic;
    void* dst = (void*)(buf_to_send->payload_management.base_addr);
    size_t size = sizeof(buf_to_send->buffer.asic);
    memcpy(dst, src, size);

    /* Get a new buffer */
    etr_get_new_asic_buffer(p_context);

    /*
     * If Die 0, notify host that the buffer is ready to be read via DCP
     * If Die 1, then notify Die 0 MCP that the buffer is ready to be read via TRP
     */
    if (mts_get_this_die_id() == DIE_0)
    {
        mts_client_send_dcp_notification(MTS_CLIENT_ID_EVENT_TRACE, DCP_NOTIFICATION_TYPE_DATA_AVAILABLE);
    }
    else
    {
        etr_notify_primary_mcp_asic_buffer_complete(buf_to_send);
    }
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
 * @brief Handle a host read request from the host.
 *
 * @param p_context -> Pointer to the ETR service context.
 * @param p_request -> Pointer to the host read service request
 * @return None
 */
static void etr_handle_host_read_request(etr_service_context_t* p_context, etr_service_request_t* p_request)
{
    FPFW_DBGPRINT_INFO("[ETR] Handling Host Read Request");

    /* Set the status to success by default */
    p_request->p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;

    bool free_buffer_found = false;
    p_ddr_buffer_info_t p_pending_asic_buffer = NULL;

    /* Walk the buffer array to find the first pending buffer */
    for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
    {
        if (p_context->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING &&
            p_context->ddr_buffers[i].type == DIAG_PAYLOAD_PARSER_TRACE_DEVICE)
        {
            free_buffer_found = true;
            p_pending_asic_buffer = &p_context->ddr_buffers[i];
            break;
        }
    }

    if (!free_buffer_found)
    {
        FPFW_DBGPRINT_ERROR("[ETR] No pending ASIC buffers found for read request");
        p_request->p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_BUSY;
        return;
    }

    p_request->p_trp_msg->payload.dcp_msg.payload.read_data.physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
    p_request->p_trp_msg->payload.dcp_msg.payload.read_data.physical_buffer_size = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE;
    p_request->p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_addr_offset =
        EVT_TELEMETRY_GET_DDR_OFFSET(mts_get_this_die_id(), p_pending_asic_buffer->payload_management.base_addr);
    p_request->p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_size =
        p_pending_asic_buffer->payload_management.size_bytes + sizeof(asic_buffer_info_t);

    p_request->p_trp_msg->payload.dcp_msg.payload.read_data.crc =
        fpfw_crc32(0,
                   (void*)(p_pending_asic_buffer->payload_management.base_addr),
                   p_pending_asic_buffer->payload_management.size_bytes);
}

/**
 * @brief Processes a read complete response from MCP Die 0 or from the host
 *
 * @param p_context -> Pointer to the ETR service context
 * @param p_request -> Pointer to the host read service request
 * @return None
 */
static void etr_handle_read_complete_response(etr_service_context_t* p_context, etr_service_request_t* p_request, uint8_t source)
{
    bool buffer_found = false;
    uint64_t rd_addr = 0;

    /* By default, set the status to success */
    /* Decode the read address for the specific source */
    if (source == CPU_AP)
    {
        p_request->p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        rd_addr = (MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR +
                   p_request->p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset);
    }
    else
    {
        p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
        rd_addr = p_request->p_trp_msg->payload.read_package_complete.phy_addr_offset;
    }

    /* Find which buffer was read by the host */
    for (uint32_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        if (p_context->ddr_buffers[i].payload_management.base_addr == rd_addr)
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

    if (!buffer_found)
    {
        if (source == CPU_AP)
        {
            p_request->p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_NONE;
        }
        else
        {
            p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_RD_DATA_NONE;
        }
        FPFW_DBGPRINT_ERROR("[ETR] No pending ASIC buffers found for read complete request");
    }
}

/**
 * @brief Handles DCP messages for the ETR service.
 *
 * @param p_context Pointer to the ETR service context - Contains the ETR service configuration and state
 * @param p_request Pointer to the ETR service request - Contains DCP message information
 * @return None
 */
static void etr_handle_dcp_msg(p_etr_service_context_t p_context, p_etr_service_request_t p_request)
{
    // The ETR telemetry service runs on all MCPs. The architecture is such that the Host only interacts
    // with Die 0 MCP. DCP transactions are only handled by the primary MCP instance.
    bool primary_instance = mts_is_primary_instance();
    uint16_t response_dcp_payload_size = 0;

    p_trp_msg_t p_trp_msg = p_request->p_trp_msg;

    // reply back to the host if primary instance, otherwise let forwarded messages drop
    if (!primary_instance)
    {
        return;
    }

    switch (p_trp_msg->payload.dcp_msg.hdr.msg_id)
    {
    case DCP_MSG_ID_GET_CAPABILITIES:
        p_dcp_msg_get_caps_t get_caps = &p_trp_msg->payload.dcp_msg.payload.get_caps;
        get_caps->caps.as_uint32 = 0;
        get_caps->caps.DCP_MSG_ID_GET_CAPABILITIES = 1;
        get_caps->caps.DCP_MSG_ID_GET_STATE = 1;
        get_caps->caps.DCP_MSG_ID_READ_DATA = 1;
        get_caps->caps.DCP_MSG_ID_READ_DATA_COMPLETE = 1;

        response_dcp_payload_size = sizeof(dcp_msg_get_caps_t);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_GET_STATE:
        /* The ET telemetry client is always running */
        p_trp_msg->payload.dcp_msg.payload.get_state.state = DCP_CLIENT_STATE_RUNNING;

        response_dcp_payload_size = sizeof(dcp_msg_get_client_state_t);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_READ_DATA:
        response_dcp_payload_size = sizeof(dcp_msg_read_data_t);
        etr_handle_host_read_request(p_context, p_request);
        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:
        etr_handle_read_complete_response(p_context, p_request, CPU_AP);
        break;

    default:
        FPFW_DBGPRINT_ERROR("[ETR] Unexpected DCP message ID: %d", p_trp_msg->payload.dcp_msg.hdr.msg_id);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_E_UNSUPPORTED_MSG;
        break;
    };

    if (p_trp_msg->payload.dcp_msg.hdr.msg_status < 0)
    {
        p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
        p_trp_msg->payload.dcp_msg.hdr.payload_size = 0; // no payload for errors and not forwarding
    }
    else
    {
        p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
    }

    // if incoming message was validated and required to be forwarded, the forward message above needs to
    // retain the original payload size.
    p_trp_msg->payload.dcp_msg.hdr.payload_size = response_dcp_payload_size;
    p_trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + p_trp_msg->payload.dcp_msg.hdr.payload_size;

    mts_client_send_trp_response(p_trp_msg);
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
    UINT status = tx_thread_create(&p_context->worker_thread,
                                   ETR_WORKER_THREAD_NAME,
                                   etr_worker_thread_func,
                                   (ULONG)p_context,
                                   p_config->thread_config.p_stack,
                                   p_config->thread_config.stack_size,
                                   p_config->thread_config.priority,
                                   p_config->thread_config.priority,
                                   p_config->thread_config.time_slice_option,
                                   TX_AUTO_START);

    BUG_ASSERT(status == TX_SUCCESS);
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

    case CPU_SCP: /* For SCP, the addr_offset is a from scratch ram, which is mapped the same on both SCP and MCP. So use as is */
    case CPU_MCP: /* For MCP, the addr_offset is a local address, use as it is */
        break;

    default:
        FPFW_DBGPRINT_ERROR("[ETR] Invalid Core ID: %d", p_request->p_trp_msg->hdr.src_node.core_id);
        return FPFW_STATUS_FAIL;
    }

    /* Extract Buffer Address and Size from the Core Buffer Header */
    PFPFW_ET_CORE_BUFFER_HEADER p_etc_header = (PFPFW_ET_CORE_BUFFER_HEADER)p_request->buffer_addr;
    p_request->buffer_size_bytes = p_etc_header->BufferSize;

    /* Print the Event Trace Buffer Header metadata */
    FPFW_DBGPRINT_VERBOSE("[ETR] Event Trace Buffer Header Metadata:\n \
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
    BUG_ASSERT_PARAM(txStatus == TX_SUCCESS, txStatus, 0);
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

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_event_trace_relay_mts_client.mts_client.rx_pool);

    /* Create a queue for receiving Event Trace MTS client messages */
    tx_status = tx_queue_create(&s_event_trace_relay_mts_client.mts_client.rx_queue, // queue_ptr
                                ETR_WORK_QUEUE_NAME,                                 // name_ptr
                                sizeof(p_trp_msg_t) / sizeof(uint32_t),              // queue_message_size
                                g_etr_mts_client_queue_mem,                          // queue_start
                                sizeof(g_etr_mts_client_queue_mem));                 // queue_size

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_event_trace_relay_mts_client.mts_client.rx_queue);

    /* Create a flag group for synchronization between MTS and the ETR worker thread */
    tx_status = tx_event_flags_create(&s_etr_mts_flags, "ETR MTS flags");
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_etr_mts_flags);
    /* Register the MTS client */
    mts_client_register(MTS_CLIENT_ID_EVENT_TRACE, &s_event_trace_relay_mts_client.mts_client);
}

/*----------------------------- Global Functions ----------------------------*/
void etr_initialize(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{
    FPFW_DBGPRINT_INFO("[ETR] Initializing Event Trace Relay Service and the MTS Client");

    BUG_ASSERT(p_context != NULL);
    BUG_ASSERT(p_config != NULL);

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
    UINT queue_status = TX_SUCCESS;
    uint8_t this_die_id = mts_get_this_die_id();

    /* Infinite loop that will decode and recycle buffers marked as completed */
    while (true)
    {
        ULONG event_flags = 0;

        /* Wait for a new message to be available. This will block until a new message is available from MTS */
        UINT status = tx_event_flags_get(&s_etr_mts_flags, ETR_EVENT_FLAG_ANY_VALID, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);

        /* Loop until all messages are processed and queue is empty */
        /** Error Handling for tx_queue_receive
         * 1. TX_QUEUE_EMPTY - This is expected when there are no messages in the queue, so we will exit the loop gracefully, waiting for a flag again.
         * 2. TX_DELETED - Should not happen in this context as the queue is not handled by anyone else. This should be a fatal error since somehow the thread is deleted (which is unrecoverable)
         * 3. TX_WAIT_ABORTED, TX_QUEUE_ERROR, TX_PTR_ERROR, TX_WAIT_ERROR - This should not happen in this context as we are not passing the correct pointers/values.
         * This should be a fatal error since somehow the thread is deleted (which is unrecoverable)
         */
        do
        {
            queue_status =
                tx_queue_receive(&s_event_trace_relay_mts_client.mts_client.rx_queue, &etr_request.p_trp_msg, TX_NO_WAIT);

            // Assert if the queue status is not TX_SUCCESS or TX_QUEUE_EMPTY
            BUG_ASSERT(((queue_status == TX_QUEUE_EMPTY) || (queue_status == TX_SUCCESS)));

            if (queue_status == TX_QUEUE_EMPTY)
            {
                /* No more messages to process, exit the loop */
                break;
            }

            if (etr_request.p_trp_msg == NULL)
            {
                FPFW_DBGPRINT_ERROR("[ETR-MTS] Received NULL TRP message from MTS queue");
                break;
            }

            FPFW_DBGPRINT_INFO("[ETR-MTS] Processing new ET MTS msg with ID: %d, from core %d",
                               etr_request.p_trp_msg->hdr.trp_msg_id,
                               etr_request.p_trp_msg->hdr.src_node.core_id);

            switch (etr_request.p_trp_msg->hdr.trp_msg_id)
            {
            case TRP_MSG_ID_DCP_FORWARD:
                etr_handle_dcp_msg(p_context, &etr_request);
                break;
            case TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION:
            case TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE:
                /* Update p_trp_msg and respond with a TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE request */
                etr_request.p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;

                if (decode_validate_buffer_metadata(&etr_request) != FPFW_STATUS_SUCCESS)
                {
                    FPFW_DBGPRINT_ERROR("[ETR] Buffer Metadata Invalid");

                    /* Update the TRP message status to TRP_STATUS_E_PARAM and notify the ETC */
                    etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                    etr_notify_etc(&etr_request);
                    break;
                }

                /* Handle the copy buffer request */
                etr_handle_copy_buffer_request(p_context, &etr_request);
                break;

            /* Handle the case where the MCP Die 0 signals the MCP Die 1 that ASIC buffer read is complete. */
            case TRP_MSG_ID_READ_PACKAGE_COMPLETE:
                if (this_die_id == DIE_1)
                {
                    etr_handle_read_complete_response(p_context, &etr_request, CPU_MCP);
                }
                else
                {
                    FPFW_DBGPRINT_ERROR(
                        "[ETR-MTS] Received TRP_MSG_ID_READ_PACKAGE_COMPLETE on Die 0. Invalid.");

                    /* Update the TRP message status to TRP_STATUS_E_PARAM and notify the ETC */
                    etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                    etr_notify_etc(&etr_request);
                }
                break;

            case TRP_MSG_ID_PACKAGE_NOTIFICATION:
            case TRP_MSG_ID_READ_PACKAGE_RESPONSE:
                if (this_die_id == DIE_0)
                {
                    /* Update p_trp_msg and respond with a TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE request */
                    /* TODO ADO2686477: Add logic to copy the ASIC buffers from Die 1 to Die 0 */
                    etr_request.p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;
                }
                else
                {
                    FPFW_DBGPRINT_ERROR(
                        "[ETR-MTS] Received TRP_MSG_ID_PACKAGE_NOTIFICATION on Die 1. Invalid.");

                    /* Update the TRP message status to TRP_STATUS_E_PARAM*/
                    etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                }

                etr_notify_etc(&etr_request);
                break;

            case TRP_MSG_ID_READ_PACKAGE:
                if (this_die_id == DIE_1)
                {
                    etr_request.p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;
                    /* TODO ADO2686477: Add logic to reply with the ASIC buffer location from Die 1 to Die 0 */
                }
                else
                {
                    /* If we receive a TRP_MSG_ID_READ_PACKAGE on Die 0, we ignore it */
                    FPFW_DBGPRINT_ERROR("[ETR-MTS] Received TRP_MSG_ID_READ_PACKAGE on Die 0. Ignoring.");

                    /* Update the TRP message status to TRP_STATUS_E_PARAM */
                    etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                }
                etr_notify_etc(&etr_request);
                break;

            default:
                FPFW_DBGPRINT_ERROR("[ETR-MTS] Unsupported ET MTS ID: %d", etr_request.p_trp_msg->hdr.trp_msg_id);

                /* Update the TRP message status to TRP_STATUS_E_PARAM and notify the ETC */
                etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                etr_notify_etc(&etr_request);
                break;
            }

            /* Release the block back to the pool */
            status = tx_block_release(etr_request.p_trp_msg);
            BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);

#ifdef _WIN32
            /* For unit tests, break out of the loop after processing one message */
            break;
#endif
        } while (queue_status == TX_SUCCESS);

/* For unit tests - break out of the loop */
#ifdef _WIN32
        break;
#endif
    }
}
