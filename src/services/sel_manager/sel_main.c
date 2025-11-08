//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_main.c
 * Implements the primary service interface of sel manager.
 */

/*------------- Includes -----------------*/
#include "sel_i.h"

#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <FpFwLock.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <icc_mhu.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <platform_management_component/pldm_oem_event_types.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif
#include <sel.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_SEL_QUEUE_LENGTH 32

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sel_event_record_t g_sel_events[MAX_SEL_QUEUE_LENGTH] = {};
static int g_sel_queue_head = 0;
static int g_sel_queue_tail = 0;
static int g_sel_queue_size = 0;
static FPFW_LOCK g_sel_queue_lock;
// static bool g_sel_transferring = false; // For MCP0, SEL is transferring to BMC, but ICC transfer for other cores.

/*------------- Functions ----------------*/
/**
 * @brief Initialize SEL queue
 */
void sel_init_queue()
{
    FpFwLockInitialize(&g_sel_queue_lock);
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&g_sel_queue_lock);
    g_sel_queue_head = 0;
    g_sel_queue_tail = 0;
    g_sel_queue_size = 0;
    memset(g_sel_events, 0, sizeof(g_sel_events));
    FpFwLockRelease(&g_sel_queue_lock, lock_state);
}

/**
 * @brief Push SEL event record into internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if overflow.
 */
bool sel_push(sel_event_record_t* event_record)
{
    bool ret = true;
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&g_sel_queue_lock);
    g_sel_events[g_sel_queue_tail] = *event_record;
    g_sel_queue_tail = (g_sel_queue_tail + 1) % MAX_SEL_QUEUE_LENGTH;

    if (g_sel_queue_size < MAX_SEL_QUEUE_LENGTH)
    {
        g_sel_queue_size++;
        FPFW_DBGPRINT_INFO("SEL pushed size=%d\n", g_sel_queue_size);
    }
    else
    {
        // Drop the oldest SEL
        g_sel_queue_head = (g_sel_queue_head + 1) % MAX_SEL_QUEUE_LENGTH;
        ret = false;
        FPFW_DBGPRINT_WARNING("SEL queue full, drop the oldest SEL size=%d\n", g_sel_queue_size);
    }
    FpFwLockRelease(&g_sel_queue_lock, lock_state);

    return ret;
}

/**
 * @brief Push SEL event record to the head of internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if overflow.
 */
bool sel_push_head(sel_event_record_t* event_record)
{
    bool ret = true;
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&g_sel_queue_lock);

    if (g_sel_queue_size >= MAX_SEL_QUEUE_LENGTH)
    {
        FPFW_DBGPRINT_ERROR("SEL queue full, can not push to head\n");
        ret = false;
    }
    else
    {
        g_sel_queue_head = (g_sel_queue_head - 1 + MAX_SEL_QUEUE_LENGTH) % MAX_SEL_QUEUE_LENGTH;
        g_sel_events[g_sel_queue_head] = *event_record;
        g_sel_queue_size++;
        FPFW_DBGPRINT_INFO("SEL pushed head size=%d\n", g_sel_queue_size);
    }

    FpFwLockRelease(&g_sel_queue_lock, lock_state);

    return ret;
}

/**
 * @brief Pop SEL event record from internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if queue is empty.
 */
bool sel_pop(sel_event_record_t* event_record)
{
    FPFW_LOCK_STATE lock_state = FpFwLockAcquire(&g_sel_queue_lock);
    if (g_sel_queue_size == 0)
    {
        FPFW_DBGPRINT_WARNING("SEL queue empty\n");
        // Queue is empty
        FpFwLockRelease(&g_sel_queue_lock, lock_state);
        return false;
    }
    *event_record = g_sel_events[g_sel_queue_head];
    g_sel_queue_head = (g_sel_queue_head + 1) % MAX_SEL_QUEUE_LENGTH;
    g_sel_queue_size--;
    FPFW_DBGPRINT_INFO("SEL popped size=%d\n", g_sel_queue_size);

    FpFwLockRelease(&g_sel_queue_lock, lock_state);

    return true;
}

/*------------ ICC SEL Send --------------*/
static void icc_transfer_send_complete_cb(void* ctx, fpfw_status_t status)
{
    psel_svc_request_t request = (psel_svc_request_t)ctx;
    request->status = status;

    // Complete the DFWK async request when ICC transfer is complete.
    DfwkAsyncRequestComplete(&request->Header);
}

static fpfw_status_t sel_icc_send_event_async(fpfw_icc_base_ctx_t* icc_ctx, sel_event_record_t* event_record, PDFWK_ASYNC_REQUEST_HEADER request)
{
    static icc_mhu_sel_payload_t transfer_msg = {0};
    transfer_msg.header.msg_header.command = ICC_SEL_TRANSFER_TO_BMC;
    transfer_msg.header.msg_header.payload_size = sizeof(icc_mhu_sel_payload_t);
    transfer_msg.record = *event_record;

    static fpfw_icc_base_send_req_t icc_msg_ready_msg_req = {.payload_buffer = &transfer_msg,
                                                             .buffer_size = sizeof(transfer_msg),
                                                             .cb = icc_transfer_send_complete_cb};
    icc_msg_ready_msg_req.cb_ctx = request;

    return fpfw_icc_base_send(icc_ctx, &icc_msg_ready_msg_req);
}

