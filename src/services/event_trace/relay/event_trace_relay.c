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
#include "event_trace_relay_quiesce_i.h"

#include <DbgPrint.h>
#include <ErrorHandler.h>
#include <IFpFwEventTracingStatus.h>
#include <atu_init.h>
#include <bug_check.h>
#include <cmsis_m7.h>
#include <data_collection_protocol.h>
#include <event_trace_collector.h>
#include <event_trace_relay_events.h>
#include <fpfw_cfg_mgr.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mts_platform_specialization.h>
#include <sdm_ext_cfg_regs.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/* Converts a MSCP ATU mapped address to a DDR offset from the telemetry base address */
#define EVT_TELEMETRY_GET_DDR_OFFSET(mscp_atu_mapped_addr) \
    (mscp_atu_mapped_addr - MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR)

/* Converts a Die 1 telemetry ATU offset to absolute offset from the telemetry base address */
#define EVT_TELEMETRY_TRANSLATE_DIE1_OFF_TO_ABS_OFF(offset_addr) \
    (offset_addr + (IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR - IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR))

/* Converts an absolute offset from telemetry base address to Die 1 telemetry ATU offset */
#define EVT_TELEMETRY_TRANSLATE_ABS_OFF_TO_DIE1_OFF(offset_addr) \
    (offset_addr - (IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR - IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR))

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static void etr_mts_rx_msg_handler(void);

/*------------------- Declarations (Statics and globals) --------------------*/

/* MTS Client Definition for Event Trace */
static mts_client_t s_etr_mts_client = {.notify_from_drv_frmwk = etr_mts_rx_msg_handler}; // Callback for receiving MTS messages

/* Message Queue Memory for the ET MTS Client */
static p_trp_msg_t g_etr_mts_client_queue_mem[ETR_MTS_CLIENT_MAX_MESSAGES];

/* Memory for the ET MTS Client Block Pool (used by the thread) */
static uint8_t g_etr_mts_client_pool_mem[ETR_MTS_CLIENT_BLOCK_POOL_SIZE];

static bool primary_instance = false;
static uint8_t this_die = 0;
static mts_platform_core_id_t this_core = 0;

static dcp_client_state_t evt_dcp_clnt_state = DCP_CLIENT_STATE_STOPPED;

static etr_service_context_t* p_etr_service_context = NULL;

static uint32_t num_buffers_pending = 0;

TX_EVENT_FLAGS_GROUP s_etr_mts_flags; // Event flags for synchronization

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
                            .FwVersion = {0},
                            .BufferSize = ASIC_BUFFER_PAYLOAD_SIZE - sizeof(diag_decoder_payload_header_t),
                            .UsedBytes = sizeof(FPFW_ET_ASIC_BUFFER_HEADER),
                        },
                },
        };

        memcpy(&p_context->ddr_buffers[i].buffer.asic.asic_header.SocId,
               p_config->soc_info.soc_id,
               sizeof(p_config->soc_info.soc_id));
    }

    p_context->p_active_asic_buffer = &p_context->ddr_buffers[0];

    for (uint32_t i = 0; i < hsp_count; i++)
    {
        /* Update the buffer context */
        p_context->ddr_buffers[i + ASIC_BUFFER_DDR_CAPACITY_MAX] = (ddr_buffer_info_t){
            .type = DIAG_PAYLOAD_PARSER_HSP_TRACE,
            .state = ETR_DDR_BUFFER_STATE_FREE,
            .payload_management =
                {
                    .base_addr = p_config->hsp_ddr_config.base_addr + (i * HSP_BUFFER_PAYLOAD_SIZE),
                    .size_bytes = sizeof(diag_decoder_payload_header_t),
                },
            .buffer.hsp = {.diag_header =
                               {
                                   .payload_parser_version = DIAG_HSP_PAYLOAD_PARSER_VERSION,
                                   .payload_parser_type = DIAG_PAYLOAD_PARSER_HSP_TRACE,
                               }},
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
                if ((p_context->health_stats.asic_buffers_reused++ % config_get_asic_buffers_reused_rpt_thresh()) == 0)
                {
                    FPFW_ET_LOG(AsicBufReused, p_context->health_stats.asic_buffers_reused)
                }
                break;
            }
        }
    }

    p_context->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;
    p_context->p_active_asic_buffer->payload_management.size_bytes = sizeof(asic_buffer_info_t);
    p_context->p_active_asic_buffer->buffer.asic.asic_header.UsedBytes = sizeof(FPFW_ET_ASIC_BUFFER_HEADER);
}

