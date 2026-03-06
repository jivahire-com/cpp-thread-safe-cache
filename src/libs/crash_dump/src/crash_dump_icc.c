//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_icc.c
 * Crash dump internal APIs for ICC (Inter Core Communication).
 */

/*------------- Includes -----------------*/
#include "crash_dump_icc.h"

#include "crash_dump_accel.h"
#include "crash_dump_pldm.h"
#include "crash_dump_status.h"

#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <accelip_id.h>           // for ACCEL_ID_SDM, ACCEL_ID_CDED
#include <bug_check.h>            // for BUG_CHECK_EXTERNAL
#include <crash_dump.h>           // for crash_dump_context
#include <crash_dump_dfwk.h>      // for crash_dump_request_t
#include <crash_dump_events.h>    // for CRASH_DUMP_ET
#include <fpfw_cfg_mgr.h>         // for knobs
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>            // for fpfw_init_get_handle
#include <health_monitor.h>       // for hm_update_accel_fatal_cper_info
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <icc_mhu.h>              // for icc_mhu_request_t
#include <icc_platform_defines.h> // for RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ
#include <kng_icc_shared.h>       // for ICC_SIGNAL_CRASH_DUMP_COLLECT

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static fpfw_status_t icc_mhu_send_cd_async(fpfw_icc_base_ctx_t* icc_ctx, uint32_t command);
static void crash_dump_register_transfer_callback(crash_dump_icc_config_t type);

/*-- Declarations (Statics and globals) --*/
static TX_TIMER cd_oob_timer;

/*------------- Functions ----------------*/
static void icc_transfer_send_complete_cb(void* ctx, fpfw_status_t status)
{
    icc_mhu_header_t* msg = (icc_mhu_header_t*)ctx;
    FPFwCDPrintf("%s: cmd=0x%08lx, status = 0x%08lx\n", __FUNCTION__, msg->msg_header.command, status);
}

static fpfw_status_t icc_mhu_send_cd_async(fpfw_icc_base_ctx_t* icc_ctx, uint32_t command)
{
    static icc_mhu_header_t transfer_msg = {};
    transfer_msg.msg_header.command = command;

    static fpfw_icc_base_send_req_t icc_msg_ready_msg_req = {.payload_buffer = &transfer_msg,
                                                             .buffer_size = sizeof(transfer_msg),
                                                             .cb = icc_transfer_send_complete_cb,
                                                             .cb_ctx = &transfer_msg};

    return fpfw_icc_base_send(icc_ctx, &icc_msg_ready_msg_req);
}

static fpfw_status_t icc_mhu_send_cd_sync(fpfw_icc_base_ctx_t* icc_ctx, uint32_t command, uint32_t payload)
{
    static uint8_t s_icc_mhu_send_payload[sizeof(icc_mhu_header_t) + sizeof(uint32_t)] = {0};
    icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)s_icc_mhu_send_payload;

    p_send_req->header.msg_header.command = command;
    p_send_req->header.msg_header.payload_size = sizeof(uint32_t);
    p_send_req->payload[0] = payload;

    return fpfw_icc_base_send_sync(icc_ctx, &s_icc_mhu_send_payload, sizeof(s_icc_mhu_send_payload));
}

/*----------------------------------------*/
static void icc_recv_crashdump_collection_mhu_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    icc_mhu_packet_t* msg = (icc_mhu_packet_t*)context;

    BUG_CHECK_EXTERNAL(msg->payload[0]);
}

static void icc_recv_crashdump_collection_mb_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    rmss_d2d_mailbox_msg* msg = (rmss_d2d_mailbox_msg*)context;

    BUG_CHECK_EXTERNAL(msg->as_uint32[1]);
}

static void icc_recv_transfer_dump_req_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);
    crash_dump_icc_config_t icc_type = (crash_dump_icc_config_t)context;

    crash_dump_request_transfer_dump();

    // Register transfer callback for this ICC context again.
    crash_dump_register_transfer_callback(icc_type);
}

