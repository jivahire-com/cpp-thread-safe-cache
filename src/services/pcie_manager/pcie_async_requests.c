//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_async_requests.c
 * Encapsulates and provides APIs for the rest of the service to send
 * and receive async requests from the PCIe driver.
 */

/*------------- Includes -----------------*/
#include <DfwkAsyncRequest.h>
#include <DfwkPtrTypes.h>
#include <bug_check.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <pcie_sync_requests_i.h>
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void pcie_sched_async_op(PDFWK_INTERFACE_HEADER iface,
                                PDFWK_ASYNC_REQUEST_HEADER req,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                void* context);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void pcie_sched_async_op(PDFWK_INTERFACE_HEADER iface,
                                PDFWK_ASYNC_REQUEST_HEADER req,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                void* context)
{
    BUG_ASSERT_PARAM((req->AllocatedSize >= sizeof(pcie_async_request_t)), req->AllocatedSize, sizeof(pcie_async_request_t));
    DfwkAsyncRequestSetCompletionRoutine(req, callback, context);
    DfwkInterfaceSendAsync(iface, req);
}

void send_async_rp_wait_for_event(pcie_manager_context_t* ctx, pcie_async_request_t* async_req, uint8_t rp_idx)
{
    DfwkAsyncRequestInitialize(&(async_req->header), sizeof(pcie_async_request_t));
    async_req->rpss_index = ctx->rpss_idx;
    async_req->rp_index = rp_idx;
    async_req->rp_op = WAIT_FOR_EVENT;
    pcie_sched_async_op(&(ctx->iface->header), &(async_req->header), rpss_req_completion_cb, ctx);
}

void init_wait_for_event_queue_on_rpss(pcie_manager_context_t* ctx)
{
    pcie_ss_entity_t* rpss = send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);

    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        if (rpss->rps[rp_idx].enabled == false)
        {
            continue;
        }

        for (uint8_t req_per_rp = 0; req_per_rp < MAX_PENDING_WAIT_FOR_EVENT_PER_RP; req_per_rp++)
        {
            pcie_async_request_t* async_req =
                &(ctx->async_req[(rp_idx * MAX_PENDING_WAIT_FOR_EVENT_PER_RP) + req_per_rp]);
            send_async_rp_wait_for_event(ctx, async_req, rp_idx);
        }
    }
}
