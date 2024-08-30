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
#define __NO_CSR_TYPEDEFS__
#include <mcp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <mcp_exp_top_regs.h>
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
// TODO: Get these addresses from a shared header file once maintained by host services
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2006871
#define TFA_FW_LOAD_ADDRESS     0x10000
#define BL33_FW_LOAD_ADDRESS    0xE0000000
#define HAFNIUM_FW_LOAD_ADDRESS 0xF8FE0000
#define STMM_FW_LOAD_ADDRESS    0xF9300000
#define SPMC_FW_LOAD_ADDRESS    0XFD3DF000
#define RMM_FW_LOAD_ADDRESS     0xFD400000

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static kng_hsp_mailbox_msg recv_payload_buffer;

/*------------- Functions ----------------*/
static void request_send_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static void request_ap_load_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_TRACE("AP FW load response received");
}

static void request_mcp_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    union _icc_max_msg_size_padding
    {
        struct kng_hsp_mailbox_cmd_start_core_req start_req;
        uint32_t as_uint32_t[4];
    };
    // The fpfw_icc_base_send needs a total of 16 bytes in payload buffer and the start request struct is not
    // large enough Hence creating a temporary union to allocate a larger size buffer
    // TODO: Replace with proper definition from HSP headers once published
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2006032
    static union _icc_max_msg_size_padding mcp_start_req = {
        .start_req.header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
        .start_req.id = HSP_FIRMWARE_ID_MCP,
        .start_req.address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS + HSP_BOOT_RAM_META_DATA_SIZE};

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &mcp_start_req,
        .cb = request_send_complete_cb,
        .cb_ctx = NULL,
        .buffer_size = sizeof(mcp_start_req),
    };

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    fpfw_icc_base_ctx_t* p_icc_hspmbx_ctx = (fpfw_icc_base_ctx_t*)context;
    status = fpfw_icc_base_send(p_icc_hspmbx_ctx, &send_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("MCP FW load completed - Requesting load core now");
}

void ap_core_request_load_ap_fw(fpfw_icc_base_ctx_t* icc_hspmbx_ctx, ap_fw_id_t fw_id)
{
    // Listen for the response
    // TODO: Use send_recv API instead once HSP respects sequence number
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1893482
    static fpfw_icc_base_recv_req_t recv_params = {
        .payload_buffer = &recv_payload_buffer,
        .buffer_size = sizeof(recv_payload_buffer),
        .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP,
        .cb_ctx = NULL,
    };

    recv_params.cb = request_ap_load_recv_complete_notify;
    if (fw_id == AP_FW_ID_MCP)
    {
        recv_params.cb = request_mcp_load_complete_notify;
        recv_params.cb_ctx = icc_hspmbx_ctx;
    }

    fpfw_status_t status = fpfw_icc_base_recv(icc_hspmbx_ctx, &recv_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    // TODO: Fill out the request with appropriate values
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1893482
    //! Prepare send request
    static kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ,
        .header.context = 0,
        .size = 0x00000000,
    };

    switch (fw_id)
    {
    case AP_FW_ID_BL31:
        send_request.id = HSP_FIRMWARE_ID_PE_SECURITY_MONITOR;
        send_request.address = TFA_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_STMM:
        send_request.id = HSP_FIRMWARE_ID_PE_MANAGEMENT_MODE;
        send_request.address = STMM_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_BL33:
        send_request.id = HSP_FIRMWARE_ID_PE_UEFI;
        send_request.address = BL33_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_HAFNIUM:
        send_request.id = HSP_FIRMWARE_ID_PE_SPM;
        send_request.address = HAFNIUM_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_RMM:
        send_request.id = HSP_FIRMWARE_ID_RMM;
        send_request.address = RMM_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_SPMC:
        send_request.id = HSP_FIRMWARE_ID_SPMC_MANIFEST;
        send_request.address = SPMC_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_MCP:
        send_request.id = HSP_FIRMWARE_ID_MCP;
        send_request.address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS;
        break;
    default:
        BUG_ASSERT(false);
        break;
    }

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &send_request,
        .cb = request_send_complete_cb,
        .cb_ctx = NULL,
        .buffer_size = sizeof(send_request),
    };

    status = fpfw_icc_base_send(icc_hspmbx_ctx, &send_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    APCORE_LOG_TRACE("Request FW load sent for FW ID: [%d]", fw_id);
}
