//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_domain_hsp.c
 * Implements hsp error domain related functions.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <health_monitor.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hsp_firmware_headers.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <mscp_exp_rmss_memory_map.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef union _kkng_hsp_ras_error_inject_mailbox_msg
{
    struct kng_hsp_mailbox_cmd_ras_error_register_req hsp_err_register_req;
    struct kng_hsp_mailbox_cmd_ras_error_report_req hsp_err_report_req;
    struct kng_hsp_mailbox_cmd_ras_error_inject_req hsp_error_injection_req;
    struct kng_hsp_mailbox_msg_rsp rsp;
    uint32_t as_uint32[4];
} kng_hsp_ras_error_inject_mailbox_msg;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void hm_hsp_error_injection_req_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    // revisit design for error injection callback, check error injection payload needs to be updated
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);
}

static acpi_einj_cmd_status_t hm_hsp_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(einj_payload);

    acpi_einj_cmd_status_t result = ACPI_EINJ_SUCCESS;

    // copy into hsp payload buffer
    hm_config_t* hm_config = get_hm_config();
    volatile uint8_t* hsp_ras_einj_payload =
        (volatile uint8_t*)(uintptr_t)hm_config->mscp_hsp_ras_payload_base + HM_HSP_ERROR_INJECTION_OFFSET;

    for (uint32_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        hsp_ras_einj_payload[i] = ((uint8_t*)einj_payload)[i];
    }

    static kng_hsp_ras_error_inject_mailbox_msg hsp_error_injection = {0};
    hsp_error_injection.hsp_error_injection_req.header.cmd = HSP_MAILBOX_CMD_RAS_ERROR_INJECT_REQ;
    hsp_error_injection.hsp_error_injection_req.error_inject_payload_address_low = (uint32_t)(uintptr_t)hsp_ras_einj_payload;
    hsp_error_injection.hsp_error_injection_req.error_inject_payload_address_high = 0;

    static fpfw_icc_base_send_recv_req_t hsp_send_recv_params;
    hsp_send_recv_params.payload_buffer = &hsp_error_injection;
    hsp_send_recv_params.buffer_size = sizeof(hsp_error_injection);
    hsp_send_recv_params.cb = hm_hsp_error_injection_req_cb;
    hsp_send_recv_params.cb_ctx = &hsp_send_recv_params;

    fpfw_status_t status = fpfw_icc_base_send_recv(get_hm_config()->icc_ctx[HM_INTERCORE_HSP], &hsp_send_recv_params);

    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("HSP error injection request failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_HSP_ICC_TRANSFER, status);
        result = ACPI_EINJ_UNKNOWN_FAILURE;
    }
    else
    {
        HM_LOG_INFO("HSP error injection request was made");
    }

    return result;
}

static void hm_hsp_error_record_submit_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    fpfw_icc_base_recv_req_t* hm_icc_hsp_err_submit_recv_req = (fpfw_icc_base_recv_req_t*)context;

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("HSP CPER ICC get failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_HSP_CPER_ERROR, status);
        return;
    }

    struct kng_hsp_mailbox_cmd_ras_error_report_req* ras_error_report_req =
        (struct kng_hsp_mailbox_cmd_ras_error_report_req*)hm_icc_hsp_err_submit_recv_req->payload_buffer;
    hm_error_record_t* hm_err_submit_payload = (hm_error_record_t*)ras_error_report_req->cper_payload_address_low;

    // Check if the payload pointer is valid first
    if (hm_err_submit_payload == NULL)
    {
        HM_LOG_CRIT("HSP CPER NULL");
        HM_ET_ERROR(HM_ET_TYPE_HSP_CPER_ERROR);
        return;
    }

    // Check if the payload is within valid memory range
    hm_config_t* hm_config = get_hm_config();

    uint32_t hsp_payload_end_addr =
        (uint32_t)(uintptr_t)hm_config->mscp_hsp_ras_payload_base + SCP_EXP_HSP_RAS_PAYLOAD_SIZE - 1;
    uint32_t cper_end_addr = (uintptr_t)hm_err_submit_payload + offsetof(hm_error_record_t, cper_section) +
                             hm_err_submit_payload->section_size;

    if (hsp_payload_end_addr < cper_end_addr)
    {
        HM_LOG_CRIT("Invalid HSP CPER payload(%p - %p)", hm_err_submit_payload, (void*)cper_end_addr);
        HM_ET_ERROR(HM_ET_TYPE_HSP_CPER_ERROR);
        return;
    }

    hm_submit_cper(hm_err_submit_payload->error_domain_idx,
                   hm_err_submit_payload->err_severity,
                   &hm_err_submit_payload->cper_section,
                   sizeof(acpi_err_sec_hsp_processor_t));

    // HSP request, clear payload after consuming it
    volatile uint8_t* hsp_cper_section_base =
        (volatile uint8_t*)(hm_config->mscp_hsp_ras_payload_base + HM_HSP_ERROR_RECORD_OFFSET);

    for (uint32_t i = 0; i < (SCP_EXP_HSP_RAS_PAYLOAD_SIZE - HM_HSP_ERROR_RECORD_OFFSET); ++i)
    {
        hsp_cper_section_base[i] = 0;
    }

    hm_hsp_error_record_submit_listener(hm_config->icc_ctx[HM_INTERCORE_HSP]);
}