/**
 * @brief Notify Primary ETR that an ASIC buffer is complete.
 *
 * @param p_buffer -> Pointer to the buffer that is complete
 * @return None
 */
static void etr_notify_primary_etr_asic_buffer_complete()
{
    /* Create a TRP message to notify the Primary ETR that the buffer is ready to be read */
    trp_msg_t send_msg = {
        /* We only fill the header since the payload is not used on the primary ETR*/
        .hdr =
            {
                .src_node = {.core_id = CPU_MCP, .die_id = this_die},
                .dest_node = {.core_id = CPU_MCP, .die_id = DIE_0},
                .trp_msg_status = TRP_STATUS_SUCCESS,
                .broadcast_type = TRP_BROADCAST_NONE,
                .mts_client_id = MTS_CLIENT_ID_EVENT_TRACE,
                .trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION,
                .payload_size = sizeof(trp_msg_read_pkg_rsp_t),
            },
    };

    mts_client_send_new_trp_msg(&send_msg);
}

/**
 * @brief Complete the current ASIC buffer and prepare the next one.
 *
 * @param p_context -> Pointer to the ETR service context
 * @return None
 */
static void etr_complete_asic_buffer(etr_service_context_t* p_context)
{
    /* Update the current buffers state */
    p_context->p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    p_ddr_buffer_info_t p_buf_to_send = p_context->p_active_asic_buffer;

    /* Copy the diag header and asic header into ddr */
    void* src = (void*)&p_buf_to_send->buffer.asic;
    void* dst = (void*)(p_buf_to_send->payload_management.base_addr);
    size_t size = sizeof(p_buf_to_send->buffer.asic);
    memcpy(dst, src, size);

    /* Flush Cache so that the data is written to DDR - Address and size aligned to 32 Byte boundaries */
    SCB_CleanDCache_by_Addr((uint32_t*)FPFW_ALIGN_BY(32, (uint32_t)dst), (int32_t)FPFW_ALIGN_BY(32, size));

    /* Get a new buffer */
    etr_get_new_asic_buffer(p_context);

    /* Notify the host that the ASIC buffer is ready to be read */
    notify_ddr_buffer_available();
}

/**
 * @brief This function updates the DCP Message status based on the ASIC buffers remaining
 *
 * @param p_trp_msg pointer to the TRP Message packet to update
 */
static void update_dcp_message_status(p_trp_msg_t p_trp_msg)
{
    /* Update DCP Message status based on number of pending buffers */
    if (num_buffers_pending == 0)
    {
        p_trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_NONE;
    }
    else if (num_buffers_pending == 1)
    {
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_VALID_LAST;
    }
    else
    {
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_VALID_MORE;
    }
}

/************************************************************************
 * Static Helper Functions to manage MTS requests between the ETC and ETR services.
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

    // FPFW_DBGPRINT_INFO("ASIC Buff, Rem: %" PRIu64 " B. Req: %" PRIu32 " B\n", size_left, p_request->buffer_size_bytes);

    /* If it doesn't fit, complete the current asic buffer */
    if (size_left <= p_request->buffer_size_bytes)
    {
        etr_complete_asic_buffer(p_context);
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
}

/************************************************************************
 * Static Helper Functions for DCP <> TRP translation
 ************************************************************************/

