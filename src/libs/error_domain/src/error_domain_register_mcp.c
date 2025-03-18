//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_register_mcp.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cper.h>
#include <fpfw_icc_base.h>
#include <health_monitor_icc.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MCP_PROC_FRU "MCP_PROC"
#define MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0x339505B3, 0x649B, 0x4F5F,                        \
        {                                                  \
            0xC1, 0x97, 0x42, 0x51, 0xD9, 0x5D, 0x65, 0xDB \
        }                                                  \
    }
/*-------- Function Prototypes -----------*/
void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* mhu_handle = NULL;

/*-------------- Functions ---------------*/
static void register_mcp_error_domain_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

void register_mcp_error_domain(fpfw_icc_base_ctx_t* icc_ctx)
{
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);
    mhu_handle = icc_ctx;

    static hm_mhu_error_domain_register_payload_t mcp_err_domain_register_payload = {0};
    mcp_err_domain_register_payload.header.msg_header.command = ICC_HM_ERROR_DOMAIN_REGISTER_MCP;
    mcp_err_domain_register_payload.header.msg_header.payload_size =
        sizeof(hm_mhu_error_domain_register_payload_t) - sizeof(icc_mhu_header_t);
    mcp_err_domain_register_payload.error_domain_idx = ACPI_ERROR_DOMAIN_MCP_PROC;
    mcp_err_domain_register_payload.valid_fru_id = 1;
    mcp_err_domain_register_payload.valid_fru_text = 1;
    mcp_err_domain_register_payload.fru_id = (guid_t)MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
    strncpy(mcp_err_domain_register_payload.fru_text, MCP_PROC_FRU, sizeof(mcp_err_domain_register_payload.fru_text) - 1);
    mcp_err_domain_register_payload.fru_text[sizeof(mcp_err_domain_register_payload.fru_text) - 1] = '\0';

    static fpfw_icc_base_send_req_t mcp_err_domain_register_req = {0};
    mcp_err_domain_register_req.payload_buffer = &mcp_err_domain_register_payload;
    mcp_err_domain_register_req.buffer_size = sizeof(mcp_err_domain_register_payload);
    mcp_err_domain_register_req.cb = register_mcp_error_domain_complete_cb;
    mcp_err_domain_register_req.cb_ctx = &mcp_err_domain_register_payload;

    fpfw_status_t status = fpfw_icc_base_send(icc_ctx, &mcp_err_domain_register_req);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}

static void mcp_error_injection_request_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    if (status == FPFW_STATUS_SUCCESS)
    {
        hm_mhu_error_injection_payload_t* hm_err_injection_payload = (hm_mhu_error_injection_payload_t*)context;

        if (hm_err_injection_payload != NULL && hm_err_injection_payload->header.msg_header.command == ICC_HM_ERROR_INJECTION_MCP)
        {
            // Placeholder for MCP error injection handling
        }
    }

    start_mcp_error_injection_listener(mhu_handle);
}

void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

    static uint8_t mcp_err_domain_injection_payload[SCP_EXP_ICC_MHU_SCP_MCP_LOCAL_RECEIVE_SIZE];
    static fpfw_icc_base_recv_req_t mcp_err_injection_recv_req = {0};

    mcp_err_injection_recv_req.recv_cmd_code = ICC_HM_ERROR_INJECTION_MCP;
    mcp_err_injection_recv_req.payload_buffer = mcp_err_domain_injection_payload;
    mcp_err_injection_recv_req.buffer_size = sizeof(mcp_err_domain_injection_payload);
    mcp_err_injection_recv_req.cb = mcp_error_injection_request_cb;
    mcp_err_injection_recv_req.cb_ctx = mcp_err_domain_injection_payload;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &mcp_err_injection_recv_req);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}