static void hm_hsp_error_domain_register_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    fpfw_icc_base_recv_req_t* hm_icc_hsp_err_register_recv_req = (fpfw_icc_base_recv_req_t*)context;

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("HSP registration failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_ED_HSP_REGISTRATION, status);
        return;
    }

    struct kng_hsp_mailbox_cmd_ras_error_register_req* ras_error_register_req =
        (struct kng_hsp_mailbox_cmd_ras_error_register_req*)hm_icc_hsp_err_register_recv_req->payload_buffer;

    hm_error_domain_info_t* hm_err_register_payload =
        (hm_error_domain_info_t*)ras_error_register_req->error_domain_payload_address_low;

    if (hm_err_register_payload == NULL || (uint8_t*)hm_err_register_payload != get_hm_config()->mscp_hsp_ras_payload_base)
    {
        HM_LOG_CRIT("Invalid HSP registration payload(%p), expected(%p)",
                    hm_err_register_payload,
                    (void*)(SCP_EXP_HSP_RAS_PAYLOAD_BASE + HM_HSP_ERROR_REGISTRATION_OFFSET));
        HM_ET_ERROR(HM_ET_TYPE_ED_HSP_REGISTRATION);
        return;
    }

    hm_register_error_domain(hm_err_register_payload->error_domain_idx,
                             hm_err_register_payload->valid_fru_id ? &hm_err_register_payload->fru_id : NULL,
                             hm_err_register_payload->valid_fru_str ? (char*)hm_err_register_payload->fru_text : NULL,
                             hm_hsp_error_injection_cb,
                             NULL);
}

void hm_hsp_error_domain_register_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting HSP error domain registration");
    static kng_hsp_ras_error_inject_mailbox_msg hm_icc_hsp_err_register_payload;
    static fpfw_icc_base_recv_req_t hm_icc_hsp_err_register_req;

    memset(&hm_icc_hsp_err_register_payload, 0, sizeof(hm_icc_hsp_err_register_payload));

    hm_icc_hsp_err_register_req.recv_cmd_code = HSP_MAILBOX_CMD_RAS_ERROR_REGISTER_REQ;
    hm_icc_hsp_err_register_req.payload_buffer = &hm_icc_hsp_err_register_payload;
    hm_icc_hsp_err_register_req.buffer_size = sizeof(hm_icc_hsp_err_register_payload);
    hm_icc_hsp_err_register_req.cb = hm_hsp_error_domain_register_listener_cb;
    hm_icc_hsp_err_register_req.cb_ctx = &hm_icc_hsp_err_register_req;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_hsp_err_register_req);
    HM_ET_INFO_PARAM(HM_ET_TYPE_HSP_ICC_TRANSFER, status);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}

void hm_hsp_error_record_submit_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    static kng_hsp_ras_error_inject_mailbox_msg hm_icc_hsp_err_submit_payload;
    static fpfw_icc_base_recv_req_t hm_icc_hsp_err_submit_recv_req;

    memset(&hm_icc_hsp_err_submit_payload, 0, sizeof(hm_icc_hsp_err_submit_payload));

    hm_icc_hsp_err_submit_recv_req.recv_cmd_code = HSP_MAILBOX_CMD_RAS_ERROR_REPORT_REQ;
    hm_icc_hsp_err_submit_recv_req.payload_buffer = &hm_icc_hsp_err_submit_payload;
    hm_icc_hsp_err_submit_recv_req.buffer_size = sizeof(hm_icc_hsp_err_submit_payload);
    hm_icc_hsp_err_submit_recv_req.cb = hm_hsp_error_record_submit_listener_cb;
    hm_icc_hsp_err_submit_recv_req.cb_ctx = &hm_icc_hsp_err_submit_recv_req;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_hsp_err_submit_recv_req);
    HM_ET_INFO_PARAM(HM_ET_TYPE_HSP_ICC_TRANSFER, status);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}

void hm_prepare_hsp_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    hm_hsp_error_domain_register_listener(icc_ctx);
    hm_hsp_error_record_submit_listener(icc_ctx);
}