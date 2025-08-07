//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_domain_apcore.c
 * Implements ap core error domain related functions.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <mscp_uefi_shared_ddrss.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void hm_apcore_error_injection_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("Error injection request from apcore get failed(%d)", (int)status);
    }
    else
    {
        hm_inject_error();
    }

    // restart listener
    hm_config_t* hm_config = get_hm_config();
    hm_apcore_error_injection_listener(hm_config->icc_ctx[HM_INTERCORE_APCORE]);
}

void hm_apcore_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t hm_icc_apcore_error_injection_payload[ICC_MHU_DDR_PAYLOAD_SIZE];
    static fpfw_icc_base_recv_req_t hm_icc_apcore_error_injection_recv_req = {0};

    hm_icc_apcore_error_injection_recv_req.recv_cmd_code = ICC_HM_ERROR_INJECTION_AP2SCP;
    hm_icc_apcore_error_injection_recv_req.payload_buffer = hm_icc_apcore_error_injection_payload;
    hm_icc_apcore_error_injection_recv_req.buffer_size = sizeof(hm_icc_apcore_error_injection_payload);
    hm_icc_apcore_error_injection_recv_req.cb = hm_apcore_error_injection_listener_cb;
    hm_icc_apcore_error_injection_recv_req.cb_ctx = hm_icc_apcore_error_injection_payload;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_apcore_error_injection_recv_req);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, FPFW_ICC_BASE_STATUS_SUCCESS);
}

void hm_prepare_ap_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    hm_apcore_error_injection_listener(icc_ctx);
}