static void translate_trp_response_to_dcp(p_trp_msg_t p_trp_msg, uint16_t translated_dcp_msg_id)
{
    uint64_t rd_data_addr_offset = 0;
    uint64_t rd_data_size = 0;
    uint64_t physical_buffer_size = 0;
    uint32_t block_crc = 0;

    /* Convert message to DCP */

    /* Update TRP Message Header */
    p_trp_msg->hdr.src_node.core_id = CPU_MCP;
    p_trp_msg->hdr.src_node.die_id = DIE_0;
    p_trp_msg->hdr.dest_node.core_id = CPU_AP;
    p_trp_msg->hdr.dest_node.die_id = DIE_0;
    p_trp_msg->hdr.broadcast_type = TRP_BROADCAST_NONE;
    p_trp_msg->hdr.mts_client_id = MTS_CLIENT_ID_EVENT_TRACE;
    p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    switch (translated_dcp_msg_id)
    {
    case DCP_MSG_ID_READ_DATA:

        /* Cache values from the TRP message */
        physical_buffer_size = p_trp_msg->payload.read_package_response.pkg_size;
        rd_data_addr_offset =
            EVT_TELEMETRY_TRANSLATE_DIE1_OFF_TO_ABS_OFF(p_trp_msg->payload.read_package_response.local_mmap_addr);
        rd_data_size = p_trp_msg->payload.read_package_response.pkg_size;
        block_crc = p_trp_msg->payload.read_package_response.crc;

        /* Update DCP Payload Size */
        p_trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_read_data_t);

        /* Update DCP Message Payload */
        p_trp_msg->payload.dcp_msg.payload.read_data.physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
        p_trp_msg->payload.dcp_msg.payload.read_data.physical_buffer_size = physical_buffer_size;
        p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_size = rd_data_size;
        p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_addr_offset = rd_data_addr_offset;
        p_trp_msg->payload.dcp_msg.payload.read_data.crc = block_crc;

        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:
        /* Cache values from the TRP message */
        rd_data_addr_offset = p_trp_msg->payload.read_package_complete.local_mmap_addr;
        rd_data_size = p_trp_msg->payload.read_package_complete.pkg_size;

        /* Update DCP Payload Size */
        p_trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_read_data_complete_t);

        /* Update DCP Message Payload */
        p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_size = rd_data_size;
        p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = rd_data_addr_offset;
        p_trp_msg->payload.dcp_msg.payload.read_data_complete.reserved1 = 0;
        p_trp_msg->payload.dcp_msg.payload.read_data_complete.reserved2 = 0;

        break;

    default:
        /* Invalid DCP message ID */
        return;
    }

    /* Update DCP Message Header */
    p_trp_msg->payload.dcp_msg.hdr.client_id = MTS_CLIENT_ID_EVENT_TRACE;
    p_trp_msg->payload.dcp_msg.hdr.msg_id = translated_dcp_msg_id;
    update_dcp_message_status(p_trp_msg);

    if (translated_dcp_msg_id == DCP_MSG_ID_READ_DATA_COMPLETE)
    {
        if (p_trp_msg->hdr.trp_msg_status == TRP_STATUS_SUCCESS)
        {
            num_buffers_pending--;
        }
    }

    /* Update TRP Message Status based on DCP message status*/
    if (p_trp_msg->payload.dcp_msg.hdr.msg_status < 0)
    {
        p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
        p_trp_msg->payload.dcp_msg.hdr.payload_size = 0; // no payload for errors and not forwarding
    }
    else
    {
        p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
    }

    /* Update TRP Payload Size */
    p_trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + p_trp_msg->payload.dcp_msg.hdr.payload_size;

    mts_client_forward_trp_msg(p_trp_msg, TRP_BROADCAST_NONE);
}

static void translate_dcp_msg_to_trp(p_trp_msg_t p_trp_msg, uint16_t translated_trp_msg_id)
{
    /* Update Source to MCP Die 0 */
    p_trp_msg->hdr.src_node.core_id = CPU_MCP;
    p_trp_msg->hdr.src_node.die_id = this_die;

    /* Update Destination to MCP Die 1 */
    p_trp_msg->hdr.dest_node.core_id = CPU_MCP;
    p_trp_msg->hdr.dest_node.die_id = DIE_1;

    /* Update trp_msg_id */
    p_trp_msg->hdr.trp_msg_id = translated_trp_msg_id;

    switch (translated_trp_msg_id)
    {
    case TRP_MSG_ID_READ_PACKAGE:
        p_trp_msg->hdr.payload_size = 0;
        break;

    case TRP_MSG_ID_READ_PACKAGE_COMPLETE:
        /* Cache TRP message response values */
        uint64_t rd_data_addr_offset = p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset;
        uint64_t rd_data_size = p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_size;

        /* Update values in the DCP Message */
        p_trp_msg->hdr.payload_size = sizeof(trp_msg_read_pkg_rsp_t);
        p_trp_msg->payload.read_package_complete.local_mmap_addr =
            EVT_TELEMETRY_TRANSLATE_ABS_OFF_TO_DIE1_OFF(rd_data_addr_offset);
        p_trp_msg->payload.read_package_complete.pkg_size = rd_data_size;
        break;

    default:
        /* Invalid TRP message ID */
        FPFW_ET_LOG(Error, this_die, this_core, ETR_ERR_INVALID_TRP_MSG, translated_trp_msg_id);
        return;
    }

    mts_client_forward_trp_msg(p_trp_msg, TRP_BROADCAST_NONE);
}

