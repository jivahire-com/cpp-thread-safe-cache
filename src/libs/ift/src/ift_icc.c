//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift_icc.c
 *  IFT ICC related API definitions
 */

/*------------------------- Includes ------------------------*/

#include "ift_i.h" // for ift_execute_mem_test, ift_execute_core_test

#include <DbgPrint.h>                       // for FPFW_DBGPRINT_INFO
#include <bug_check.h>                      // for BUG_ASSERT, BUG_ASSERT_PARAM
#include <fpfw_icc_base.h>                  // for fpfw_icc_base_send_recv_sync, fpfw_icc_base_ctx_t
#include <fpfw_init.h>                      // for fpfw_init_get_handle
#include <icc_platform_defines.h>           // for rmss_d2d_mailbox_ift_tx_msg
#include <idsw.h>                           // for idsw_get_die_id
#include <idsw_kng.h>                       // for DIE_1
#include <kingsgate_hsp_mailbox_commands.h> // for HSP_MAILBOX_CMD_IFT_INTENT_REQ, HSP_MAILBOX_CMD_IFT_RUN_STATUS_REQ
#include <silibs_status.h>                  // for SILIBS_SUCCESS

/*-------------------- Structure Definitions -----------------*/

/*------------------------- Typedefs -------------------------*/

/*--------------------- Static Declarations ------------------*/

static fpfw_icc_base_recv_req_t s_recv_params = {0};
static rmss_d2d_mailbox_ift_tx_msg s_ift_tx_msg = {0};

static fpfw_icc_base_send_recv_req_t s_ift_send_recv_param = {0};
static uint32_t ift_icc_msg_buf[HSP_MBOX_MAX_MESG_SIZE_BYTES / MBOX_WORD_SIZE_BYTE] = {0};

/*--------------------- Global Declarations ------------------*/

/*--------------------- Externs Declarations -----------------*/

extern fpfw_icc_base_ctx_t* g_hsp_icc_ctx;
extern uint32_t g_ift_result_offset;
extern silibs_status_t g_ift_execute_status;

/*----------------------- Enums & Defines --------------------*/

/*---------------------- Static Functions --------------------*/

static void ift_recv_d2d_fw_done_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);

    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
    BUG_ASSERT_PARAM(output_size_bytes >= sizeof(rmss_d2d_mailbox_msg_header), output_size_bytes, 0);

    ift_execute_test_dfwk();

    FPFW_DBGPRINT_INFO("IFT D2D FW download complete msg received from Die 0\n");
}

static void ift_icc_recv_status_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
    BUG_ASSERT_PARAM(output_size_bytes >= sizeof(struct kng_hsp_mailbox_msg_header), output_size_bytes, 0);

    idsw_die_id_t die_id = idsw_get_die_id();

    if (die_id == DIE_1)
    {
        ift_notify_scp_die0();
    }

    FPFW_DBGPRINT_INFO("IFT Test Status sent to HSP from Die %d\n", die_id);
}

static void ift_icc_send_test_status(fpfw_icc_base_ctx_t* hsp_icc_ctx)
{
    union kng_hsp_mailbox_ift_status_msg* ift_run_status_msg = (void*)ift_icc_msg_buf;

    s_ift_send_recv_param.payload_buffer = ift_run_status_msg;
    s_ift_send_recv_param.buffer_size = sizeof(*ift_run_status_msg);
    s_ift_send_recv_param.cb = ift_icc_recv_status_cb;
    s_ift_send_recv_param.cb_ctx = NULL;

    ift_run_status_msg->ift_status_req.header.cmd = HSP_MAILBOX_CMD_IFT_RUN_STATUS_REQ;
    ift_run_status_msg->ift_status_req.ift_run_status =
        g_ift_execute_status == SILIBS_SUCCESS ? IFT_TEST_STATUS_SUCCESS : IFT_TEST_STATUS_FAILURE;
    ift_run_status_msg->ift_status_req.scp_run_status = 0;

    fpfw_status_t fpfw_status = fpfw_icc_base_send_recv(hsp_icc_ctx, &s_ift_send_recv_param);

    BUG_ASSERT_PARAM(fpfw_status == FPFW_ICC_BASE_STATUS_SUCCESS, fpfw_status, 0);
}

