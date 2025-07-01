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
#include <cmsis_m7.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <hsp_firmware_headers.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <kng_error.h>
#include <kng_icc_shared.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

// TODO: Add the below cmd code in silibs shared ICC header file

#define ICC_MODULE_STARTUP_SHUTDOWN (0x0009)

#define ICC_D2D_SHUTDOWN_REQUEST ICC_GEN_CMD(ICC_MODULE_STARTUP_SHUTDOWN, 0x1) // 0x0009_0001

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_hspmbx_ctx = NULL;
static fpfw_icc_base_ctx_t* icc_die2die_ctx = NULL;

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

// Hang the core after quiesce is complete
void reset_complete_wait_forever()
{
    SOS_LOG_INFO("Reset complete, waiting forever...");
    while (1)
    {
        __WFI(); // Wait for interrupt
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
        reset_complete_wait_forever();
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
    ((kng_hsp_mailbox_msg*)recv_params->payload_buffer)->header.cmd = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_RSP;
    send_params.payload_buffer = recv_params->payload_buffer; // Buffer containing the message to send
    send_params.buffer_size = HSP_MBOX_MAX_MESG_SIZE_BYTES;   // Size of the buffer
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

// Utility function to convert HSP reset reason to shutdown type
// ToDo: Need to review how this mapping should be.
static ssi_shutdown_type_t convert_HSP_reset_reason_to_shutdown_type(uint32_t reset_reason)
{
    switch (reset_reason)
    {
    case HSP_FIRMWARE_RESET_REASON_HSP_RECOVERY_MODE:
    case HSP_FIRMWARE_RESET_REASON_CRASH:
    case HSP_FIRMWARE_RESET_REASON_UPDATE:
    case HSP_FIRMWARE_RESET_REASON_WARM_RESET:
    case HSP_FIRMWARE_RESET_REASON_HSP_ONLY_WARM_RESET:
        return MSCP_SUBSYS_RESET;
    case HSP_FIRMWARE_RESET_REASON_DEBUG_UNLOCK:
    case HSP_FIRMWARE_RESET_REASON_COLD_BOOT:
        return COLD_RESET;
    case HSP_FIRMWARE_RESET_REASON_UNDEFINED:
    case HSP_FIRMWARE_RESET_REASON_SHUTDOWN:
    default:
        break;
    }

    return SHUTDOWN; // Default
}

// Callback function for the asynchronous PrepareForCoreReset receive operation
void prepare_reset_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    fpfw_icc_base_recv_req_t* recv_context = (fpfw_icc_base_recv_req_t*)context;

    if (status != DFWK_SUCCESS)
    {
        SOS_LOG_CRIT("ICC Receive Error in prepare_reset_recv_cb: 0x%08x", (unsigned int)status);
    }
    else
    {
        FPFW_RUNTIME_ASSERT(recv_context->recv_cmd_code == HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
        FPFW_RUNTIME_ASSERT(recv_context->payload_buffer != NULL);
        FPFW_RUNTIME_ASSERT(recv_context->buffer_size >= sizeof(kng_hsp_mailbox_msg));
        kng_hsp_mailbox_msg* hsp_recv_msg = (kng_hsp_mailbox_msg*)(recv_context->payload_buffer);

        // Prepare the queisce request with shutdown type.
        static startup_shutdown_request_t shutdown_request;
        DfwkAsyncRequestInitialize((void*)&shutdown_request.header, sizeof(shutdown_request));

        shutdown_request.header.RequestType = STARTUP_REQUEST_QUIESCE_ASYNC;
        shutdown_request.shutdown_type =
            convert_HSP_reset_reason_to_shutdown_type(hsp_recv_msg->prepare_for_core_reset_req.reason);

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

void sos_icc_init(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_ctx_t* icc_d2dmbx_ctx)
{
    FpFwLockInitialize(&icc_lock);
    icc_hspmbx_ctx = icc_ctx;
    icc_die2die_ctx = icc_d2dmbx_ctx;
    // Setup receive request and register to receive and handle PrepareForCoreReset command from HSP
    receive_prep_core_reset();
    recv_d2d_shutdown_request();
}

void sos_d2d_shutdown_send_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);

    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        SOS_LOG_CRIT("PPU shutdown request failed to send: 0x%08x", (int)status);
    }
    else
    {
        SOS_LOG_INFO("PPU shutdown request sent successfully.");
    }
}

void sos_send_d2d_shutdown_request()
{
    SOS_LOG_INFO("Sending PPU shutdown request to remote die");
    static uint8_t icc_mhu_send_d2d_shutdown_payload[sizeof(icc_mhu_header_t) + 1] = {0};
    icc_mhu_packet_t* icc_send_req = (icc_mhu_packet_t*)icc_mhu_send_d2d_shutdown_payload;

    icc_send_req->header.msg_header.command = ICC_D2D_SHUTDOWN_REQUEST;
    icc_send_req->header.msg_header.payload_size = 1;

    static fpfw_icc_base_send_req_t ppu_shutdown_send_params;

    ppu_shutdown_send_params.payload_buffer = icc_mhu_send_d2d_shutdown_payload; // Buffer containing the message to send
    ppu_shutdown_send_params.buffer_size = sizeof(icc_mhu_send_d2d_shutdown_payload); // Size of the buffer
    ppu_shutdown_send_params.cb = sos_d2d_shutdown_send_cb; // Callback function for handling the send completion
    ppu_shutdown_send_params.cb_ctx = NULL;                 // Context for the callback

    // Send the message to the remote die
    fpfw_status_t icc_status = fpfw_icc_base_send(icc_die2die_ctx, &ppu_shutdown_send_params);
    if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        SOS_LOG_CRIT("Failed to send shutdown request to remote die");
    }
    else
    {
        SOS_LOG_INFO("Shutdown request sent successfully to remote die");
    }
}