/************************************************************************
 * Static Helper Functions to handle Host Read Requests.
 ************************************************************************/

/**
 * @brief Handle a host read request from the host on the ETR.
 * On the Primary ETR, this is received via DCP and the response is a DCP message.
 * On a remote ETR, this is received via TRP and the response is a TRP message.
 *
 * @param p_context -> Pointer to the ETR service context.
 * @param p_request -> Pointer to the host read service request
 *
 * @return bool -> Result of the buffer search (found or not found)
 */
static bool etr_handle_host_read_request(etr_service_context_t* p_context, etr_service_request_t* p_request)
{
    uint64_t rd_data_size = 0;
    uintptr_t base_addr = 0;
    uint32_t block_crc = 0;
    uint64_t physical_buffer_size = 0;
    char* payload_type_str = "None";
    bool buffer_found = false;

    /* Walk the buffer array to find the first pending buffer */
    for (uint32_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        if (p_context->ddr_buffers[i].state == ETR_DDR_BUFFER_STATE_PENDING)
        {
            buffer_found = true;

            /* If a pending buffer is found, check if it is an ASIC buffer or HSP buffer */
            p_ddr_buffer_info_t p_pending_buffer = &p_context->ddr_buffers[i];

            base_addr = p_pending_buffer->payload_management.base_addr;
            if (p_context->ddr_buffers[i].type == DIAG_PAYLOAD_PARSER_HSP_TRACE)
            {
                payload_type_str = "Rd HSP";
                physical_buffer_size = IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE;
                rd_data_size = p_pending_buffer->payload_management.size_bytes;
            }
            else
            {
                payload_type_str = "Rd ASIC";
                physical_buffer_size = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE;
                rd_data_size = p_pending_buffer->payload_management.size_bytes + sizeof(asic_buffer_info_t);
            }

            block_crc = fpfw_crc32(0, (void*)base_addr, rd_data_size);

            break;
        }
    }

    FPFW_ET_LOG(HostCmd, EVT_TELEMETRY_GET_DDR_OFFSET(base_addr), physical_buffer_size, block_crc, payload_type_str);

    if (primary_instance)
    {
        if (buffer_found)
        {
            update_dcp_message_status(p_request->p_trp_msg);

            p_request->p_trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_read_data_t);
            p_request->p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_size = rd_data_size;
            p_request->p_trp_msg->payload.dcp_msg.payload.read_data.physical_buffer_size = physical_buffer_size;
            p_request->p_trp_msg->payload.dcp_msg.payload.read_data.physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
            p_request->p_trp_msg->payload.dcp_msg.payload.read_data.rd_data_addr_offset =
                EVT_TELEMETRY_GET_DDR_OFFSET(base_addr);
            p_request->p_trp_msg->payload.dcp_msg.payload.read_data.crc = block_crc;
            return true;
        }

        /* If no local pending buffer found, send a read_package message to the remote ETR via TRP */
        translate_dcp_msg_to_trp(p_request->p_trp_msg, TRP_MSG_ID_READ_PACKAGE);
    }
    else
    {
        p_request->p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;
        p_request->p_trp_msg->hdr.payload_size = sizeof(trp_msg_read_pkg_rsp_t);
        p_request->p_trp_msg->payload.read_package_response.crc = block_crc;
        p_request->p_trp_msg->payload.read_package_response.pkg_size = rd_data_size;
        p_request->p_trp_msg->payload.read_package_response.source_die_id = this_die;
        p_request->p_trp_msg->payload.read_package_response.source_core_id = this_core;
        p_request->p_trp_msg->payload.read_package_response.local_mmap_addr = EVT_TELEMETRY_GET_DDR_OFFSET(base_addr);
        if (buffer_found)
        {
            p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
        }
        else
        {
            p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_RD_DATA_NONE;
        }

        return true;
    }
    return false;
}

/**
 * @brief Processes a read complete response from Primary ETR or from the host.
 * On the Primary ETR, this is received via DCP.
 * On a remote ETR, this is received via TRP.
 *
 * @param p_context -> Pointer to the ETR service context
 * @param p_request -> Pointer to the host read service request
 * @return bool -> Result of the buffer search (found or not found)
 */