/*------------ ICC SEL Receive ------------*/
static void icc_recv_sel_log_req_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    icc_mhu_sel_ctx_t* ctx = (icc_mhu_sel_ctx_t*)context;

    if (status == FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        BUG_ASSERT_PARAM(output_size_bytes >= sizeof(sel_event_record_t), output_size_bytes, 0);
        BUG_ASSERT_PARAM(ctx->payload.header.msg_header.command == ICC_SEL_TRANSFER_TO_BMC,
                         ctx->payload.header.msg_header.command,
                         0);

        // Received SEL event record from other core via ICC.
        // Push to the queue.
        if (!sel_push(&ctx->payload.record))
        {
            FPFW_DBGPRINT_WARNING("SEL queue full, drop the oldest SEL\n");
        }

        int status = sel_flush_queue();
        if (KNG_FAILED(status))
        {
            FPFW_DBGPRINT_ERROR("SEL flush_queue failed 0x%08lx\n", status);
        }
    }
    else
    {
        FPFW_DBGPRINT_ERROR("SEL received 0x%08lx\n", status);
    }

    // Register transfer callback for this ICC context again.
    fpfw_status_t reg_status = icc_register_mhu_sel_transfer_callback(ctx);
    BUG_ASSERT_PARAM(reg_status == FPFW_ICC_BASE_STATUS_SUCCESS, reg_status, 0);
}

fpfw_status_t icc_register_mhu_sel_transfer_callback(icc_mhu_sel_ctx_t* ctx)
{
    memset(&ctx->payload, 0, sizeof(ctx->payload));
    ctx->transfer_recv_req.payload_buffer = &ctx->payload;
    ctx->transfer_recv_req.buffer_size = MAX_MHU_PAYLOAD_SIZE;
    ctx->transfer_recv_req.recv_cmd_code = ICC_SEL_TRANSFER_TO_BMC;
    ctx->transfer_recv_req.cb = icc_recv_sel_log_req_cb;
    ctx->transfer_recv_req.cb_ctx = ctx;

    return fpfw_icc_base_recv(ctx->icc_ctx, &ctx->transfer_recv_req);
}

/*------------- PLDM Send ----------------*/
static void sel_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* ctx)
{
    psel_svc_request_t request = (psel_svc_request_t)ctx;

    request->status = (completionCode == FPFW_PLDM_CC_SUCCESS) ? KNG_SUCCESS : KNG_E_FAIL;
    if (request->status != KNG_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("SEL report to BMC failed: PLDM CC = 0x%x\n", completionCode);
    }

    // Complete the DFWK async request when PLDM transfer is complete.
    DfwkAsyncRequestComplete(&request->Header);
}

static KNG_STATUS sel_transfer_event_to_bmc(sel_event_record_t* event_record, PDFWK_ASYNC_REQUEST_HEADER request)
{
    KNG_STATUS status = KNG_SUCCESS;
    static fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = FPFW_PLDM_OEM_EVENT_TYPE_SEL,
                                                              .event_payload_size = sizeof(sel_event_record_t)};
    descriptor.event_payload = event_record;

    static pldm_platform_event_config_t event = {.p_descriptor = &descriptor};
    static pldm_platform_event_notification notification = {.CallBack = sel_pldm_on_ppe_complete};
    notification.context = request;

#ifdef PLDM_DRV_WORKAROUND
    static pldm_request_t pldm_request = {0};

    status = pldm_drv_raise_platform_event(&pldm_request, &event, &notification);
#else
    status = fpfw_pldm_service_raise_platform_event(&event, &notification);
#endif

    if (FPFW_STATUS_FAILED(status))
    {
        psel_svc_request_t sel_request = (psel_svc_request_t)request;
        sel_request->status = status;
        FPFW_DBGPRINT_ERROR("SEL report to BMC failed: status = 0x%08lx\n", status);
        DfwkAsyncRequestComplete(request);
    }

    return status;
}

/*------------- Log SEL ------------------*/
/**
 * @brief Log SEL event record
 *
 * @param event_record SEL event record.
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS log_sel_event(sel_event_record_t* event_record)
{
    // Push event record into internal queue
    if (!sel_push(event_record))
    {
        FPFW_DBGPRINT_WARNING("SEL queue full, drop the oldest SEL\n");
    }

    // Try to flush the queue
    return sel_flush_queue();
}

/**
 * @brief Send single SEL event record asynchronously
 *
 * @param event_record SEL event record
 * @param request SEL async request header
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_send_event_async(sel_event_record_t* event_record, PDFWK_ASYNC_REQUEST_HEADER request)
{
    fpfw_icc_base_ctx_t* icc_ctx = NULL;
    KNG_STATUS status = KNG_SUCCESS;

    if (event_record == NULL || request == NULL)
    {
        return KNG_E_INVALIDARG;
    }

    if (idsw_get_die_id() == DIE_0) // DIE 0
    {
        if (idsw_get_cpu_type() == CPU_MCP)
        {
            // Transfer event to the BMC
            FPFW_DBGPRINT_INFO("SEL: MCP0 starts SEL transfer\n");
            status = sel_transfer_event_to_bmc(event_record, request);
        }
        else
        {
            // SCP0 need to relay this request to the MCP0
            FPFW_DBGPRINT_INFO("SEL: SCP0 request to MCP0\n");
            icc_ctx = sel_icc_ctx(SEL_ICC_MSCP2MSCP);
        }
    }
    else // DIE 1
    {
        if (idsw_get_cpu_type() == CPU_MCP)
        {
            // MCP1 need to relay this request to the MCP0
            FPFW_DBGPRINT_INFO("SEL: MCP1 request to MCP0\n");
            icc_ctx = sel_icc_ctx(SEL_ICC_DIE2DIE);
        }
        else
        {
            // SCP1 need to relay this request to the SCP0
            FPFW_DBGPRINT_INFO("SEL: SCP1 request to SCP0\n");
            icc_ctx = sel_icc_ctx(SEL_ICC_DIE2DIE);
        }
    }

    if (icc_ctx != NULL)
    {
        status = sel_icc_send_event_async(icc_ctx, event_record, request);
    }

    return status;
}