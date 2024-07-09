//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_fw_load.c
 * Implements the APcore FW load request APIs
 */

/*------------- Includes -----------------*/
#include "ap_core_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send, fpfw_icc_base...
#include <fpfw_status.h>          // for fpfw_init_get_handle, FPFW_INIT_S...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static kng_hsp_mailbox_msg recv_payload_buffer;

/*------------- Functions ----------------*/
static void request_load_tfa_send_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static void request_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_TRACE("Request FW load received");
}

void ap_core_request_load_tfa(fpfw_icc_base_ctx_t* icc_hspmbx_ctx)
{
    // Listen for the response
    // TODO: Use send_recv API instead once HSP respects sequence number
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1893482
    static fpfw_icc_base_recv_req_t recv_params = {
        .payload_buffer = &recv_payload_buffer,
        .buffer_size = sizeof(recv_payload_buffer),
        .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP,
        .cb = request_recv_complete_notify,
        .cb_ctx = NULL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_hspmbx_ctx, &recv_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    // TODO: Fill out the request with appropriate values
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1893482
    //! Prepare send request
    static kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .id = HspFirmwareIdPeManagementMode,
        .address = 0x00000000,
        .size = 0x00000000,
    };

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &send_request,
        .cb = request_load_tfa_send_complete_cb,
        .cb_ctx = NULL,
        .buffer_size = sizeof(send_request),
    };

    status = fpfw_icc_base_send(icc_hspmbx_ctx, &send_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    APCORE_LOG_TRACE("Request TFA load sent");
}
