//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file etr_hsp_manager.c
 *  This modules handles the HSP communication for the Event Trace Relay service.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "event_trace_relay_i.h"

#include <IFpFwEventTracingStatus.h>
#include <bug_check.h>
#include <event_trace_relay_events.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <in_band_telemetry_ddr.h>
#include <message_transfer_service.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
static fpfw_icc_base_recv_req_t hsp_recv_params;
static kng_hsp_mailbox_cmd_send_log_req hsp_recv_msg;

#ifdef _WIN32 // Unit Test Only
p_hsp_log_payload_header_t override_atu_test_addr = NULL;
#endif

/*----------------------------- Static Functions ----------------------------*/

static void etr_receive_hsp(etr_service_context_t* p_service_context)
{
    /* Setup the receive request. The send request is setup as needed before sending it to the hsp */
    memset(&hsp_recv_msg, 0, sizeof(hsp_recv_msg));
    memset(&hsp_recv_params, 0, sizeof(hsp_recv_params));
    hsp_recv_params.payload_buffer = &hsp_recv_msg;
    hsp_recv_params.buffer_size = sizeof(hsp_recv_msg);
    hsp_recv_params.recv_cmd_code = HSP_MAILBOX_CMD_SEND_LOG_REQ;
    hsp_recv_params.cb = etr_icc_handle_hsp;
    hsp_recv_params.cb_ctx = p_service_context;

    fpfw_status_t icc_base_status = fpfw_icc_base_recv(p_service_context->icc.p_hsp_icc_ctx, &hsp_recv_params);
    BUG_ASSERT_PARAM(icc_base_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_base_status, 0);
}

static void etr_notify_hsp(etr_service_context_t* p_service_context)
{
    static kng_hsp_mailbox_msg hsp_send_msg;

    /* Setup the send message */
    hsp_send_msg.header.cmd = HSP_MAILBOX_CMD_SEND_LOG_RSP;

    /* The HSP ICC Base context does not setup a max sync attempt count, making this an infinite loop until
    the send is successful.*/
    fpfw_status_t icc_base_status =
        fpfw_icc_base_send_sync(p_service_context->icc.p_hsp_icc_ctx, &hsp_send_msg, sizeof(hsp_send_msg));

    BUG_ASSERT_PARAM(icc_base_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_base_status, 0);

    /* Setup the next receive request */
    etr_receive_hsp(p_service_context);
}

/**
 * @brief Handle a copy buffer request from any HSP received via Mailbox.
 *
 * @param p_service_context -> Pointer to the ETR service context
 * @param buffer_addr -> DDR address of the buffer with HSP Logs
 *
 * @return FPFW_STATUS_SUCCESS if the buffer was copied successfully, FPFW_STATUS_FAIL otherwise.
 */
static fpfw_status_t etr_copy_hsp_telemetry(etr_service_context_t* p_service_context, p_hsp_log_payload_header_t p_payload_header)
{
    /* Find the first HSP buffer that is free */
    bool found_free_buffer = false;
    for (unsigned int buffer_num = ASIC_BUFFER_DDR_CAPACITY_MAX; buffer_num < ETR_DDR_BUFFERS_CAPACITY_MAX; buffer_num++)
    {
        if (p_service_context->ddr_buffers[buffer_num].state == ETR_DDR_BUFFER_STATE_FREE &&
            p_service_context->ddr_buffers[buffer_num].type == DIAG_PAYLOAD_PARSER_HSP_TRACE)
        {
            found_free_buffer = true;

            p_ddr_buffer_info_t p_active_hsp_buffer = &p_service_context->ddr_buffers[buffer_num];

            /* Set buffer state to pending and update size. Since we use one buffer for one payload, there's no ACTIVE state */
            p_active_hsp_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
            p_active_hsp_buffer->payload_management.size_bytes = p_payload_header->output_header.manifest_size +
                                                                 p_payload_header->output_header.log_size +
                                                                 (uint64_t)sizeof(hsp_log_payload_header_t);

            memcpy((void*)p_active_hsp_buffer->payload_management.base_addr,
                   (void*)p_payload_header,
                   p_active_hsp_buffer->payload_management.size_bytes);

            /* Log the contents of payload_header */
            FPFW_ET_LOG(HspBufferProc,
                        p_payload_header->output_header.header_version,
                        p_payload_header->output_header.message_version,
                        p_payload_header->output_header.platform_id,
                        p_payload_header->output_header.manifest_size,
                        p_payload_header->output_header.log_size);

            break;
        }
    }

    etr_notify_hsp(p_service_context);

    if (!found_free_buffer)
    {
        /* TODO (ADO3102046) This should be a warning. Leaving it at Verbose since this is filling up the MCP Trace (no AP read of telemetry)*/
        FPFW_ET_LOG_ETR_ASCII_VERBOSE("HSP buff full, dumping payload");
        return FPFW_STATUS_FAIL;
    }

    return FPFW_STATUS_SUCCESS;
}

