//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_core_comm.c
 * Implements inter-core communication for startup or shutdown driver
 */

/*------------- Includes -----------------*/
#include "startup_shutdown.h"
#include "startup_shutdown_i.h"
#include "startup_shutdown_init.h"
#include "system_info.h"

#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <hsp_firmware_headers.h>
#include <icc_platform_defines.h>
#include <kng_error.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_hspmbx_ctx = NULL;
static shutdown_icc_req_t shutdown_req = {0};
static FPFW_LOCK icc_lock; //! lock for protecting req_mem

/*------------- Functions ----------------*/

// Setup receive request for PrepareForCoreReset command from HSP
void receive_prep_core_reset()
{
    static kng_hsp_mailbox_msg hsp_recv_msg;

    static fpfw_icc_base_recv_req_t recv_params;
    memset(&hsp_recv_msg, 0, sizeof(hsp_recv_msg));
    recv_params.payload_buffer = &hsp_recv_msg;     // Buffer to receive the message
    recv_params.buffer_size = sizeof(hsp_recv_msg); // Size of the buffer
    recv_params.recv_cmd_code = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ;
    recv_params.cb = prepare_reset_recv_cb; // Set the callback function for asynchronous receive
    recv_params.cb_ctx = &recv_params;      // Pass the context to the callback function
    fpfw_status_t recv_status = fpfw_icc_base_recv(icc_hspmbx_ctx, &recv_params);
    //! Print the status message
    if (recv_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        SOS_LOG_INFO("[RESET REQ] Not registered to receive PrepareForCoreReset Cmd from HSP: Status[0x%x]\n",
                     (int)recv_status);
    }
    else
    {
        SOS_LOG_INFO("[RESET REQ] Registered to receive PrepareForCoreReset Cmd from HSP\n");
    }
}

// Callback function for the completion of the reset notification
void reset_complete_notify(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    if (status != DFWK_SUCCESS)
    {
        SOS_LOG_CRIT("ICC Send Error in reset_complete_notify: 0x%08x", (int)status);
    }
    else
    {
        SOS_LOG_INFO("Reset complete notification sent successfully.");
    }
}

// Callback function for the completion of the quiesce request
void quiesce_complete_notify(DFWK_ASYNC_REQUEST_HEADER* request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    SOS_LOG_INFO("Quiesce complete successfully.");

    // Send ACK back to HSP after quiesce
    fpfw_icc_base_recv_req_t* recv_params = (fpfw_icc_base_recv_req_t*)p_completion_context;
    static fpfw_icc_base_send_req_t send_params;
    memset(&send_params, 0, sizeof(send_params));
    send_params.payload_buffer = recv_params->payload_buffer; // Buffer containing the message to send
    send_params.buffer_size = HSP_MBOX_MAX_MESG_SIZE_BYTES;   // Size of the buffer
    recv_params->recv_entry.cmd_code = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_RSP; // Set the appropriate command code
    send_params.cb = reset_complete_notify;
    send_params.cb_ctx = &send_params; // Pass the context to the callback function

    SOS_LOG_INFO("Preparing to send RSP for PrepareForCoreReset\n");

    fpfw_status_t send_status = fpfw_icc_base_send(icc_hspmbx_ctx, &send_params);
    if (send_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        SOS_LOG_INFO("ICC Send Error in prepare_reset_recv_cb: 0x%08x", (int)send_status);
    }
    else
    {
        SOS_LOG_INFO("[RESET REQ] Registered to send PrepareForCoreReset Cmd ACK: Status[0x%x]\n", (int)send_status);
    }
}

// Callback function for the asynchronous PrepareForCoreReset receive operation
void prepare_reset_recv_cb(void* context, size_t cmd_code, fpfw_status_t status)
{
    FPFW_UNUSED(cmd_code);
    FPFW_UNUSED(context);
    fpfw_icc_base_recv_req_t* recv_context = (fpfw_icc_base_recv_req_t*)context;

    if (status != DFWK_SUCCESS)
    {
        SOS_LOG_CRIT("ICC Receive Error in prepare_reset_recv_cb: 0x%08x", (unsigned int)status);
    }
    else
    {
        static startup_shutdown_request_t shutdown_request;
        DfwkAsyncRequestInitialize((void*)&shutdown_request.header, sizeof(shutdown_request));
        sos_quiesce((void*)fpfw_init_get_handle("sos_int"), &shutdown_request, quiesce_complete_notify, recv_context);
    }
}

void shutdown_req_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(status);
    FPFW_UNUSED(context);

    SOS_LOG_INFO("[RESET REQ] In callback function - ICC HSP mbx send reset req\n");
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&icc_lock);
    shutdown_req.in_use = false;
    FpFwLockRelease(&icc_lock, oldState);
}

void sos_icc_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    FpFwLockInitialize(&icc_lock);
    icc_hspmbx_ctx = icc_ctx;
    // Setup receive request and register to receive and handle PrepareForCoreReset command from HSP
    receive_prep_core_reset();
}

KNG_STATUS sos_request_shutdown(ssi_shutdown_type_t type)
{
    BUG_ASSERT_PARAM(icc_hspmbx_ctx != NULL, icc_hspmbx_ctx, &shutdown_req);

    KNG_STATUS result = KNG_SUCCESS;
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&icc_lock);

    if (shutdown_req.in_use)
    {
        return KNG_E_BUSY;
    }
    shutdown_req.in_use = true;
    FpFwLockRelease(&icc_lock, oldState);

    memset(&shutdown_req.msg, 0, sizeof(shutdown_req.msg));
    memset(&shutdown_req.hsp_send_params, 0, sizeof(shutdown_req.hsp_send_params));

    shutdown_req.msg.header.cmd = HSP_MAILBOX_MSG_UNKNOWN;

    switch (type)
    {
    case SHUTDOWN:
    case SHUTDOWN_SCP_INITIATED:
        shutdown_req.msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_SHUTDOWN_REQ, 0, 0);
        break;
    case COLD_RESET:
    case COLD_RESET_SCP_INITIATED:
        shutdown_req.msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_COLD_RESET_REQ, 0, 0);
        break;
    case MSCP_SUBSYS_RESET:
        shutdown_req.msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_WARM_RESET_REQ, 0, 0);
        break;
    default:
        shutdown_req.in_use = false;
        result = KNG_E_NOTIMPL;
        break;
    }

    if (shutdown_req.msg.header.cmd != HSP_MAILBOX_MSG_UNKNOWN)
    {
        shutdown_req.hsp_send_params.payload_buffer = &shutdown_req.msg;
        shutdown_req.hsp_send_params.buffer_size = sizeof(shutdown_req.msg);
        shutdown_req.hsp_send_params.cb = shutdown_req_cb;
        shutdown_req.hsp_send_params.cb_ctx = &shutdown_req;

        fpfw_status_t status = fpfw_icc_base_send(icc_hspmbx_ctx, &shutdown_req.hsp_send_params);
        if (status != FPFW_STATUS_SUCCESS)
        {
            SOS_LOG_CRIT("ICC Error: 0x%08x", (int)status);
            result = KNG_E_FAIL;
            FPFW_LOCK_STATE oldState = FpFwLockAcquire(&icc_lock);
            shutdown_req.in_use = false;
            FpFwLockRelease(&icc_lock, oldState);
        }
    }

    return result;
}