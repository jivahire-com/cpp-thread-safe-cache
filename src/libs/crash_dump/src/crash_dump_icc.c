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
#include <crash_dump_events.h>    // for CRASH_DUMP_ET
#include <fpfw_cfg_mgr.h>         // for knobs
#include <fpfw_icc_base.h>        // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <icc_mhu.h>              // for icc_mhu_request_t
#include <icc_platform_defines.h> // for RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ
#include <kng_icc_shared.h>       // for ICC_SIGNAL_CRASH_DUMP_COLLECT

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void icc_base_recv_complete_notify_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    BUG_CHECK_EXTERNAL();
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
    crash_dump_register_post_dump_callback(crash_dump_copy_accel_cd_file, (void*)accel_type, CRASH_DUMP_TYPE_FULL);
    FPFwCDPrintf("[CD][Accel %u]: CD file offset 0x%lx size 0x%lx\n", accel_type, msg->cd_file_offset, msg->cd_file_size);
}

static bool icc_register_mhu_remote_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t rem_byte_payload[512];
    static fpfw_icc_base_recv_req_t crashdump_rem_mhu_recv_req = {
        .payload_buffer = rem_byte_payload,
        .buffer_size = FPFW_ARRAY_SIZE(rem_byte_payload),
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_COLLECT,
        .cb = icc_base_recv_complete_notify_cb,
        .cb_ctx = NULL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_rem_mhu_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_mhu_local_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t loc_byte_payload[512];
    static fpfw_icc_base_recv_req_t crashdump_loc_mhu_recv_req = {
        .payload_buffer = loc_byte_payload,
        .buffer_size = FPFW_ARRAY_SIZE(loc_byte_payload),
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_COLLECT,
        .cb = icc_base_recv_complete_notify_cb,
        .cb_ctx = NULL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_loc_mhu_recv_req);

    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static bool icc_register_spi_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static rmss_d2d_mailbox_msg d2d_msg = {0};
    static fpfw_icc_base_recv_req_t crashdump_remote_noti_recv_req = {
        .payload_buffer = &d2d_msg,
        .buffer_size = sizeof(rmss_d2d_mailbox_msg),
        .recv_cmd_code = RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ,
        .cb = icc_base_recv_complete_notify_cb,
        .cb_ctx = NULL,
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

void crash_dump_config_icc(crash_dump_icc_config_t type, fpfw_icc_base_ctx_t* icc_ctx)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump context is not set for %d ICC\n", type);
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_INVALID_ADDRESS, type);
        return;
    }

    if (icc_ctx != NULL && ctx->icc_ctx[type] == NULL)
    {
        bool callback_registered = false;

        switch (type)
        {
        case CRASH_DUMP_ICC_CONFIG_MHU_LOCAL:
            callback_registered = icc_register_mhu_local_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_MHU_REMOTE:
            callback_registered = icc_register_mhu_remote_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_SPI_REMOTE:
            callback_registered = icc_register_spi_callback(icc_ctx);
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
        }
    }
    else
    {
        FPFwCDPrintf("ICC context is NULL for %d ICC or ICC context is already set %p\n", type, ctx->icc_ctx[type]);
    }
}

static void crash_dump_send_hsp_command(uint32_t command)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP] != NULL)
    {
        kng_hsp_mailbox_msg hsp_crash_dump_msg;

        hsp_crash_dump_msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(command, 0, 0);

        fpfw_status_t status =
            fpfw_icc_base_send_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP], &hsp_crash_dump_msg, sizeof(hsp_crash_dump_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send 0x%08lx command to HSP: status = 0x%08lx\n", command, status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR, status);
        }
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
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ICC_INVALID_ADDRESS);
        return;
    }

    if (ctx->core_index == CRASH_DUMP_CORE_SCP)
    {
        crash_dump_send_hsp_command(HSP_MAILBOX_CMD_CRASHDUMP_REQ);
    }
}

/**
 * Notify Accel devices that this core has crashed by using Largefifo mailbox sync send.
 */
void crash_dump_notify_accelerators()
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump config is not set to notify SDM/CDED\n");
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ICC_INVALID_ADDRESS);
        return;
    }

    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        crash_dump_icc_config_t icc_config_type;
        if (accel_type == ACCEL_ID_SDM)
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_SDM;
        }
        else
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_CDED;
        }

        fpfw_icc_base_ctx_t* icc_ctx = ctx->icc_ctx[icc_config_type];
        large_fifo_mailbox_msg largefifo_crash_dump_msg;

        if (icc_ctx == NULL)
        {
            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
            continue;
        }

        largefifo_crash_dump_msg.as_uint32[0] =
            SET_LARGE_FIFO_MAILBOX_HEADER_ASUNIT32(LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_REQ, 0, 0);

        fpfw_status_t status =
            fpfw_icc_base_send_sync(icc_ctx, &largefifo_crash_dump_msg, sizeof(largefifo_crash_dump_msg));

        if (status == FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_IN_PROGRESS);
        }
        else
        {
            FPFwCDPrintf("Fail to send CD req to %u: status = 0x%08lx\n", accel_type, status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR, status);

            crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
        }
    }
}

static fpfw_status_t icc_mhu_send_cd_sync(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t s_icc_mhu_send_payload[sizeof(icc_mhu_header_t) + 1] = {0};
    icc_mhu_packet_t* p_send_req = (icc_mhu_packet_t*)s_icc_mhu_send_payload;

    p_send_req->header.msg_header.command = ICC_SIGNAL_CRASH_DUMP_COLLECT;
    p_send_req->header.msg_header.payload_size = 1;

    return fpfw_icc_base_send_sync(icc_ctx, &s_icc_mhu_send_payload, sizeof(s_icc_mhu_send_payload));
}

/**
 * Notify local peer core that this core has crashed
 */
void crash_dump_notify_cores()
{
    fpfw_status_t status;
    bool remote_notified = false;
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        FPFwCDPrintf("Crash dump context is not set to notify other cores\n");
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_ICC_INVALID_ADDRESS);
        return;
    }

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL] != NULL)
    {
        status = icc_mhu_send_cd_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL]);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to local core : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR, status);
        }
    }

    if (ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE] != NULL)
    {
        status = icc_mhu_send_cd_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE]);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR, status);
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

        status = fpfw_icc_base_send_sync(ctx->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE],
                                         &remote_core_cd_msg,
                                         sizeof(rmss_d2d_mailbox_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core via SPI : status = 0x%08lx\n", status);
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR, status);
        }
    }
}

void crash_dump_remote_trigger()
{
    if (!crash_dump_context()->single_core_mode)
    {
        // Notify to local and remote cores
        crash_dump_notify_cores();

        // Notify Accel devices SDM and CDED
        crash_dump_notify_accelerators();
    }

    // Notify to HSP
    crash_dump_notify_hsp();
}

void crash_dump_transfer_full_dump_to_bmc()
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (config_get_crash_dump_emit_only_minidump() || ctx->die_index != 0 || ctx->core_index != CRASH_DUMP_CORE_MCP)
    {
        // HSP will transfer mini dump to BMC
        return;
    }

    crash_dump_pldm_transfer_dump();
}