static bool etr_handle_read_complete_response(etr_service_context_t* p_context, etr_service_request_t* p_request)
{
    bool buffer_found = false;
    uint64_t rd_addr = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR;
    char* payload_status = "None";

    /* Decode Address */
    rd_addr += (primary_instance ? p_request->p_trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset
                                 : p_request->p_trp_msg->payload.read_package_complete.local_mmap_addr);

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
                payload_status = "Freed";
            }
            else
            {
                /* If it isn't, it's been re-used at some point, so we don't need to do anything */
                if ((p_context->health_stats.delayed_host_reads++ % config_get_delayed_host_reads_rpt_thresh()) == 0)
                {
                    FPFW_ET_LOG(DelHostReads, p_context->health_stats.delayed_host_reads);
                }
                payload_status = "Skipped";
            }
            num_buffers_pending--;

            break;
        }
    }

    FPFW_ET_LOG(HostCmd, rd_addr, 0, 0, payload_status);

    if (primary_instance)
    {
        if (!buffer_found)
        {
            /* Forward the read complete response to the remote ETR if primary instance */
            translate_dcp_msg_to_trp(p_request->p_trp_msg, TRP_MSG_ID_READ_PACKAGE_COMPLETE);
            return false;
        }

        /* If buffer found, update status based on number of buffers pending */
        update_dcp_message_status(p_request->p_trp_msg);
    }
    else
    {
        if (!buffer_found)
        {
            p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_RD_DATA_NONE;
        }
        else
        {
            p_request->p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
        }
    }

    return true;
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
    p_trp_msg_t p_trp_msg = p_request->p_trp_msg;
    bool send_trp_response = true;

    switch (p_trp_msg->payload.dcp_msg.hdr.msg_id)
    {
    case DCP_MSG_ID_GET_CAPABILITIES:
        p_dcp_msg_get_caps_t get_caps = &p_trp_msg->payload.dcp_msg.payload.get_caps;
        get_caps->caps.as_uint32 = 0;
        get_caps->caps.DCP_MSG_ID_GET_CAPABILITIES = 1;
        get_caps->caps.DCP_MSG_ID_GET_STATE = 1;
        get_caps->caps.DCP_MSG_ID_START_STOP = 1;
        get_caps->caps.DCP_MSG_ID_READ_DATA = 1;
        get_caps->caps.DCP_MSG_ID_READ_DATA_COMPLETE = 1;
        get_caps->caps.DCP_MSG_ID_RESET = 1;

        p_trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_get_caps_t);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_GET_STATE:
        p_trp_msg->payload.dcp_msg.payload.get_state.state = get_evt_dcp_client_state();

        p_trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_get_client_state_t);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_START_STOP:
        set_evt_dcp_client_state((dcp_client_state_t)p_trp_msg->payload.dcp_msg.payload.start_stop.state);
        break;

    case DCP_MSG_ID_READ_DATA:
        send_trp_response = etr_handle_host_read_request(p_context, p_request);
        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:
        send_trp_response = etr_handle_read_complete_response(p_context, p_request);
        break;

    case DCP_MSG_ID_RESET:
        /* Stop the Client */
        set_evt_dcp_client_state(DCP_CLIENT_STATE_STOPPED);

        /* Start the Client */
        set_evt_dcp_client_state(DCP_CLIENT_STATE_RUNNING);
        break;

    default:
        FPFW_ET_LOG(Error, this_die, this_core, ETR_ERR_INVALID_DCP_MSG, p_trp_msg->payload.dcp_msg.hdr.msg_id);
        p_trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_E_UNSUPPORTED_MSG;
        break;
    };

    if (send_trp_response)
    {
        if (p_trp_msg->payload.dcp_msg.hdr.msg_status < 0)
        {
            p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
            p_trp_msg->payload.dcp_msg.hdr.payload_size = 0; // no payload for errors and not forwarding
        }
        else
        {
            p_trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
        }
        p_trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + p_trp_msg->payload.dcp_msg.hdr.payload_size;

        mts_client_send_trp_response(p_trp_msg);
    }
}

