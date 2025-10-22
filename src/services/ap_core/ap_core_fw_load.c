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
#include <ift_fw.h>
#include <in_band_telemetry_ddr.h>
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
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
// https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc={F8844B94-FFCC-4B46-8043-7D75085AEC0B}&file=KG
// FW Architecture.docx&action=default&mobileredirect=true Section 6.7
#define KMP_LOAD_ADDRESS  0XFFFFFF0480000
#define KMP_START_ADDRESS 0x00080080

enum
{
    HSP_FIRMWARE_ID_IFT_MEM_TEST = 0,
    HSP_FIRMWARE_ID_IFT_CORE_TEST0 = 1,
    HSP_FIRMWARE_ID_IFT_CORE_TEST1 = 2,
    HSP_FIRMWARE_ID_IFT_CORE_TEST2 = 3,
    HSP_FIRMWARE_ID_IFT_CORE_TEST3 = 4,
    HSP_FIRMWARE_ID_IFT_CORE_TEST4 = 5,
    HSP_FIRMWARE_ID_IFT_CORE_TEST5 = 6,
    HSP_FIRMWARE_ID_IFT_CORE_TEST6 = 7,
    HSP_FIRMWARE_ID_IFT_CORE_TEST7 = 8,
    HSP_FIRMWARE_ID_IFT_CORE_TEST8 = 9,
    HSP_FIRMWARE_ID_IFT_CORE_TEST9 = 10,
    HSP_FIRMWARE_ID_IFT_CORE_TEST10 = 11,
    HSP_FIRMWARE_ID_IFT_CORE_TEST11 = 12,
    HSP_FIRMWARE_ID_IFT_CORE_TEST12 = 13,
    HSP_FIRMWARE_ID_IFT_CORE_TEST13 = 14,
    HSP_FIRMWARE_ID_IFT_CORE_TEST14 = 15,
    HSP_FIRMWARE_ID_IFT_CORE_TEST15 = 16,
    HSP_FIRMWARE_ID_IFT_CORE_TEST16 = 17,
    HSP_FIRMWARE_ID_IFT_CORE_TEST17 = 18,
    HSP_FIRMWARE_ID_IFT_CORE_TEST18 = 19,
    HSP_FIRMWARE_ID_IFT_CORE_TEST19 = 20,
    HSP_FIRMWARE_ID_IFT_CORE_TEST20 = 21,
    HSP_FIRMWARE_ID_IFT_CORE_TEST21 = 22,
    HSP_FIRMWARE_ID_IFT_CORE_TEST22 = 23,
    HSP_FIRMWARE_ID_IFT_CORE_TEST23 = 24,
    HSP_FIRMWARE_ID_IFT_CORE_TEST24 = 25,
    HSP_FIRMWARE_ID_IFT_CORE_TEST25 = 26,
    HSP_FIRMWARE_ID_IFT_CORE_TEST26 = 27,
    HSP_FIRMWARE_ID_IFT_CORE_TEST27 = 28,
    HSP_FIRMWARE_ID_IFT_CORE_TEST28 = 29,
    HSP_FIRMWARE_ID_IFT_CORE_TEST29 = 30,
    HSP_FIRMWARE_ID_IFT_CORE_TEST30 = 31,
    HSP_FIRMWARE_ID_IFT_CORE_TEST31 = 32,
    HSP_FIRMWARE_ID_IFT_CORE_TEST32 = 33,
    HSP_FIRMWARE_ID_IFT_CORE_TEST33 = 34,
    HSP_FIRMWARE_ID_IFT_CORE_TEST34 = 35,
    HSP_FIRMWARE_ID_IFT_CORE_TEST35 = 36,
    HSP_FIRMWARE_ID_IFT_CORE_TEST36 = 37,
    HSP_FIRMWARE_ID_IFT_CORE_TEST37 = 38,
    HSP_FIRMWARE_ID_IFT_CORE_TEST38 = 39,
    HSP_FIRMWARE_ID_IFT_CORE_TEST39 = 40,
    HSP_FIRMWARE_ID_IFT_CORE_TEST40 = 41,
};

