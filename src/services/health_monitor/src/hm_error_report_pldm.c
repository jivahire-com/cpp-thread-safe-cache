//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_pldm.c
 * Implements transfer CPER via PLDM.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_pldm_service.h>
#include <fpfw_status.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <icc_platform_defines.h>
#include <libpldm/platform.h>
#include <mscp_exp_rmss_memory_map.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void hm_transfer_cper_to_bmc_local();

/*-- Declarations (Statics and globals) --*/
static volatile bool cper_transfer_ongoing = false;
static volatile bool cper_transfer_reroute_ongoing = false;

/*------------- Functions ----------------*/
static void hm_transfer_cper_completion_pldm_cb(fpfw_pldm_cc_t completionCode, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (completionCode == FPFW_PLDM_CC_SUCCESS)
    {
        HM_LOG_INFO("PLDM CPER transfer succeeded.");
    }
    else
    {
        HM_LOG_CRIT("PLDM CPER transfer failed(%d)", completionCode);
    }

    cper_transfer_ongoing = false;
}

static void hm_cper_transfer_reroute_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);

    cper_transfer_reroute_ongoing = false;
}

static void hm_mcp_cper_transfer_d2d_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    hm_transfer_cper_to_bmc_local();

    hm_config_t* hm_config = get_hm_config();
    hm_cper_transfer_listener_from_secondary_mcp(hm_config->icc_ctx[HM_INTERCORE_LOCAL]);
}

static void hm_mcp_cper_transfer_listener_from_scp_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(status);
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    hm_transfer_cper_to_bmc();

    hm_config_t* hm_config = get_hm_config();
    hm_cper_transfer_listener_from_scp(hm_config->icc_ctx[HM_INTERCORE_REMOTE]);
}

void hm_transfer_cper_to_bmc_local()
{
    if (cper_transfer_ongoing)
    {
        HM_LOG_INFO("Another PLDM CPER transfer is already in progress.");
        return;
    }

    cper_transfer_ongoing = true;
    volatile uint8_t* rmss_cper_record_base = (volatile uint8_t*)(uintptr_t)get_hm_config()->mscp_full_cper_record_base;
    BUG_ASSERT_PARAM(rmss_cper_record_base != NULL, rmss_cper_record_base, 0);

    static acpi_cper_record_t full_cper;

    volatile uint8_t* src = rmss_cper_record_base;
    uint8_t* dst = (uint8_t*)&full_cper;

    wait_for_semaphore(get_hm_config()->semaphore_id, get_hm_config()->semaphore_key);
    for (size_t i = 0; i < sizeof(acpi_cper_record_t); i++)
    {
        dst[i] = src[i];
    }
    release_semaphore(get_hm_config()->semaphore_id);

    static fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = PLDM_CPER_EVENT,
                                                              .event_payload_size = sizeof(acpi_cper_record_t)};

    descriptor.event_payload = &full_cper;

    static pldm_platform_event_config_t event = {.p_descriptor = &descriptor};

    static pldm_platform_event_notification notification = {.CallBack = hm_transfer_cper_completion_pldm_cb,
                                                            .context = NULL};

    fpfw_status_t status = fpfw_pldm_service_raise_platform_event(&event, &notification);

    if (FPFW_STATUS_SUCCEEDED(status))
    {
    }
    else
    {
        cper_transfer_ongoing = false;
    }
}

void hm_transfer_cper_to_bmc()
{
    if (get_hm_config()->is_mcp == false)
    {
        return;
    }

    if (get_hm_config()->is_primary)
    {
        hm_transfer_cper_to_bmc_local();
    }
    else
    {
        HM_LOG_INFO("PLDM transfer req on secondary mcp, re-route\n");

        if (cper_transfer_reroute_ongoing)
        {
            return;
        }

        cper_transfer_reroute_ongoing = true;
        static uint8_t s_icc_mhu_pldm_req_payload[sizeof(icc_mhu_header_t) + 1] = {0};
        icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)s_icc_mhu_pldm_req_payload;

        p_send_req->header.msg_header.command = ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP;
        p_send_req->header.msg_header.payload_size = 1;

        static fpfw_icc_base_send_req_t cper_pldm_transfer_req = {0};
        cper_pldm_transfer_req.payload_buffer = s_icc_mhu_pldm_req_payload;
        cper_pldm_transfer_req.buffer_size = sizeof(s_icc_mhu_pldm_req_payload);
        cper_pldm_transfer_req.cb = hm_cper_transfer_reroute_cb;
        cper_pldm_transfer_req.cb_ctx = s_icc_mhu_pldm_req_payload;

        fpfw_status_t status = fpfw_icc_base_send(get_hm_config()->icc_ctx[HM_INTERCORE_LOCAL], &cper_pldm_transfer_req);

        if (FPFW_STATUS_FAILED(status))
        {
            cper_transfer_reroute_ongoing = false;
        }
    }
}

void hm_cper_transfer_listener_from_secondary_mcp(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting CPER transfer request from secondary MCP\n");

    static uint8_t hm_cper_transfer_pldm_req_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t hm_cper_transfer_pldm_recv = {0};

    hm_cper_transfer_pldm_recv.recv_cmd_code = ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP;
    hm_cper_transfer_pldm_recv.payload_buffer = hm_cper_transfer_pldm_req_payload;
    hm_cper_transfer_pldm_recv.buffer_size = sizeof(hm_cper_transfer_pldm_req_payload);
    hm_cper_transfer_pldm_recv.cb = hm_mcp_cper_transfer_d2d_listener_cb;
    hm_cper_transfer_pldm_recv.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_cper_transfer_pldm_recv);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}

// Set up CPER transfer listener from SCP
void hm_cper_transfer_listener_from_scp(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting CPER transfer request from SCP\n");

    static uint8_t hm_cper_transfer_req_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t hm_cper_transfer_recv = {0};

    hm_cper_transfer_recv.recv_cmd_code = ICC_HM_CPER_TRANSFER_REQ_MCP;
    hm_cper_transfer_recv.payload_buffer = hm_cper_transfer_req_payload;
    hm_cper_transfer_recv.buffer_size = sizeof(hm_cper_transfer_req_payload);
    hm_cper_transfer_recv.cb = hm_mcp_cper_transfer_listener_from_scp_cb;
    hm_cper_transfer_recv.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_cper_transfer_recv);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}