/************************************************************************
 * Static Helper Functions for Event Trace relay and state management.
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

    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
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
fpfw_status_t decode_and_validate_buffer_metadata(etr_service_request_t* p_request)
{
    /* Read the MTS buffer address from the TRP message */
    p_request->buffer_addr = p_request->p_trp_msg->payload.intercore_block_notification.addr_offset;

    /* Core specific actions for the ETR */
    switch (p_request->p_trp_msg->hdr.src_node.core_id)
    {
    /* For accelerators, the addr_offset is relative to DTCM Base, so + Accel ATU Base Address + DTCM Offset */
    case CPU_SDM:
        p_request->buffer_addr += atu_svc_accel_atu_addr(ACCEL_ID_SDM) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
        break;

    case CPU_CDED_SDM:
        p_request->buffer_addr += atu_svc_accel_atu_addr(ACCEL_ID_CDED) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
        break;

    /* For SCP, the addr_offset is from EXP RAM, which is mapped the same on both SCP and MCP, use as is */
    /* On SCP, data is written on the other side of the (cached) EXP RAM, so invalidate cache for Trace Buffer Memory */
    /* Align address and size to 32B boundaries */
    case CPU_SCP:
        SCB_InvalidateDCache_by_Addr((uint32_t*)FPFW_ALIGN_BY(32, SCP_EXP_SCP_TRACE_BUFFER_BASE),
                                     FPFW_ALIGN_BY(32, SCP_EXP_SCP_TRACE_BUFFER_SIZE));
        break;

    /* For MCP, the addr_offset is local, use as it is */
    case CPU_MCP:
        break;

    default:
        return FPFW_STATUS_FAIL;
    }

    /* Extract Buffer Address and Size from the Core Buffer Header */
    PFPFW_ET_CORE_BUFFER_HEADER p_etc_header = (PFPFW_ET_CORE_BUFFER_HEADER)p_request->buffer_addr;
    p_request->buffer_size_bytes = p_etc_header->UsedBytes;

    /* Sanity Check the Buffer Header */
    if ((p_etc_header->CoreId != CPU_SCP && p_etc_header->CoreId != CPU_MCP &&
         p_etc_header->CoreId != CPU_SDM && p_etc_header->CoreId != CPU_CDED_SDM) ||
        (p_etc_header->BufferSize == 0))
    {
        return FPFW_STATUS_FAIL;
    }

    return FPFW_STATUS_SUCCESS;
}

/**
 * @brief Handle the completion of a read block complete operation.
 *
 * @param p_request Pointer to the service request containing the block information.
 * @return None
 */
static void etr_handle_read_block_complete(etr_service_request_t* p_request)
{
    /* Handle the case where the block ID is invalid, since this is not done by FPFwETControllerRecycleBuffer */
    uint32_t buffer_id = p_request->p_trp_msg->payload.read_intercore_block_complete.block_id;
    if (buffer_id >= ETC_SERVICE_CORE_BUFFER_COUNT)
    {
        /* Invalid Block ID, report error and drop the message */
        FPFW_ET_LOG_ETR_ASCII_ERROR("Fail: Invalid ET Buffer: %1d", buffer_id);
    }
    else
    {
        /* Recycle the buffer. Bug Assert on fail.
        Failure can cause Event Trace Telemetry to clog up, hindering performance till the next core reset */
        FPFW_ET_STATUS status = FPFwETControllerRecycleBuffer(FPFwETGetController(), buffer_id);
        BUG_ASSERT_PARAM(status == FPFW_ET_SUCCESS, status, 0);
    }
}

/**
 * @brief Initialize the MTS client for the Event Trace Relay.
 *
 * @param[in] None
 * @return None
 */
static void etr_mts_client_init(void)
{
    /* Create a block pool for the MTS client to use for receiving messages */
    UINT tx_status = tx_block_pool_create(&s_etr_mts_client.rx_pool,          // pool_ptr
                                          ETR_BLOCK_POOL_NAME,                // name_ptr
                                          MAX_TRP_MSG_BLOCK_SIZE,             // block_size
                                          g_etr_mts_client_pool_mem,          // pool_start
                                          sizeof(g_etr_mts_client_pool_mem)); // pool_size

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_etr_mts_client.rx_pool);

    /* Create a queue for receiving Event Trace MTS client messages */
    tx_status = tx_queue_create(&s_etr_mts_client.rx_queue,             // queue_ptr
                                ETR_WORK_QUEUE_NAME,                    // name_ptr
                                sizeof(p_trp_msg_t) / sizeof(uint32_t), // queue_message_size
                                g_etr_mts_client_queue_mem,             // queue_start
                                sizeof(g_etr_mts_client_queue_mem));    // queue_size

    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_etr_mts_client.rx_queue);

    /* Create a flag group for synchronization between MTS and the ETR worker thread */
    tx_status = tx_event_flags_create(&s_etr_mts_flags, ETR_FLAGS_NAME);
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_etr_mts_flags);

    /* Register the MTS client */
    mts_client_register(MTS_CLIENT_ID_EVENT_TRACE, &s_etr_mts_client);
}

