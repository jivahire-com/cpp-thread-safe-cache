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
#include <accelerator_ip.h>
#include <accelerator_knobs.h>
#include <accelip_id.h>
#include <ap_fw_info.h>
#include <assert.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send, fpfw_icc_base...
#include <fpfw_status.h>          // for fpfw_init_get_handle, FPFW_INIT_S...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <mcp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <mcp_exp_top_regs.h>
#include <mscp_exp_rmss_memory_map.h>
#include <pcie_phy_load_events.h> // PhyFW load event
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
// https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc={F8844B94-FFCC-4B46-8043-7D75085AEC0B}&file=KG
// FW Architecture.docx&action=default&mobileredirect=true Section 6.7
#define KMP_LOAD_ADDRESS  0XFFFFFF0480000
#define KMP_START_ADDRESS 0x00080080

/*------------- Typedefs -----------------*/
// The fpfw_icc_base_send needs a total of 16 bytes in payload buffer and the start request struct is not
// large enough
// TODO: Replace with proper definition from HSP headers once published
// https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2006032
union _icc_max_msg_size_padding
{
    struct kng_hsp_mailbox_cmd_start_core_req start_req;
    uint32_t as_uint32_t[4];
};
static_assert(sizeof(union _icc_max_msg_size_padding) == 16, "Size of hsp mbox payload is not 16 bytes");

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

static void request_pcie_phy_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    // Set Pcie Phy FW Load Event Flag
    pcie_phyfw_set_load_event(&pcie_phyfw_load_event);
    APCORE_LOG_INFO("PCIE PHY FW load [0x%x] completed.", SCP_EXP_PCIE_PHY_FW_BASE);
}

static void request_mcp_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

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

static void request_kmp_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    static union _icc_max_msg_size_padding kmp_start_req = {
        .start_req.header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
        .start_req.id = HSP_FIRMWARE_ID_KMP,
        .start_req.address = KMP_START_ADDRESS,
    };

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &kmp_start_req,
        .cb = request_send_complete_cb,
        .cb_ctx = NULL,
        .buffer_size = sizeof(kmp_start_req),
    };

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    fpfw_icc_base_ctx_t* p_icc_hspmbx_ctx = (fpfw_icc_base_ctx_t*)context;
    status = fpfw_icc_base_send(p_icc_hspmbx_ctx, &send_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("KMP FW load completed - Requesting load core now");
}

static void request_accel_itcm_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    ACCEL_ID accel_type = (ACCEL_ID)context;

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("Accel[%d] ITCM FW load completed", accel_type);
}

static void request_rmm_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("RMM FW load completed");
}

static void request_accel_dtcm_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    ACCEL_ID accel_type = (ACCEL_ID)context;

    scp_download_accel_knobs(accel_type);
    accel_disable_cpu_wait(accel_type);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("Accel[%d] DTCM FW load completed", accel_type);
}

