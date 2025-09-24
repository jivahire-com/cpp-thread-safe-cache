//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift_icc.c
 *  IFT ICC related API definitions
 */

/*------------------------- Includes ------------------------*/

#include "ift_i.h" // for ift_execute_mem_test, ift_execute_core_test

#include <bug_check.h>                      // for BUG_ASSERT, BUG_ASSERT_PARAM
#include <fpfw_icc_base.h>                  // for fpfw_icc_base_send_recv_sync, fpfw_icc_base_ctx_t
#include <fpfw_init.h>                      // for fpfw_init_get_handle
#include <icc_platform_defines.h>           // for rmss_d2d_mailbox_ift_tx_msg
#include <kingsgate_hsp_mailbox_commands.h> // for HSP_MAILBOX_CMD_IFT_INTENT_REQ, HSP_MAILBOX_CMD_IFT_RUN_STATUS_REQ

/*-------------------- Structure Definitions -----------------*/

/*------------------------- Typedefs -------------------------*/

/*--------------------- Static Declarations ------------------*/

static fpfw_icc_base_recv_req_t s_recv_params = {0};
static rmss_d2d_mailbox_ift_tx_msg s_ift_tx_msg = {0};

/*--------------------- Global Declarations ------------------*/

/*--------------------- Static Declarations ------------------*/

/*----------------------- Enums & Defines --------------------*/

/*---------------------- Static Functions --------------------*/

static void ift_recv_d2d_fw_done_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
    BUG_ASSERT_PARAM(output_size_bytes >= sizeof(rmss_d2d_mailbox_msg_header), output_size_bytes, 0);

    if (s_ift_tx_msg.ift_intent_type == IFT_MGR_IFT_INTENT_MEM_TEST)
    {
        /* Execute the IFT memory test */
        ift_execute_mem_test(context);
    }
    else if (s_ift_tx_msg.ift_intent_type == IFT_MGR_IFT_INTENT_CORE_TEST)
    {
        /* Execute the IFT core test */
        ift_execute_core_test(context);
    }
}

/*---------------------- Global Functions --------------------*/

void ift_icc_send_d2d_fw_done_msg()
{
    s_ift_tx_msg.header.cmd = RMSS_D2D_MAILBOX_MSG_IFT_BIN_TRANSFER_DONE_REQ;
    s_ift_tx_msg.ift_intent_type = ift_get_intent_type();
    s_ift_tx_msg.ift_fw_size = ift_get_current_fw_size();

    fpfw_status_t status =
        fpfw_icc_base_send_sync(fpfw_init_get_handle("icc_d2dmbx"), &s_ift_tx_msg, sizeof(s_ift_tx_msg));

    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
}

void ift_icc_register_d2d_fw_done_cb(void* ctx)
{
    s_recv_params.recv_cmd_code = RMSS_D2D_MAILBOX_MSG_IFT_BIN_TRANSFER_DONE_REQ;
    s_recv_params.payload_buffer = &s_ift_tx_msg;
    s_recv_params.buffer_size = sizeof(s_ift_tx_msg);
    s_recv_params.cb = ift_recv_d2d_fw_done_cb;
    s_recv_params.cb_ctx = ctx;

    fpfw_status_t status = fpfw_icc_base_recv(fpfw_init_get_handle("icc_d2dmbx"), &s_recv_params);

    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
}

void ift_icc_send_test_status(fpfw_icc_base_ctx_t* hsp_icc_ctx, uint32_t run_status)
{
    union kng_hsp_mailbox_ift_status_msg ift_run_status_msg = {0};
    size_t recv_msg_size_bytes = 0;

    ift_run_status_msg.ift_status_req.header.cmd = HSP_MAILBOX_CMD_IFT_RUN_STATUS_REQ;
    ift_run_status_msg.ift_status_req.ift_run_status = run_status;
    ift_run_status_msg.ift_status_req.scp_run_status = 0;

    fpfw_status_t fpfw_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &ift_run_status_msg, sizeof(ift_run_status_msg), &recv_msg_size_bytes);

    BUG_ASSERT_PARAM(fpfw_status == FPFW_ICC_BASE_STATUS_SUCCESS, fpfw_status, 0);
    BUG_ASSERT_PARAM(recv_msg_size_bytes >= sizeof(struct kng_hsp_mailbox_msg_header), recv_msg_size_bytes, 0);
    BUG_ASSERT_PARAM(ift_run_status_msg.ift_status_rsp.cmd == HSP_MAILBOX_CMD_IFT_RUN_STATUS_RSP,
                     ift_run_status_msg.ift_status_rsp.cmd,
                     0);
}

void ift_icc_send_test_results(fpfw_icc_base_ctx_t* hsp_icc_ctx, uint32_t fail_index)
{
    struct kng_hsp_mailbox_cmd_ift_ist_run_status_req ift_results_msg = {0};
    size_t recv_msg_size_bytes = 0;
    uint32_t* ift_test_results = (void*)SCP_EXP_IFT_RESULT_BASE;

    for (uint32_t i = 0; i < fail_index; i++)
    {
        ift_results_msg.header.cmd = HSP_MAILBOX_CMD_IFT_IST_RUN_STATUS_REQ;
        ift_results_msg.ift_ist_run_status = MMIO_READ32((ift_test_results + i));

        fpfw_status_t fpfw_status =
            fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &ift_results_msg, sizeof(ift_results_msg), &recv_msg_size_bytes);

        BUG_ASSERT_PARAM(fpfw_status == FPFW_ICC_BASE_STATUS_SUCCESS, fpfw_status, 0);
        BUG_ASSERT_PARAM(recv_msg_size_bytes >= sizeof(struct kng_hsp_mailbox_msg_header), recv_msg_size_bytes, 0);
        BUG_ASSERT_PARAM(ift_results_msg.header.cmd == HSP_MAILBOX_CMD_IFT_IST_RUN_STATUS_RSP,
                         ift_results_msg.header.cmd,
                         0);
    }
}

void ift_icc_get_ift_intent(fpfw_icc_base_ctx_t* hsp_icc_ctx, uint8_t* ift_enabled, uint32_t* ift_intent_type)
{
    union kng_hsp_mailbox_ift_intent_msg ift_icc_msg = {0};

    ift_icc_msg.ift_intent_req.cmd = HSP_MAILBOX_CMD_IFT_INTENT_REQ;

    size_t recv_msg_size_bytes = 0;
    fpfw_status_t fpfw_status =
        fpfw_icc_base_send_recv_sync(hsp_icc_ctx, &ift_icc_msg, sizeof(ift_icc_msg), &recv_msg_size_bytes);

    BUG_ASSERT_PARAM(fpfw_status == FPFW_ICC_BASE_STATUS_SUCCESS, fpfw_status, 0);
    BUG_ASSERT_PARAM(recv_msg_size_bytes >= sizeof(struct kng_hsp_mailbox_msg_header), recv_msg_size_bytes, 0);
    BUG_ASSERT_PARAM(ift_icc_msg.ift_intent_rsp.hdr.cmd == HSP_MAILBOX_CMD_IFT_INTENT_RSP,
                     ift_icc_msg.ift_intent_rsp.hdr.cmd,
                     0);

    *ift_enabled = ift_icc_msg.ift_intent_rsp.ift_enabled;
    *ift_intent_type = ift_icc_msg.ift_intent_rsp.ift_intent_type;
}