static void cd_accel_recv_addr_notify_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    accel_cd_params* accel_params = context;
    ACCEL_ID accel_type = accel_params->accel_type;
    accel_cd_addr_msg* msg = accel_params->params.payload_buffer;
    crash_dump_context_t* ctx = crash_dump_context();

    ctx->accel_cd_ctx[accel_type].cd_file_offset = msg->cd_file_offset;
    ctx->accel_cd_ctx[accel_type].cd_file_size = msg->cd_file_size;
    ctx->accel_cd_ctx[accel_type].cd_magic_nr_offset = msg->magic_nr_offset;

    hm_update_accel_fatal_cper_info((uint32_t)accel_type, msg->cper_buffer_offset, msg->cper_magic_nr_offset);
    crash_dump_register_post_dump_callback(crash_dump_copy_accel_cd_file, (void*)accel_type, CRASH_DUMP_TYPE_FULL);
    if (ctx->type_ctx[CRASH_DUMP_TYPE_FULL])
    {
        if (!(crash_dump_is_transferring(ctx->type_ctx[CRASH_DUMP_TYPE_FULL]) &&
              crash_dump_core_state(CRASH_DUMP_TYPE_FULL, ctx->die_index, CRASH_DUMP_CORE_SDM + accel_type) == CRASH_DUMP_STATE_COMPLETED))
        {
            // If crash dump is in transferring, accel state will be set to ready after transfer complete
            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_READY);
        }
    }
    FPFwCDPrintf("[CD][Accel %u]: CD file offset 0x%lx size 0x%lx\n", accel_type, msg->cd_file_offset, msg->cd_file_size);
}

static void crash_dump_transfer_req_cb(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);

    crash_dump_request_t* cd_request = (crash_dump_request_t*)Request;

    if (cd_request->status == FPFW_STATUS_SUCCESS)
    {
        FPFwCDPrintf("Crash dump transfer started successfully.\n");
    }
    else if (cd_request->status == (uint32_t)KNG_E_NOT_READY)
    {
        FPFwCDPrintf("Crash dump is empty.\n");
    }
    else
    {
        FPFwCDPrintf("Crash dump transfer failed to start: status = 0x%08lx\n", cd_request->status);
    }
}

/*----------------------------------------*/
static void hsp_mbox_send_complete_cb(void* ctx, fpfw_status_t status)
{
    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)ctx;
    FPFwCDPrintf("%s: msg=0x%08lx, status = 0x%08lx\n", __FUNCTION__, msg->as_uint32[0], status);
}

static fpfw_status_t crash_dump_send_hsp_command_async(uint32_t command, uint32_t flags)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP] != NULL)
    {
        static kng_hsp_mailbox_msg hsp_crash_dump_msg;
        hsp_crash_dump_msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(command, 0, flags);

        static fpfw_icc_base_send_req_t icc_msg_ready_msg_req = {.payload_buffer = &hsp_crash_dump_msg,
                                                                 .buffer_size = sizeof(hsp_crash_dump_msg),
                                                                 .cb = hsp_mbox_send_complete_cb,
                                                                 .cb_ctx = &hsp_crash_dump_msg};

        status = fpfw_icc_base_send(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP], &icc_msg_ready_msg_req);

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send 0x%08lx command to HSP: status = 0x%08lx\n", command, status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_HSP_CMD_ASYNC_ERROR, status);
        }
    }

    return status;
}

static void crash_dump_send_hsp_command_sync(uint32_t command, uint32_t flags)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP] != NULL)
    {
        kng_hsp_mailbox_msg hsp_crash_dump_msg;

        hsp_crash_dump_msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(command, 0, flags);

        fpfw_status_t status =
            fpfw_icc_base_send_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP], &hsp_crash_dump_msg, sizeof(hsp_crash_dump_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send 0x%08lx command to HSP: status = 0x%08lx\n", command, status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_HSP_CMD_SYNC_ERROR, status);
        }
    }
}

