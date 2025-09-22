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
#include <hsp_firmware_headers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/* Mailbox Payloads and RRequests */
static kng_hsp_mailbox_msg hsp_recv_msg;
static kng_hsp_mailbox_msg hsp_send_msg;

static fpfw_icc_base_recv_req_t hsp_recv_params;

/*----------------------------- Static Functions ----------------------------*/

static void etr_receive_hsp(etr_service_context_t* p_service)
{
    /* Setup the receive request. The send request is setup as needed before sending it to the hsp */
    memset(&hsp_recv_msg, 0, sizeof(hsp_recv_msg));
    memset(&hsp_recv_params, 0, sizeof(hsp_recv_params));
    hsp_recv_params.payload_buffer = &hsp_recv_msg;
    hsp_recv_params.buffer_size = sizeof(hsp_recv_msg);
    hsp_recv_params.recv_cmd_code = HSP_MAILBOX_CMD_SEND_LOG_REQ;
    hsp_recv_params.cb = etr_icc_handle_hsp;
    hsp_recv_params.cb_ctx = p_service;

    fpfw_status_t icc_base_status = fpfw_icc_base_recv(p_service->icc.p_hsp_icc_ctx, &hsp_recv_params);
    BUG_ASSERT_PARAM(icc_base_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_base_status, 0);
}

static void etr_notify_hsp(etr_service_context_t* p_service, struct kng_hsp_mailbox_cmd_send_log_req* p_req)
{
    FPFW_UNUSED(p_req);

    hsp_send_msg.header.cmd = HSP_MAILBOX_CMD_SEND_LOG_RSP;

    /* The HSP ICC Base context does not setup a max sync attempt count, making this an infinite loop until the send is successful.*/
    fpfw_status_t icc_base_status =
        fpfw_icc_base_send_sync(p_service->icc.p_hsp_icc_ctx, &hsp_send_msg, sizeof(hsp_send_msg));

    BUG_ASSERT_PARAM(icc_base_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_base_status, 0);

    /* Setup the next receive request */
    etr_receive_hsp(p_service);
}

void etr_icc_handle_hsp(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    etr_service_context_t* p_service = (etr_service_context_t*)context;
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, (uintptr_t)p_service, status);

    struct kng_hsp_mailbox_cmd_send_log_req* p_req = (struct kng_hsp_mailbox_cmd_send_log_req*)hsp_recv_params.payload_buffer;

    /* For now send the HSP a response back to let it know we are done with the buffer from the request */
    etr_notify_hsp(p_service, p_req);
}

/*----------------------------- Global Functions ----------------------------*/
void etr_initialize_hsp_communication(etr_service_context_t* p_service, const etr_service_config_t* p_config)
{

    /*
    The ETR communicates with the HSP by:
        1. Receiving a request from the HSP, containing it's buffer information it whats us to offload
        2. Sending a response back to the HSP, once the buffer has been copied / read elsewhere
    */

    BUG_ASSERT_PARAM(p_config->icc_config.p_hsp_icc_ctx != NULL, FPFW_ET_E_INVALIDARG, 0);

    p_service->icc.p_hsp_icc_ctx = p_config->icc_config.p_hsp_icc_ctx;

    etr_receive_hsp(p_service);
}