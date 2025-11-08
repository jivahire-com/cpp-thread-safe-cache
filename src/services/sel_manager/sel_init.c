//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_init.c
 * Implements the initialization of sel manager.
 */

/*------------- Includes -----------------*/
#include "sel_i.h"

#include <DbgPrint.h>
#include <bug_check.h>
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <idsw_kng.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif
#include <sel.h>
#include <sel_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool s_pldm_ready = false;
static icc_mhu_sel_ctx_t s_icc_ctx[SEL_ICC_MAX] = {0};

/*------------- Functions ----------------*/
#ifdef PLDM_DRV_WORKAROUND
static void pldm_platform_event_ready_callback(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    pldm_request_t* pldm_request = (pldm_request_t*)Request;
    FPFW_UNUSED(CompletionContext);

    BUG_ASSERT_PARAM(Request->RequestType == PLDM_GET_READY_ASYNC, Request->RequestType, 0);
    BUG_ASSERT_PARAM(pldm_request->status == FPFW_STATUS_SUCCESS, pldm_request->status, 0);

    // Configure SEL to ready to send and send any pendding SEL logs.
    s_pldm_ready = true;

    // Send any pendding SEL logs.
    sel_flush_queue();
}
#else
static void pldm_platform_event_ready_callback(uint16_t event_id, void* context)
{
    FPFW_UNUSED(event_id);
    FPFW_UNUSED(context);

    // Configure SEL to ready to send and send any pendding SEL logs.
    s_pldm_ready = true;

    // Send any pendding SEL logs.
    sel_flush_queue();
}
#endif

bool sel_is_PLDM_ready()
{
    return s_pldm_ready;
}

fpfw_icc_base_ctx_t* sel_icc_ctx(sel_icc_type_t type)
{
    return s_icc_ctx[type].icc_ctx;
}

void sel_init()
{
    // Initialize SEL event entity queue.
    sel_init_queue();
}

KNG_STATUS sel_register_icc(sel_icc_type_t type, fpfw_icc_base_ctx_t* icc_ctx)
{
    fpfw_status_t status = 0;

    if (icc_ctx == NULL || type >= SEL_ICC_MAX)
    {
        FPFW_DBGPRINT_ERROR("SEL: Invalid ICC ctx\n");
        return KNG_E_INVALIDARG;
    }

    s_icc_ctx[type].icc_ctx = icc_ctx;

    // Register ICC receive callback
    status = icc_register_mhu_sel_transfer_callback(&s_icc_ctx[type]);
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);

    return KNG_SUCCESS;
}

KNG_STATUS sel_register_pldm()
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    // Register PLDM Ready Callback for MCP0
    if (idsw_get_die_id() == DIE_0 && idsw_get_cpu_type() == CPU_MCP)
    {
#ifdef PLDM_DRV_WORKAROUND
        static pldm_request_t ready_request = {0};

        DfwkAsyncRequestSetCompletionRoutine(&ready_request.header, pldm_platform_event_ready_callback, NULL);
        status = pldm_drv_register_platform_event_ready_notification(&ready_request);
#else
        pldm_platform_event_ready_notification ready_notification = {
            .CallBack = pldm_platform_event_ready_callback,
            .context = NULL, // No context needed for this callback
        };

        status = fpfw_pldm_service_register_platform_event_ready_notification(&ready_notification);
#endif
        BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);
    }

    return status;
}