/*----------------------------------------*/
static bool icc_register_mhu_remote_cd_collection_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t rem_byte_payload[512];
    static fpfw_icc_base_recv_req_t crashdump_rem_mhu_recv_req = {
        .payload_buffer = rem_byte_payload,
        .buffer_size = FPFW_ARRAY_SIZE(rem_byte_payload),
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_COLLECT,
        .cb = icc_recv_crashdump_collection_mhu_cb,
        .cb_ctx = rem_byte_payload,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_rem_mhu_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_mhu_local_cd_collection_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t loc_byte_payload[512];
    static fpfw_icc_base_recv_req_t crashdump_loc_mhu_recv_req = {
        .payload_buffer = loc_byte_payload,
        .buffer_size = FPFW_ARRAY_SIZE(loc_byte_payload),
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_COLLECT,
        .cb = icc_recv_crashdump_collection_mhu_cb,
        .cb_ctx = loc_byte_payload,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_loc_mhu_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_spi_cd_collection_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static rmss_d2d_mailbox_msg d2d_msg = {0};
    static fpfw_icc_base_recv_req_t crashdump_remote_noti_recv_req = {
        .payload_buffer = &d2d_msg,
        .buffer_size = sizeof(rmss_d2d_mailbox_msg),
        .recv_cmd_code = RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ,
        .cb = icc_recv_crashdump_collection_mb_cb,
        .cb_ctx = &d2d_msg,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_remote_noti_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_accel_addr_callback(fpfw_icc_base_ctx_t* icc_ctx, ACCEL_ID accel_type)
{
    static accel_cd_params accel_params[NUM_VALID_ACCEL_ID];
    static accel_cd_addr_msg msg[NUM_VALID_ACCEL_ID];

    accel_params[accel_type].accel_type = accel_type;
    accel_params[accel_type].params.payload_buffer = &msg[accel_type];
    accel_params[accel_type].params.buffer_size = sizeof(msg[accel_type]);
    accel_params[accel_type].params.recv_cmd_code = LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_ADDR_REQ;
    accel_params[accel_type].params.cb = cd_accel_recv_addr_notify_cb;
    accel_params[accel_type].params.cb_ctx = &accel_params[accel_type].params;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &accel_params[accel_type].params);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_mhu_local_transfer_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static icc_mhu_header_t transfer_byte_payload;
    static fpfw_icc_base_recv_req_t crashdump_transfer_recv_req = {
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_TRANSFER,
        .payload_buffer = &transfer_byte_payload,
        .buffer_size = sizeof(transfer_byte_payload),
        .cb = icc_recv_transfer_dump_req_cb,
        .cb_ctx = (void*)CRASH_DUMP_ICC_CONFIG_MHU_LOCAL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_transfer_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_mhu_remote_transfer_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static icc_mhu_header_t transfer_byte_payload;
    static fpfw_icc_base_recv_req_t crashdump_transfer_recv_req = {
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_TRANSFER,
        .payload_buffer = &transfer_byte_payload,
        .buffer_size = sizeof(transfer_byte_payload),
        .cb = icc_recv_transfer_dump_req_cb,
        .cb_ctx = (void*)CRASH_DUMP_ICC_CONFIG_MHU_REMOTE,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_transfer_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_accel_transfer_callback(fpfw_icc_base_ctx_t* icc_ctx, ACCEL_ID accel_type)
{
    static accel_cd_params accel_params[NUM_VALID_ACCEL_ID];
    static accel_cd_addr_msg msg[NUM_VALID_ACCEL_ID];

    accel_params[accel_type].accel_type = accel_type;
    accel_params[accel_type].params.payload_buffer = &msg[accel_type];
    accel_params[accel_type].params.buffer_size = sizeof(msg[accel_type]);
    accel_params[accel_type].params.recv_cmd_code = LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_TRANSFER;
    accel_params[accel_type].params.cb = icc_recv_transfer_dump_req_cb;
    accel_params[accel_type].params.cb_ctx =
        (void*)(accel_type == ACCEL_ID_SDM ? CRASH_DUMP_ICC_CONFIG_SDM : CRASH_DUMP_ICC_CONFIG_CDED);

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &accel_params[accel_type].params);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static void crash_dump_register_transfer_callback(crash_dump_icc_config_t type)
{
    crash_dump_context_t* ctx = crash_dump_context();

    switch (type)
    {
    case CRASH_DUMP_ICC_CONFIG_MHU_LOCAL:
        icc_register_mhu_local_transfer_callback(ctx->icc_ctx[type]);
        break;
    case CRASH_DUMP_ICC_CONFIG_MHU_REMOTE:
        icc_register_mhu_remote_transfer_callback(ctx->icc_ctx[type]);
        break;
    case CRASH_DUMP_ICC_CONFIG_SDM:
        icc_register_accel_transfer_callback(ctx->icc_ctx[type], ACCEL_ID_SDM);
        break;
    case CRASH_DUMP_ICC_CONFIG_CDED:
        icc_register_accel_transfer_callback(ctx->icc_ctx[type], ACCEL_ID_CDED);
        break;
    default:
        break;
    }
}

/*----------------------------------------*/
void crash_dump_config_icc(crash_dump_icc_config_t type, fpfw_icc_base_ctx_t* icc_ctx)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump context is not set for %d ICC\n", type);
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_INVALID_CRASHDUMP_CONTEXT, __LINE__);
        return;
    }

    if (icc_ctx != NULL && ctx->icc_ctx[type] == NULL)
    {
        bool callback_registered = false;

        switch (type)
        {
        case CRASH_DUMP_ICC_CONFIG_MHU_LOCAL:
            callback_registered = icc_register_mhu_local_cd_collection_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_MHU_REMOTE:
            callback_registered = icc_register_mhu_remote_cd_collection_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_SPI_REMOTE:
            callback_registered = icc_register_spi_cd_collection_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_HSP:
            // No need to register HSP context for receiving crash dump notification
            callback_registered = true;
            break;
        case CRASH_DUMP_ICC_CONFIG_SDM:
            callback_registered = icc_register_accel_addr_callback(icc_ctx, ACCEL_ID_SDM);
            break;
        case CRASH_DUMP_ICC_CONFIG_CDED:
            callback_registered = icc_register_accel_addr_callback(icc_ctx, ACCEL_ID_CDED);
            break;
        default:
            break;
        }

        if (callback_registered)
        {
            ctx->icc_ctx[type] = icc_ctx;

            // Register transfer callback for this ICC context
            crash_dump_register_transfer_callback(type);
        }
    }
    else
    {
        FPFwCDPrintf("ICC context is NULL for %d ICC or ICC context is already set %p\n", type, ctx->icc_ctx[type]);
        CRASH_DUMP_ET_WARNING_PARAM(CRASH_DUMP_ET_TYPE_ICC_ICC_CONTEXT_ALREADY_SET, __LINE__);
    }
}

/**
 * Notify HSP that this core has crashed by using HSP mailbox sync send.
 */
void crash_dump_notify_hsp()
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump context is not set to notify HSP\n");
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_INVALID_CRASHDUMP_CONTEXT, __LINE__);
        return;
    }

    if (ctx->core_index == CRASH_DUMP_CORE_SCP)
    {
        uint32_t flags = config_get_crash_dump_warm_reset() ? 0 : HSP_MAILBOX_FLAGS_CRASHDUMP_SKIP_WARM_RESET;

        crash_dump_send_hsp_command_sync(HSP_MAILBOX_CMD_CRASHDUMP_REQ, flags);
    }
}

