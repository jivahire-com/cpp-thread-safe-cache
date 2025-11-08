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
#include <crash_dump.h>
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <fpfw_status.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <icc_platform_defines.h>
#include <libpldm/platform.h>
#include <mscp_exp_rmss_memory_map.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct _pldm_cper_record_t
{
    struct pldm_platform_cper_event cper_event_header;
    acpi_cper_record_t cper_record;
} pldm_cper_record_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static volatile bool pldm_cper_transfer_ongoing = false;
static volatile bool cper_transfer_reroute_ongoing = false;
static volatile bool pldm_stack_ready = false;
static pldm_cper_record_t full_pldm_cper;

/*------------- Functions ----------------*/
static void hm_transfer_cper_completion_pldm_cb(fpfw_pldm_cc_t completionCode, void* ctx)
{
    HM_LOG_INFO("CPER OOB transfer completed, status (%d)", completionCode);
    pldm_cper_record_t* rec = (pldm_cper_record_t*)ctx;
    BUG_ASSERT_PARAM(rec != NULL, rec, &full_pldm_cper);

    hm_config_t* cfg = get_hm_config();
    BUG_ASSERT_PARAM(cfg != NULL, cfg, 0);

    wait_for_semaphore(cfg->semaphore_id, cfg->semaphore_key);
    hm_set_pldm_transfer_status(HM_PLDM_TRANSFER_STATUS_IDLE,
                                hm_is_fatal_error(rec->cper_record.record_header.error_severity));
    release_semaphore(cfg->semaphore_id);

    pldm_cper_transfer_ongoing = false;
}

static void hm_cper_transfer_reroute_mcpd2d_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);

    cper_transfer_reroute_ongoing = false;
}

static void hm_mcp_cper_transfer_listener_mcpd2d_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    hm_transfer_cper_to_bmc_internal(false);

    hm_config_t* hm_config = get_hm_config();
    hm_cper_transfer_listener_from_secondary_mcp(hm_config->icc_ctx[HM_INTERCORE_LOCAL]);
}

static void hm_mcp_cper_transfer_listener_from_scp_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(status);
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    hm_transfer_cper_mcp2bmc();

    hm_config_t* hm_config = get_hm_config();
    hm_cper_transfer_listener_from_scp(hm_config->icc_ctx[HM_INTERCORE_REMOTE]);
}

static bool hm_get_pldm_cper_data(bool is_ue, uint8_t* cper_record_base)
{
    hm_config_t* cfg = get_hm_config();
    BUG_ASSERT_PARAM(cfg != NULL, cfg, false);

    volatile hm_arsm_cper_backup_t* backup = (volatile hm_arsm_cper_backup_t*)(uintptr_t)cfg->mscp_full_cper_record_base;

    if (hm_get_pldm_transfer_status(is_ue) != HM_PLDM_TRANSFER_STATUS_REQUESTED)
    {
        return false;
    }

    wait_for_semaphore(cfg->semaphore_id, cfg->semaphore_key);
    hm_copy_cper_record((volatile uint8_t*)cper_record_base,
                        (const uint8_t*)(is_ue ? &backup->last_ue_cper_record.cper_record
                                               : &backup->last_cper_record.cper_record),
                        sizeof(acpi_cper_record_t));
    release_semaphore(cfg->semaphore_id);
    return true;
}

void hm_transfer_cper_to_bmc_internal(bool is_ue)
{
    hm_flush_GHES_until_OS_boot();

    if (get_hm_config()->is_primary == false)
    {
        HM_LOG_INFO("CPER OOB transfer request made on secondary MCP, aborting\n");
        return;
    }

    if (pldm_stack_ready == false)
    {
        HM_LOG_INFO("PLDM stack not ready, abort OOB transfer\n");
        return;
    }

    if (pldm_cper_transfer_ongoing)
    {
        HM_LOG_INFO("CPER OOB transfer request dropped, PLDM busy\n");
        return;
    }

    if (hm_get_pldm_cper_data(is_ue, (uint8_t*)&full_pldm_cper.cper_record))
    {
        pldm_cper_transfer_ongoing = true;
        full_pldm_cper.cper_event_header.format_version = PLDM_PLATFORM_EVENT_MESSAGE_FORMAT_VERSION;
        full_pldm_cper.cper_event_header.format_type = PLDM_PLATFORM_CPER_EVENT_WITH_HEADER;
        full_pldm_cper.cper_event_header.event_data_length = sizeof(acpi_cper_record_t);

        HM_LOG_INFO("CPER OOB transfer requested, (domain=%s, sev=%ld)\n",
                    full_pldm_cper.cper_record.section_descriptor[0].fru_text,
                    full_pldm_cper.cper_record.record_header.error_severity);

        static fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = PLDM_CPER_EVENT,
                                                                  .event_payload_size = sizeof(pldm_cper_record_t)};
        descriptor.event_payload = &full_pldm_cper;

        static pldm_platform_event_config_t event = {.p_descriptor = &descriptor};
        static pldm_platform_event_notification notification = {.CallBack = hm_transfer_cper_completion_pldm_cb,
                                                                .context = &full_pldm_cper};

#ifdef PLDM_DRV_WORKAROUND
        static pldm_request_t pldm_request = {0};

        fpfw_status_t status = pldm_drv_raise_platform_event(&pldm_request, &event, &notification);
#else
        fpfw_status_t status = fpfw_pldm_service_raise_platform_event(&event, &notification);
#endif

        if (FPFW_STATUS_FAILED(status))
        {
            HM_LOG_INFO("CPER OOB transfer request failed (status=0x%x)\n", status);
            pldm_cper_transfer_ongoing = false;
        }
    }
}

void hm_transfer_cper_mcp2bmc()
{
    if (get_hm_config()->is_mcp == false)
    {
        return;
    }

    if (get_hm_config()->is_primary)
    {
        hm_transfer_cper_to_bmc_internal(false);
    }
    else
    {
        HM_LOG_INFO("CPER OOB request on secondary mcp, re-route\n");

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
        cper_pldm_transfer_req.cb = hm_cper_transfer_reroute_mcpd2d_cb;
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
    static uint8_t hm_cper_transfer_pldm_req_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t hm_cper_transfer_pldm_recv = {0};

    hm_cper_transfer_pldm_recv.recv_cmd_code = ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP;
    hm_cper_transfer_pldm_recv.payload_buffer = hm_cper_transfer_pldm_req_payload;
    hm_cper_transfer_pldm_recv.buffer_size = sizeof(hm_cper_transfer_pldm_req_payload);
    hm_cper_transfer_pldm_recv.cb = hm_mcp_cper_transfer_listener_mcpd2d_cb;
    hm_cper_transfer_pldm_recv.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_cper_transfer_pldm_recv);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}

// Set up CPER transfer listener from SCP
void hm_cper_transfer_listener_from_scp(fpfw_icc_base_ctx_t* icc_ctx)
{
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

void hm_set_pldm_ready_status()
{
    pldm_stack_ready = true;

    if (hm_get_pldm_cper_data(true, (uint8_t*)&full_pldm_cper.cper_record))
    {
        HM_LOG_INFO("Pending UE CPER found, rescheduling transfer\n");
        // ToDo - when pldm scheduler available, delegate transfer
    }
    else
    {
        HM_LOG_INFO("UE CPER data not found\n");
    }
}