struct kng_hsp_mailbox_cmd_load_fw_64bit_ift_rsp
{
    struct kng_hsp_mailbox_msg_header header; /**< msg header containing cmd, seq,context and flags. */
    uint32_t status;                          /**< return status code. */
    uint32_t ift_fw_size;                     /**< firmware size. */
};

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
static ap_fw_id_t last_requested_fw_id;

/*------------- Functions ----------------*/
static void request_send_complete_cb(void* context, fpfw_status_t status)
{
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, context);

    if (context != NULL)
    {
        ap_fw_id_t fw_id = *(ap_fw_id_t*)context;
        APCORE_LOG_INFO("AP FW load request sent: id(%d)", fw_id);
    }
}

static void request_ap_load_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    if (context != NULL)
    {
        ap_fw_id_t fw_id = *(ap_fw_id_t*)context;
        APCORE_LOG_INFO("AP FW load response received: id (%d)", fw_id);
    }
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

    last_requested_fw_id = AP_FW_ID_MCP;

    static union _icc_max_msg_size_padding mcp_start_req = {
        .start_req.header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
        .start_req.id = HSP_FIRMWARE_ID_MCP,
        .start_req.address = MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS + HSP_BOOT_RAM_META_DATA_SIZE};

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &mcp_start_req,
        .cb = request_send_complete_cb,
        .cb_ctx = &last_requested_fw_id,
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

static void request_mscp_manifest_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("MSCP Manifest load completed");
}

static void request_accel_manifest_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT(recv_payload_buffer.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_payload_buffer.rsp.status == 0);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());

    APCORE_LOG_INFO("Accel Manifest load completed");
}

static void request_kmp_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    last_requested_fw_id = AP_FW_ID_KMP;

    static union _icc_max_msg_size_padding kmp_start_req = {
        .start_req.header.cmd = HSP_MAILBOX_CMD_START_CORE_REQ,
        .start_req.id = HSP_FIRMWARE_ID_KMP,
        .start_req.address = KMP_START_ADDRESS,
    };

    static fpfw_icc_base_send_req_t send_params = {
        .payload_buffer = &kmp_start_req,
        .cb = request_send_complete_cb,
        .cb_ctx = &last_requested_fw_id,
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

    // Start the boot timer for given accel.
    status = accel_start_boot_status_timer(accel_type);
    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);

    // Enable the error interrupts at this point
    accel_intr_scp_err_intr_enable(accel_type);

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

    last_requested_fw_id = fw_id;

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
    else if (fw_id == AP_FW_ID_MSCP_MANIFEST)
    {
        FPFW_DBGPRINT_INFO("MSCP Manifest load requested");
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
        recv_params.cb = request_mscp_manifest_load_complete_notify;
        recv_params.cb_ctx = icc_hspmbx_ctx;
    }
    else if (fw_id == AP_FW_ID_ACCEL_MANIFEST)
    {
        FPFW_DBGPRINT_INFO("Accelerator Manifest load requested");
        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP;
        recv_params.cb = request_accel_manifest_load_complete_notify;
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
        static ap_fw_id_t last_loaded_fw_id;
        last_loaded_fw_id = fw_id;

        recv_params.recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_RSP;
        recv_params.cb = request_ap_load_recv_complete_notify;
        recv_params.cb_ctx = &last_loaded_fw_id;
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
        send_request.load_fw_req.address = SCP_EXP_MSCP_BOOT_DATA_BASE;
        break;
    case AP_FW_ID_MSCP_MANIFEST:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_MSCP_MANIFEST;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;

        uint64_t mscp_manifest_load_addr =
            IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR +
            EVT_TELEMETRY_MANIFEST_GET_DDR_OFFSET(IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR);
        send_request.load_fw_64bit_req.load_addr_low = mscp_manifest_load_addr & (uint32_t)0xFFFFFFFF;
        send_request.load_fw_64bit_req.load_addr_high = mscp_manifest_load_addr >> 32;
        break;
    case AP_FW_ID_ACCEL_MANIFEST:
        send_request.load_fw_64bit_req.id = HSP_FIRMWARE_ID_ACCEL_MANIFEST;
        send_request.load_fw_64bit_req.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;

        uint64_t accel_manifest_load_addr =
            IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR +
            EVT_TELEMETRY_MANIFEST_GET_DDR_OFFSET(IB_TLM_DDR_ATU_AP_SDM_CDED_STAGING_MANIFEST_BASE_ADDR);
        send_request.load_fw_64bit_req.load_addr_low = accel_manifest_load_addr & (uint32_t)0xFFFFFFFF;
        send_request.load_fw_64bit_req.load_addr_high = accel_manifest_load_addr >> 32;
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
        .cb_ctx = &last_requested_fw_id,
        .buffer_size = sizeof(send_request),
    };

    status = fpfw_icc_base_send(icc_hspmbx_ctx, &send_params);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, fw_id);
    APCORE_LOG_INFO("Request FW load initiated (%d)", fw_id);
}

