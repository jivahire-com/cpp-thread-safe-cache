//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_icc.c
 * Crash dump internal APIs for ICC (Inter Core Communication).
 */

/*------------- Includes -----------------*/
#include "crash_dump_icc.h"

#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <bug_check.h>            // for BUG_CHECK_EXTERNAL
#include <crash_dump.h>           // for GetCrashDumpConfig
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

static bool icc_register_mhu_callback(fpfw_icc_base_ctx_t* icc_ctx)
{
    static uint8_t byte_payload[512];
    static fpfw_icc_base_recv_req_t crashdump_mhu_recv_req = {
        .payload_buffer = byte_payload,
        .buffer_size = FPFW_ARRAY_SIZE(byte_payload),
        .recv_cmd_code = ICC_SIGNAL_CRASH_DUMP_COLLECT,
        .cb = icc_base_recv_complete_notify_cb,
        .cb_ctx = NULL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &crashdump_mhu_recv_req);

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

void crash_dump_config_icc(crash_dump_icc_config_t type, fpfw_icc_base_ctx_t* icc_ctx)
{
    crash_dump_config_t* config = GetCrashDumpConfig();

    if (config == NULL)
    {
        FPFwCDPrintf("Crash dump config is not set for %d ICC\n", type);
        return;
    }

    if (icc_ctx != NULL && config->icc_ctx[type] == NULL)
    {
        bool callback_registered = false;

        switch (type)
        {
        case CRASH_DUMP_ICC_CONFIG_MHU_LOCAL:
        case CRASH_DUMP_ICC_CONFIG_MHU_REMOTE:
            callback_registered = icc_register_mhu_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_SPI_REMOTE:
            callback_registered = icc_register_spi_callback(icc_ctx);
            break;
        case CRASH_DUMP_ICC_CONFIG_HSP:
            // No need to register HSP context for receiving crash dump notification
            callback_registered = true;
            break;
        default:
            break;
        }

        if (callback_registered)
        {
            config->icc_ctx[type] = icc_ctx;
        }
    }
    else
    {
        FPFwCDPrintf("ICC context is NULL for %d ICC or ICC context is already set %p\n", type, config->icc_ctx[type]);
    }
}

/**
 * Notify HSP that this core has crashed by using HSP mailbox sync send.
 */
void crash_dump_notify_hsp()
{
    crash_dump_config_t* config = GetCrashDumpConfig();

    if (config == NULL)
    {
        FPFwCDPrintf("Crash dump config is not set to notify HSP\n");
        return;
    }

    if (config->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP] != NULL)
    {
        kng_hsp_mailbox_msg hsp_crash_dump_msg;

        hsp_crash_dump_msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_CRASHDUMP_REQ, 0, 0);

        fpfw_status_t status = fpfw_icc_base_send_sync(config->icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP],
                                                       &hsp_crash_dump_msg,
                                                       sizeof(hsp_crash_dump_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crashdump request to HSP: status = 0x%08lx\n", status);
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
    crash_dump_config_t* config = GetCrashDumpConfig();

    if (config == NULL)
    {
        FPFwCDPrintf("Crash dump config is not set to notify other cores\n");
        return;
    }

    if (config->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL] != NULL)
    {
        status = icc_mhu_send_cd_sync(config->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL]);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to local core : status = 0x%08lx\n", status);
        }
    }

    if (config->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE] != NULL)
    {
        status = icc_mhu_send_cd_sync(config->icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE]);
        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core : status = 0x%08lx\n", status);
        }
        else
        {
            remote_notified = true;
        }
    }

    if (config->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE] != NULL && !remote_notified)
    {
        // Try with SPI transport ICC if MHU transport fore remote is not available.
        static rmss_d2d_mailbox_msg remote_core_cd_msg = {.header.cmd = RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ};

        status = fpfw_icc_base_send_sync(config->icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE],
                                         &remote_core_cd_msg,
                                         sizeof(rmss_d2d_mailbox_msg));

        if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFwCDPrintf("Failed to send Crash dump signal to remote core via SPI : status = 0x%08lx\n", status);
        }
    }
}

void crash_dump_remote_trigger()
{
    // Notify to local and remote cores
    crash_dump_notify_cores();

    // Notify to HSP
    crash_dump_notify_hsp();
}