fpfw_status_t crash_dump_notify_hsp_transfer_complete(uint32_t flags)
{
    return crash_dump_send_hsp_command_async(HSP_MAILBOX_CMD_MCP_CRASHDUMP_COPY_COMPLETE_NOTIFY, flags);
}

/**
 * Notify Accel devices that this core has crashed by using Largefifo mailbox sync send.
 */
void crash_dump_notify_accelerators(bool is_ue, uint32_t origin_core)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump config is not set to notify SDM/CDED\n");
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_INVALID_CRASHDUMP_CONTEXT, __LINE__);
        return;
    }

    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        crash_dump_icc_config_t icc_config_type;
        uint32_t accel_core_id = 0;
        if (accel_type == ACCEL_ID_SDM)
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_SDM;
            accel_core_id = CRASH_DUMP_CORE_SDM;
        }
        else
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_CDED;
            accel_core_id = CRASH_DUMP_CORE_CDED;
        }

        fpfw_icc_base_ctx_t* icc_ctx = ctx->icc_ctx[icc_config_type];
        large_fifo_mailbox_msg largefifo_crash_dump_msg;

        if (icc_ctx == NULL)
        {
            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
            continue;
        }

        largefifo_crash_dump_msg.as_uint32[0] =
            SET_LARGE_FIFO_MAILBOX_HEADER_ASUNIT32(LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_REQ, is_ue ? CRASH_DUMP_UE_FLAG : 0, 0);
        largefifo_crash_dump_msg.data[0] = origin_core;

        fpfw_status_t status =
            fpfw_icc_base_send_sync(icc_ctx, &largefifo_crash_dump_msg, sizeof(largefifo_crash_dump_msg));

        if (status == FPFW_ICC_BASE_STATUS_SUCCESS &&
            crash_dump_core_state(CRASH_DUMP_TYPE_FULL, ctx->die_index, accel_core_id) == CRASH_DUMP_STATE_READY)
        {
            // Set accel crash dump state to in progress if the core is ready to collect crash dump.
            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_IN_PROGRESS);
        }
        else
        {
            FPFwCDPrintf("Fail to send CD req to %u: status = 0x%08lx\n", accel_type, status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_LARGE_FIFO_CD_MSG_ERROR, status);

            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
        }
    }
}

/**
 * Notify local peer core that this core has crashed
 */