/*----------------------------- Global Functions ----------------------------*/

void notify_ddr_buffer_available()
{
    /*
     * If Die 0, notify host that the buffer is ready to be read via DCP
     * If Die 1, then notify Primary ETR that the buffer is ready to be read via TRP. The Primary ETR will
     * then notify the host via DCP
     */
    if (primary_instance)
    {
        if ((num_buffers_pending == 0) && (evt_dcp_clnt_state == DCP_CLIENT_STATE_RUNNING))
        {
            mts_client_send_dcp_notification(MTS_CLIENT_ID_EVENT_TRACE, DCP_NOTIFICATION_TYPE_DATA_AVAILABLE);
        }
    }
    else
    {
        etr_notify_primary_etr_asic_buffer_complete();
    }

    num_buffers_pending++;
}

void etr_initialize(etr_service_context_t* p_context, const etr_service_config_t* p_config)
{
    BUG_ASSERT(p_context != NULL);
    BUG_ASSERT(p_config != NULL);

    p_etr_service_context = p_context;

    FPFW_ET_LOG_ETR_ASCII_INFO("Init ETR Svc");

    /* Initialize the ddr buffer management */
    etr_initialize_ddr_buffers(p_context, p_config);

    /* Initialize the hsp communication */
    etr_initialize_hsp_communication(p_context, p_config);

    /* Initialize the worker thread */
    etr_initialize_worker_thread(p_context, p_config);

    /* Initialize the MTS client for the Event Trace Relay */
    etr_mts_client_init();

    /* Register with the SOS service */
    event_trace_relay_dfwk_init();

    /* Cache MTS client information and core/die data for future use */
    primary_instance = mts_is_primary_instance();
    this_die = mts_get_this_die_id();
    this_core = mts_get_this_core_id();

    FPFW_ET_LOG_ETR_ASCII_INFO("ETR Svc Init Done");
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
    UINT tx_status = tx_event_flags_set(&s_etr_mts_flags, ETR_EVENT_FLAG_NEW_MTS_MSG, TX_OR);
    BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, 0);
}

