//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_domain_mcp.c
 * Implements mcp error domain related functions.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <health_monitor.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <mscp_exp_rmss_memory_map.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void hm_mcp_error_injection_req_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static acpi_einj_cmd_status_t hm_mcp_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    acpi_einj_cmd_status_t result = ACPI_EINJ_SUCCESS;

    static hm_mhu_error_injection_payload_t err_injection_payload = {0};
    err_injection_payload.header.msg_header.command = ICC_HM_ERROR_INJECTION_MCP;
    err_injection_payload.header.msg_header.payload_size =
        sizeof(hm_mhu_error_injection_payload_t) - sizeof(icc_mhu_header_t);
    memcpy(&err_injection_payload.error_injection_info, einj_payload, sizeof(ras_einj_info_t));

    static fpfw_icc_base_send_req_t hm_icc_mcp_err_injection_req = {0};
    hm_icc_mcp_err_injection_req.payload_buffer = &err_injection_payload;
    hm_icc_mcp_err_injection_req.buffer_size = sizeof(hm_mhu_error_injection_payload_t);
    hm_icc_mcp_err_injection_req.cb = hm_mcp_error_injection_req_cb;
    hm_icc_mcp_err_injection_req.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_send(get_hm_config()->icc_ctx[HM_INTERCORE_MCP], &hm_icc_mcp_err_injection_req);

    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("MCP error injection request failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_MCP_ICC_TRANSFER, status);
        result = ACPI_EINJ_UNKNOWN_FAILURE;
    }
    else
    {
        HM_LOG_INFO("MCP error injection request was made");
    }

    return result;
}

static void hm_mcp_error_domain_register_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("MCP registration failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_MCP_REGISTRATION, status);
        return;
    }

    hm_mhu_error_domain_register_payload_t* hm_err_register_payload = (hm_mhu_error_domain_register_payload_t*)context;

    if (hm_err_register_payload == NULL || hm_err_register_payload->header.msg_header.command != ICC_HM_ERROR_DOMAIN_REGISTER_MCP)
    {
        HM_LOG_CRIT("Invalid MCP registration payload(%p)", hm_err_register_payload);
        HM_ET_ERROR(HM_ET_TYPE_MCP_REGISTRATION);
        return;
    }

    hm_register_error_domain(hm_err_register_payload->error_domain_idx,
                             hm_err_register_payload->valid_fru_id ? &hm_err_register_payload->fru_id : NULL,
                             hm_err_register_payload->valid_fru_text ? (char*)hm_err_register_payload->fru_text : NULL,
                             hm_mcp_error_injection_cb,
                             NULL);
}

static void hm_mcp_error_record_submit_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("MCP CPER ICC get failed(%d)", (int)status);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_MCP_CPER_ERROR, status);
        return;
    }

    hm_mhu_error_record_payload_t* hm_err_submit_payload = (hm_mhu_error_record_payload_t*)context;

    if (hm_err_submit_payload == NULL || hm_err_submit_payload->header.msg_header.command != ICC_HM_ERROR_RECORD_SUBMIT_MCP)
    {
        HM_LOG_CRIT("Invalid MCP CPER payload(%p)", hm_err_submit_payload);
        HM_ET_ERROR(HM_ET_TYPE_MCP_CPER_ERROR);
        return;
    }

    hm_submit_cper(hm_err_submit_payload->error_record.error_domain_idx,
                   hm_err_submit_payload->error_record.err_severity,
                   &hm_err_submit_payload->error_record.cper_section,
                   hm_err_submit_payload->error_record.section_size);

    hm_config_t* hm_config = get_hm_config();
    hm_mcp_error_record_submit_listener(hm_config->icc_ctx[HM_INTERCORE_MCP]);
}

bool hm_mcp_error_domain_register_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting MCP error domain registration");
    static uint8_t hm_icc_mcp_err_register_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t hm_icc_mcp_err_register_recv_req = {0};

    hm_icc_mcp_err_register_recv_req.recv_cmd_code = ICC_HM_ERROR_DOMAIN_REGISTER_MCP;
    hm_icc_mcp_err_register_recv_req.payload_buffer = hm_icc_mcp_err_register_payload;
    hm_icc_mcp_err_register_recv_req.buffer_size = sizeof(hm_icc_mcp_err_register_payload);
    hm_icc_mcp_err_register_recv_req.cb = hm_mcp_error_domain_register_listener_cb;
    hm_icc_mcp_err_register_recv_req.cb_ctx = hm_icc_mcp_err_register_payload;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_mcp_err_register_recv_req);
    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

bool hm_mcp_error_record_submit_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t hm_icc_mcp_err_submit_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t hm_icc_mcp_err_submit_req = {0};

    hm_icc_mcp_err_submit_req.recv_cmd_code = ICC_HM_ERROR_RECORD_SUBMIT_MCP;
    hm_icc_mcp_err_submit_req.payload_buffer = hm_icc_mcp_err_submit_payload;
    hm_icc_mcp_err_submit_req.buffer_size = sizeof(hm_icc_mcp_err_submit_payload);
    hm_icc_mcp_err_submit_req.cb = hm_mcp_error_record_submit_listener_cb;
    hm_icc_mcp_err_submit_req.cb_ctx = hm_icc_mcp_err_submit_payload;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_mcp_err_submit_req);
    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

void hm_prepare_mscp_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    hm_mcp_error_domain_register_listener(icc_ctx);
    hm_mcp_error_record_submit_listener(icc_ctx);
}