static void ift_icc_recv_results_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
    BUG_ASSERT_PARAM(output_size_bytes >= sizeof(struct kng_hsp_mailbox_msg_header), output_size_bytes, 0);

    ift_icc_send_test_result_and_status(context);
}

static void ift_icc_send_test_result(fpfw_icc_base_ctx_t* hsp_icc_ctx, uint16_t ift_test_results_idx)
{
    union kng_hsp_mailbox_ift_ist_run_status_msg* ift_results_msg = (void*)ift_icc_msg_buf;
    uint32_t* ift_test_results = (void*)SCP_EXP_IFT_RESULT_BASE;

    s_ift_send_recv_param.payload_buffer = ift_results_msg;
    s_ift_send_recv_param.buffer_size = sizeof(*ift_results_msg);
    s_ift_send_recv_param.cb = ift_icc_recv_results_cb;
    s_ift_send_recv_param.cb_ctx = (void*)hsp_icc_ctx;

    ift_results_msg->ift_ist_run_status_req.header.cmd = HSP_MAILBOX_CMD_IFT_IST_RUN_STATUS_REQ;
    ift_results_msg->ift_ist_run_status_req.ift_ist_run_status = MMIO_READ32((ift_test_results + ift_test_results_idx));

    fpfw_status_t fpfw_status = fpfw_icc_base_send_recv(hsp_icc_ctx, &s_ift_send_recv_param);

    BUG_ASSERT_PARAM(fpfw_status == FPFW_ICC_BASE_STATUS_SUCCESS, fpfw_status, 0);
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

void ift_icc_register_d2d_fw_done()
{
    s_recv_params.recv_cmd_code = RMSS_D2D_MAILBOX_MSG_IFT_BIN_TRANSFER_DONE_REQ;
    s_recv_params.payload_buffer = &s_ift_tx_msg;
    s_recv_params.buffer_size = sizeof(s_ift_tx_msg);
    s_recv_params.cb = ift_recv_d2d_fw_done_cb;
    s_recv_params.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_recv(fpfw_init_get_handle("icc_d2dmbx"), &s_recv_params);

    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, 0);
}

void ift_icc_send_test_result_and_status(fpfw_icc_base_ctx_t* hsp_icc_ctx)
{
    static uint16_t s_ift_test_results_idx = 0;

    /**
     * Send test results if any are available. The test results are stored in SCP EXP RAM starting at
     * SCP_EXP_IFT_RESULT_BASE.
     *
     * HSP can send only one word (32-bits) result at a time to BMC. So we have to send multiple ICC
     * messages to transfer all test results to BMC. HSP uses SEL logs to transfer test results to BMC.
     *
     * The total number of 32-bit word test results is indicated by g_ift_result_offset.
     * The index s_ift_test_results_idx keeps track of which test result to send next.
     */
    if (s_ift_test_results_idx < g_ift_result_offset)
    {
        // Send the test results for the current index
        ift_icc_send_test_result(hsp_icc_ctx, s_ift_test_results_idx++);
    }
    /* All test results sent, send the overall test status */
    else
    {
        s_ift_test_results_idx = 0;
        ift_icc_send_test_status(hsp_icc_ctx);
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
    BUG_ASSERT_PARAM(ift_icc_msg.ift_intent_rsp.header.cmd == HSP_MAILBOX_CMD_IFT_INTENT_RSP,
                     ift_icc_msg.ift_intent_rsp.header.cmd,
                     0);

    *ift_enabled = ift_icc_msg.ift_intent_rsp.ift_enabled;
    *ift_intent_type = ift_icc_msg.ift_intent_rsp.ift_intent_type;
}