// Callback function for the completion of the reset notification
void d2d_shutdown_req_complete_notify(DFWK_ASYNC_REQUEST_HEADER* request, void* context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(context);
    SOS_LOG_INFO("D2D shutdown request complete notification received.");
}

void d2d_shutdown_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    // Process the received message
    SOS_LOG_INFO("Received PPU shutdown request from remote die");

    static startup_shutdown_request_t shutdown_request;
    DfwkAsyncRequestInitialize((void*)&shutdown_request.header, sizeof(shutdown_request));
    // Perform the necessary actions to shut down PPUs
    sos_shutdown((void*)fpfw_init_get_handle("sos_int"), &shutdown_request, REMOTE_SCP_SHUTDOWN, d2d_shutdown_req_complete_notify, NULL);
}

void recv_d2d_shutdown_request()
{
    SOS_LOG_INFO("Registering to receive shutdown request from remote die");

    // Static buffer to hold the received message
    static uint8_t ppu_shutdown_recv_payload[512] = {0};

    static fpfw_icc_base_recv_req_t ppu_shutdown_recv_params;
    ppu_shutdown_recv_params.recv_cmd_code = ICC_D2D_SHUTDOWN_REQUEST;        // Command code to listen for
    ppu_shutdown_recv_params.payload_buffer = ppu_shutdown_recv_payload;      // Buffer to receive the message
    ppu_shutdown_recv_params.buffer_size = sizeof(ppu_shutdown_recv_payload); // Size of the buffer
    ppu_shutdown_recv_params.cb = d2d_shutdown_recv_cb; // Callback function for handling the received message
    ppu_shutdown_recv_params.cb_ctx = ppu_shutdown_recv_payload; // Context for the callback

    SOS_LOG_INFO("PPU shutdown send buffer size: %zu", ppu_shutdown_recv_params.buffer_size);
    // Register to receive the shutdown request
    fpfw_status_t recv_status = fpfw_icc_base_recv(icc_die2die_ctx, &ppu_shutdown_recv_params);
    if (recv_status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        SOS_LOG_CRIT("Failed to register for shutdown request: 0x%08x", (int)recv_status);
    }
    else
    {
        SOS_LOG_INFO("Successfully registered to receive shutdown request");
    }
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
    case REMOTE_SCP_SHUTDOWN:
        // Shutdown request to HSP has already been sent by the initiating SCP, skip sending superfluous shutdown request to HSP
        return result;
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

void sos_core_override_timeout(pstartup_reset_timeout_request_t request)
{
    if (request->timeout.stage_category == BOOT_STAGE)
    {
        sos_boot_timeout_override(request->timeout);
    }
    else if (request->timeout.stage_category == SHUTDOWN_STAGE)
    {
        sos_shutdown_timeout_override(request->timeout);
    }
    else
    {
        SOS_LOG_CRIT("Invalid stage type %d", request->timeout.stage_category);
        FPFW_RUNTIME_ASSERT(false);
    }
}