void crash_dump_notify_cores(uint32_t origin_core)
{
    fpfw_status_t status;
    bool remote_notified = false;
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump context is not set to notify other cores\n");
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_INVALID_CRASHDUMP_CONTEXT, __LINE__);
        return;
    }

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL] != NULL)
    {
        status = icc_mhu_send_cd_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL], ICC_SIGNAL_CRASH_DUMP_COLLECT, origin_core);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to local core : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_MHU_SEND_CD_SYNC_LOCAL_ERROR, status);
        }
    }

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE] != NULL)
    {
        status = icc_mhu_send_cd_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE], ICC_SIGNAL_CRASH_DUMP_COLLECT, origin_core);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_MHU_SEND_CD_SYNC_REMOTE_ERROR, status);
        }
        else
        {
            remote_notified = true;
        }
    }

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE] != NULL && !remote_notified)
    {
        // Try with SPI transport ICC if MHU transport fore remote is not available.
        static rmss_d2d_mailbox_msg remote_core_cd_msg = {.header.cmd = RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ};
        remote_core_cd_msg.as_uint32[1] = origin_core;

        status = fpfw_icc_base_send_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE],
                                         &remote_core_cd_msg,
                                         sizeof(rmss_d2d_mailbox_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core via SPI : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_LARGE_REMOTE_CORE_CD_MSG_ERROR, status);
        }
    }
}

void crash_dump_remote_trigger(bool is_ue, uint32_t origin_core)
{
    FPFwCDPrintf("Notify crash from %lx\n", origin_core);
    if (!crash_dump_context()->single_core_mode)
    {
        // Notify to local and remote cores
        crash_dump_notify_cores(origin_core);

        // Notify Accel devices SDM and CDED
        crash_dump_notify_accelerators(is_ue, origin_core);
    }

    // Notify to HSP
    crash_dump_notify_hsp();
}

/*----------------------------------------*/
uint32_t crash_dump_transfer_full_dump_to_bmc()
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (config_get_crash_dump_emit_only_minidump() || ctx->die_index != 0 || ctx->core_index != CRASH_DUMP_CORE_MCP)
    {
        // HSP will transfer mini dump to BMC
        return KNG_SUCCESS;
    }

    pcrash_dump_interface_t cd_iface = fpfw_init_get_handle("cd_drv");
    static crash_dump_request_t cd_request = {};

    return crash_dump_start_transfer_async(cd_iface, &cd_request, crash_dump_transfer_req_cb, NULL);
}

static void crash_dump_oob_transfer_cb(ULONG input)
{
    FPFW_UNUSED(input);

    if (crash_dump_oob_transfer())
    {
        crash_dump_transfer_full_dump_to_bmc();
    }
}

uint32_t crash_dump_start_oob_transfer_agent()
{
    uint32_t interval = config_get_crash_dump_transfer_try_interval_ms() * TX_TIMER_TICKS_PER_SECOND / 1000;

    UINT ret = tx_timer_create(&cd_oob_timer,              // timer_ptr
                               "cd_oob_tx",                // name_ptr
                               crash_dump_oob_transfer_cb, // expiration_function
                               0,                          // input
                               1,                          // initial_ticks
                               interval,                   // reschedule_ticks
                               TX_AUTO_ACTIVATE);          // auto_activate
    BUG_ASSERT(ret == TX_SUCCESS);

    return ret;
}

uint32_t crash_dump_request_transfer_dump()
{
    crash_dump_context_t* ctx = crash_dump_context();
    fpfw_icc_base_ctx_t* icc_ctx = NULL;
    uint32_t status = KNG_SUCCESS;

    if (ctx->die_index == 0) // DIE 0
    {
        if (ctx->core_index == CRASH_DUMP_CORE_MCP)
        {
            // Transfer dump to the BMC
            FPFwCDPrintf("%s: MCP0 starts transfer\n", __FUNCTION__);
            status = crash_dump_transfer_full_dump_to_bmc();
        }
        else
        {
            // SCP0 need to relay this request to the MCP0
            FPFwCDPrintf("%s: SCP0 request to MCP0\n", __FUNCTION__);
            icc_ctx = ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL];
        }
    }
    else // DIE 1
    {
        if (ctx->core_index == CRASH_DUMP_CORE_MCP)
        {
            // MCP1 need to relay this request to the SCP1
            FPFwCDPrintf("%s: MCP1 request to SCP1\n", __FUNCTION__);
            icc_ctx = ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL];
        }
        else
        {
            // SCP1 need to relay this request to the SCP0
            FPFwCDPrintf("%s: SCP1 request to SCP0\n", __FUNCTION__);
            icc_ctx = ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE] != NULL
                          ? ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE]
                          : ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE];
        }
    }

    if (icc_ctx != NULL)
    {
        status = icc_mhu_send_cd_async(icc_ctx, ICC_SIGNAL_CRASH_DUMP_TRANSFER);
    }

    return status;
}