void etr_worker_thread_func(ULONG thread_input)
{
    etr_service_context_t* p_context = (etr_service_context_t*)thread_input;
    etr_service_request_t etr_request = {0};
    mts_platform_core_id_t src_core_id;

    /* Infinite loop that will decode and recycle buffers marked as completed */
    while (true)
    {
        ULONG event_flags = 0;

        /* Wait for a new message to be available. This will block until a new message is available from MTS */
        UINT tx_status =
            tx_event_flags_get(&s_etr_mts_flags, ETR_EVENT_FLAG_ANY_VALID, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, 0);

        /* If new MTS message event flag is set */
        if ((event_flags & ETR_EVENT_FLAG_NEW_MTS_MSG) == ETR_EVENT_FLAG_NEW_MTS_MSG)
        {
            tx_status = tx_queue_receive(&s_etr_mts_client.rx_queue, &etr_request.p_trp_msg, TX_WAIT_FOREVER);
            // etr_handle_copy_buffer_request will modify the core_id hence caching it
            src_core_id = etr_request.p_trp_msg->hdr.src_node.core_id;

            // Assert if the queue status is not TX_SUCCESS or TX_QUEUE_EMPTY
            BUG_ASSERT_PARAM((tx_status == TX_SUCCESS), tx_status, 0);

            switch (etr_request.p_trp_msg->hdr.trp_msg_id)
            {
            /* Handle DCP Messages from the Host */
            /* The ETR telemetry service runs on all MCPs. The architecture is such that the Host only interacts
             * with Die 0 MCP (Primary ETR). DCP transactions from AP are only handled by the primary ETR instance. */
            case TRP_MSG_ID_DCP_FORWARD:
                etr_handle_dcp_msg(p_context, &etr_request);
                break;

            /* Handle intercore block complete the ETR. This is primarily an ETC functionality */
            case TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE:
                etr_handle_read_block_complete(&etr_request);
                break;

            /* Handle intercore block notifications from individual cores indicating ET buffers are available */
            case TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION:
                // Read trp_msg_status in trp_msg_hdr_t for local core notifications
                /* Update p_trp_msg and respond with a TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE request */
                etr_request.p_trp_msg->hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;

                if (decode_and_validate_buffer_metadata(&etr_request) != FPFW_STATUS_SUCCESS)
                {
                    FPFW_ET_LOG(Error, this_die, this_core, ETR_ERR_INVALID_ETC_BUFFER_METADATA, 0);

                    /* Update the TRP message status to TRP_STATUS_E_PARAM and notify the ETC */
                    etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                    etr_notify_etc(&etr_request);
                    break;
                }

                /* Handle the copy buffer request */
                etr_handle_copy_buffer_request(p_context, &etr_request);

                if (etr_request.p_trp_msg->hdr.trp_msg_status == TRP_STATUS_RD_DATA_NONE)
                {
                    // This is the indication that the message source core has quiesced
                    // Need to update the quiesce status here
                    event_trace_relay_external_core_quiesce_update(src_core_id);
                }
                break;

            case TRP_MSG_ID_PACKAGE_NOTIFICATION:
                notify_ddr_buffer_available();
                break;

            case TRP_MSG_ID_READ_PACKAGE:
                etr_handle_host_read_request(p_context, &etr_request);
                mts_client_send_trp_response(etr_request.p_trp_msg);
                break;

            case TRP_MSG_ID_READ_PACKAGE_COMPLETE:
                /* If primary die, then forward to host via DCP, if secondary, respond via TRP */
                if (primary_instance)
                {
                    translate_trp_response_to_dcp(etr_request.p_trp_msg, DCP_MSG_ID_READ_DATA_COMPLETE);
                }
                else
                {
                    etr_handle_read_complete_response(p_context, &etr_request);
                    mts_client_send_trp_response(etr_request.p_trp_msg);
                }
                break;

            case TRP_MSG_ID_READ_PACKAGE_RESPONSE:
                translate_trp_response_to_dcp(etr_request.p_trp_msg, DCP_MSG_ID_READ_DATA);
                break;

            default:
                FPFW_ET_LOG(Error, this_die, this_core, ETR_ERR_INVALID_TRP_MSG, etr_request.p_trp_msg->hdr.trp_msg_id);

                /* Update the TRP message status to TRP_STATUS_E_PARAM and notify the ETC */
                etr_request.p_trp_msg->hdr.trp_msg_status = TRP_STATUS_E_PARAM;
                etr_notify_etc(&etr_request);
                break;
            }

            /* Release the block back to the pool */
            tx_status = tx_block_release(etr_request.p_trp_msg);
            BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, 0);
        }

        /* If send DCP notification event flag is set */
        if ((event_flags & ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION) == ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION)
        {
            /* TODO: The Agent seems to need an additional delay in notification to catch it.
             * Leaving this printf to unblock telemetry Agent development. Will clean as part of ADO3314039
             */
            FPFW_DBGPRINT_INFO("Sending out DCP Notification from ETR Worker Thread\n");
            mts_client_send_dcp_notification(MTS_CLIENT_ID_EVENT_TRACE, DCP_NOTIFICATION_TYPE_DATA_AVAILABLE);
        }

/* For unit tests - break out of the loop */
#ifdef _WIN32
        break;
#endif
    }
}

void set_evt_dcp_client_state(dcp_client_state_t state)
{
    /* Update DCP Client State */
    evt_dcp_clnt_state = state;

    /* If the Client is running and there are pending buffers, send out a DCP notification */
    if ((num_buffers_pending > 0) && (evt_dcp_clnt_state == DCP_CLIENT_STATE_RUNNING))
    {
        UINT txStatus = tx_event_flags_set(&s_etr_mts_flags, ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION, TX_OR);
        BUG_ASSERT_PARAM(txStatus == TX_SUCCESS, txStatus, 0);
    }
}

dcp_client_state_t get_evt_dcp_client_state(void)
{
    return evt_dcp_clnt_state;
}

/************************************************************************
 * Functions to override internal variables for unit tests
 ************************************************************************/
#ifdef _WIN32
void set_num_buffers_pending(uint32_t num_buffers)
{
    num_buffers_pending = num_buffers;
}
#endif
