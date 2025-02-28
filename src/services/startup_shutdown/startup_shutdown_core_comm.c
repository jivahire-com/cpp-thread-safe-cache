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
typedef struct
{
    bool in_use;
    kng_hsp_mailbox_msg msg;
    fpfw_icc_base_send_req_t hsp_send_params;
} shutdown_icc_req_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* icc_hspmbx_ctx = NULL;
static shutdown_icc_req_t shutdown_req = {0};
static FPFW_LOCK icc_lock; //! lock for protecting req_mem

/*------------- Functions ----------------*/
static void shutdown_req_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(status);
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&icc_lock);
    shutdown_icc_req_t* request = (shutdown_icc_req_t*)context;
    request->in_use = false;
    FpFwLockRelease(&icc_lock, oldState);
}

void sos_icc_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    FpFwLockInitialize(&icc_lock);
    icc_hspmbx_ctx = icc_ctx;
}

KNG_STATUS sos_request_shutdown(ssi_shutdown_type_t type)
{
    BUG_ASSERT_PARAM(icc_hspmbx_ctx != NULL, icc_hspmbx_ctx, &shutdown_req);

    KNG_STATUS result = KNG_SUCCESS;
    FPFW_LOCK_STATE oldState = FpFwLockAcquire(&icc_lock);

    if (shutdown_req.in_use)
    {
        result = KNG_E_BUSY;
    }
    else
    {
        shutdown_req.in_use = true;
    }

    FpFwLockRelease(&icc_lock, oldState);

    if (KNG_SUCCEEDED(result))
    {
        memset(&shutdown_req.msg, 0, sizeof(shutdown_req.msg));
        memset(&shutdown_req.hsp_send_params, 0, sizeof(shutdown_req.hsp_send_params));

        shutdown_req.msg.header.cmd = HSP_MAILBOX_MSG_UNKNOWN;

        switch (type)
        {
        case SHUTDOWN:
            shutdown_req.msg.as_uint32[0] = SET_HSP_MAILBOX_HEADER_ASUNIT32(HSP_MAILBOX_CMD_SHUTDOWN_REQ, 0, 0);
            break;
        case COLD_RESET:
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
                shutdown_req.in_use = false;
                result = KNG_E_FAIL;
            }
        }
    }

    return result;
}