void ap_core_request_load_ap_fw(fpfw_icc_base_ctx_t* icc_hspmbx_ctx, ap_fw_id_t fw_id)
{
    //! Prepare for recv response
    // TODO: Use send_recv API instead once HSP respects sequence number
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1893482
    static fpfw_icc_base_recv_req_t recv_params = {
        .payload_buffer = &recv_payload_buffer,
        .buffer_size = sizeof(recv_payload_buffer),
        .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP, //! default,  resp code for `HSP_MAILBOX_CMD_LOAD_FW_REQ` cmd
        .cb_ctx = NULL,
    };

    if (fw_id == AP_FW_PCIE_PHY)
    {
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP;
        recv_params.cb = request_pcie_phy_load_complete_notify;
        recv_params.cb_ctx = icc_hspmbx_ctx;
    }
    else if (fw_id == AP_FW_ID_MCP)
    {
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP;
        recv_params.cb = request_mcp_load_complete_notify;
        recv_params.cb_ctx = icc_hspmbx_ctx;
    }
    else if (fw_id == AP_FW_ID_KMP)
    {
        recv_params.recv_cmd_code =
            HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP; //! resp code for `HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ` cmd
        recv_params.cb = request_kmp_load_complete_notify;
        recv_params.cb_ctx = icc_hspmbx_ctx;
    }
    else if (fw_id == AP_FW_ID_SDM_ITCM || fw_id == AP_FW_ID_CDED_ITCM)
    {
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
        recv_params.cb = request_accel_itcm_load_complete_notify;
        recv_params.cb_ctx = (void*)(fw_id == AP_FW_ID_SDM_ITCM ? ACCEL_ID_SDM : ACCEL_ID_CDED);
    }
    else if (fw_id == AP_FW_ID_SDM_DTCM || fw_id == AP_FW_ID_CDED_DTCM)
    {
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
        recv_params.cb = request_accel_dtcm_load_complete_notify;
        recv_params.cb_ctx = (void*)(fw_id == AP_FW_ID_SDM_DTCM ? ACCEL_ID_SDM : ACCEL_ID_CDED);
    }
    else if (fw_id == AP_FW_ID_RMM)
    {
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
        recv_params.cb = request_rmm_load_complete_notify;
        recv_params.cb_ctx = (void*)(fw_id);
    }
    else
    {
        //! Default case for all other firmware IDs that use
        //! req cmd code HSP_MAILBOX_CMD_LOAD_FW_RSP to send & resp code HSP_MAILBOX_CMD_LOAD_FW_REQ to recv
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP;
        recv_params.cb = request_ap_load_recv_complete_notify;
        recv_params.cb_ctx = NULL;
    }
    fpfw_status_t status = fpfw_icc_base_recv(icc_hspmbx_ctx, &recv_params);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);

    //! Prepare send request
    typedef union
    {
        kng_hsp_mailbox_cmd_load_fw_req load_fw_req;
        kng_hsp_mailbox_cmd_load_fw_64bit_req load_fw_64bit_req;
    } hsp_mailbox_cmd_load_fw_req_t;

    static hsp_mailbox_cmd_load_fw_req_t send_request = {};
    memset(&send_request, 0, sizeof(send_request));
    send_request.load_fw_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_REQ;

    switch (fw_id)
    {
    case AP_FW_ID_BL31:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_PE_SECURITY_MONITOR;
        send_request.load_fw_req.address = TFA_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_STMM:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_PE_MANAGEMENT_MODE;
        send_request.load_fw_req.address = STMM_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_BL33:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_PE_UEFI;
        send_request.load_fw_req.address = BL33_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_HAFNIUM:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_PE_SPM;
        send_request.load_fw_req.address = HAFNIUM_FW_LOAD_ADDRESS;
        break;
    case AP_FW_ID_RMM:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_RMM;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        send_request.load_fw_64bit_req.load_addr_low = REALM_FW_LOAD_ADDRESS & (uint32_t)0xFFFFFFFF;
        send_request.load_fw_64bit_req.load_addr_high = REALM_FW_LOAD_ADDRESS >> 32;
        break;
    case AP_FW_ID_RP_EXE:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_RP_EXE;
        send_request.load_fw_req.address = RP_EXE_LOAD_ADDRESS;
        break;
    case AP_FW_ID_RP_DATA:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_RP_DATA;
        send_request.load_fw_req.address = RP_DATA_LOAD_ADDRESS;
        break;
    case AP_FW_PCIE_PHY:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_PCIE_PHY;
        send_request.load_fw_req.address = SCP_EXP_PCIE_PHY_FW_BASE;
        break;
    case AP_FW_ID_MCP:
        send_request.load_fw_req.id = HSP_FIRMWARE_ID_MCP;
        send_request.load_fw_req.address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS;
        break;
    case AP_FW_ID_SDM_ITCM:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_SDM_ITCM;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        accel_get_itcm_addr(ACCEL_ID_SDM,
                            &send_request.load_fw_64bit_req.load_addr_low,
                            &send_request.load_fw_64bit_req.load_addr_high);
        break;
    case AP_FW_ID_SDM_DTCM:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_SDM_DTCM;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        accel_get_dtcm_addr(ACCEL_ID_SDM,
                            &send_request.load_fw_64bit_req.load_addr_low,
                            &send_request.load_fw_64bit_req.load_addr_high);
        break;
    case AP_FW_ID_CDED_ITCM:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_CDED_ITCM;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        accel_get_itcm_addr(ACCEL_ID_CDED,
                            &send_request.load_fw_64bit_req.load_addr_low,
                            &send_request.load_fw_64bit_req.load_addr_high);
        break;
    case AP_FW_ID_CDED_DTCM:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_CDED_DTCM;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        accel_get_dtcm_addr(ACCEL_ID_CDED,
                            &send_request.load_fw_64bit_req.load_addr_low,
                            &send_request.load_fw_64bit_req.load_addr_high);
        break;
    case AP_FW_ID_KMP:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_KMP;
        send_request.load_fw_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
        send_request.load_fw_64bit_req.load_addr_low = KMP_LOAD_ADDRESS & (uint32_t)0xFFFFFFFF;
        send_request.load_fw_64bit_req.load_addr_high = KMP_LOAD_ADDRESS >> 32;
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