static void request_ap_ift_load_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    struct kng_hsp_mailbox_cmd_load_fw_64bit_ift_rsp* recv_ift_resp = (void*)&recv_payload_buffer;

    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);
    BUG_ASSERT_PARAM(recv_ift_resp->header.cmd == HSP_MAILBOX_CMD_IFT_LOAD_FW_64BIT_RSP,
                     recv_ift_resp->header.cmd,
                     HSP_MAILBOX_CMD_IFT_LOAD_FW_64BIT_RSP);
    BUG_ASSERT(recv_ift_resp->status == 0);
    BUG_ASSERT_PARAM(recv_ift_resp->ift_fw_size > 0, recv_ift_resp->ift_fw_size, 0);
    BUG_ASSERT_PARAM(recv_ift_resp->ift_fw_size % BYTES_IN_WORD32 == 0, recv_ift_resp->ift_fw_size, 0);

    ap_ift_fw_id_t fw_id = (ap_ift_fw_id_t)(uintptr_t)context;
    ift_set_current_fw_size(recv_ift_resp->ift_fw_size);

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)ap_core_get_outstanding_request());
    APCORE_LOG_TRACE("IFT FW[%d] load response received", (int)fw_id);
}

void ap_core_request_load_ift_fw(fpfw_icc_base_ctx_t* icc_hspmbx_ctx, ap_ift_fw_id_t fw_id)
{
    static fpfw_icc_base_recv_req_t recv_params = {};

    recv_params.recv_cmd_code = HSP_MAILBOX_CMD_IFT_LOAD_FW_64BIT_RSP;
    recv_params.payload_buffer = &recv_payload_buffer;
    recv_params.buffer_size = sizeof(recv_payload_buffer);
    recv_params.cb = request_ap_ift_load_recv_complete_notify;
    recv_params.cb_ctx = (void*)(uintptr_t)fw_id;

    fpfw_status_t status = fpfw_icc_base_recv(icc_hspmbx_ctx, &recv_params);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);

    static kng_hsp_mailbox_cmd_load_fw_64bit_req send_request = {};

    send_request.header.cmd = HSP_MAILBOX_CMD_IFT_LOAD_FW_64BIT_REQ;

    switch (fw_id)
    {
    case AP_IFT_FW_ID_IFT_MEM_TEST:
        send_request.id = HSP_FIRMWARE_ID_IFT_MEM_TEST;
        send_request.load_addr_low = ift_get_ift_fw_addr();
        send_request.load_addr_high = 0;
        break;
    case AP_IFT_FW_ID_IFT_CORE_TEST:
        send_request.id = HSP_FIRMWARE_ID_IFT_CORE_TEST0 + ift_get_core_test_fw_idx();
        send_request.load_addr_low = ift_get_ift_fw_addr();
        send_request.load_addr_high = 0;
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

    APCORE_LOG_TRACE("Request IFT FW load sent for FW ID: [%d]", fw_id);
}