static p_hsp_log_payload_header_t decode_hsp_telemetry_atu_mapped_address(uint64_t hsp_payload_addr)
{
    uint32_t atu_mapped_addr =
        (uint32_t)(MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR +
                   (hsp_payload_addr - IB_TLM_DDR_ATU_AP_ADDR_TRACE_HSP_BASE_ADDR(idsw_get_die_id())));

    return (p_hsp_log_payload_header_t)atu_mapped_addr;
}

void etr_icc_handle_hsp(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    etr_service_context_t* p_service_context = (etr_service_context_t*)context;
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, (uintptr_t)p_service_context, status);

    uint64_t hsp_payload_addr = ((uint64_t)hsp_recv_msg.buffer_lower + ((uint64_t)hsp_recv_msg.buffer_upper << 32));

    /* ATU Map the payload address */
    p_hsp_log_payload_header_t hsp_payload_atu_addr = decode_hsp_telemetry_atu_mapped_address(hsp_payload_addr);
    FPFW_ET_LOG(HspReq, hsp_payload_addr, (uint32_t)hsp_payload_atu_addr);

#ifdef _WIN32 // Unit Test Only
    if (override_atu_test_addr != NULL)
    {
        hsp_payload_atu_addr = override_atu_test_addr;
    }
#endif
    /* Copy the payload data to the appropriate location */
    fpfw_status_t status_copy_hsp_buffer = etr_copy_hsp_telemetry(p_service_context, hsp_payload_atu_addr);

    /* Send a notification to AP */
    if (idsw_get_die_id() == DIE_0)
    {
        if (status_copy_hsp_buffer == FPFW_STATUS_SUCCESS)
        {
            FPFW_ET_LOG_ETR_ASCII_INFO("Die 0 - notifying AP");
            mts_client_send_dcp_notification(MTS_CLIENT_ID_EVENT_TRACE, DCP_NOTIFICATION_TYPE_DATA_AVAILABLE);
        }
    }
    else
    {
        FPFW_ET_LOG_ETR_ASCII_INFO("Die 1 - notifying MCP D0");
        /* This is a stub, update later (TODO) */
    }
}

/*----------------------------- Global Functions ----------------------------*/
void etr_initialize_hsp_communication(etr_service_context_t* p_service_context, const etr_service_config_t* p_config)
{
    /*
    The ETR communicates with the HSP by:
        1. Receiving a request from the HSP, containing it's buffer information it wants us to upload
        2. Sending a response back to the HSP, once the buffer has been copied / read elsewhere
    */

    BUG_ASSERT(p_config->icc_config.p_hsp_icc_ctx != NULL);
    p_service_context->icc.p_hsp_icc_ctx = p_config->icc_config.p_hsp_icc_ctx;

    etr_receive_hsp(p_service_context);
}

#ifdef _WIN32 // Unit Test Only
void etr_set_override_atu_test_address(p_hsp_log_payload_header_t atu_test_addr)
{
    override_atu_test_addr = atu_test_addr;
}
#endif
