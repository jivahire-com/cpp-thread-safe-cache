//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe subsystem service threads.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>   // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwAssert.h>   // for FPFW_RUNTIME_ASSERT
#include <idsw_kng.h>
#include <pcie_config_variable.h>
#include <pcie_dfwk.h> // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_link_management_i.h>
#include <pcie_manager_i.h>        // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_phy_load_events.h>  // PhyFW load event
#include <pcie_rp_event_handler.h> // Process Events/misc RP helper functions
#include <pcie_ss_common.h>        // pciess_entity
#include <pcie_sync_requests_i.h>
#include <pciess.h>
#include <scp_pcie_manager.h>
#include <silibs_kng_soc.h>
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
/*
 * Service initialization sleep durations to yield worker threads while
 * they are waiting for phy sram load and reset events to complete
 * within an rpss.
 */
#define PRE_SI_WORKER_YIELD_TICKS  (2)
#define POST_SI_WORKER_YIELD_TICKS (200)

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
    FPFW_RUNTIME_ASSERT(req->AllocatedSize >= sizeof(pcie_async_request_t));
    DfwkAsyncRequestSetCompletionRoutine(req, callback, context);
    DfwkInterfaceSendAsync(iface, req);
}

static void send_async_wait_for_event_for_pciess(pcie_manager_context_t* ctx)
{
    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        send_async_wait_for_event(ctx, rp_idx, MAX_PENDING_WAIT_FOR_EVENT_PER_RP);
    }
}

static void send_full_pciess_init(pcie_manager_context_t* ctx)
{
    unsigned long worker_yield_ticks =
        (idsw_get_platform_sdv() >= PLATFORM_RVP_EVT_SILICON) ? POST_SI_WORKER_YIELD_TICKS : PRE_SI_WORKER_YIELD_TICKS;

    /*
     * Send initial configuration request - this is the first step in pcie init.
     * As this step will also populate all pcie configurations, set a bit in
     * the configuration event flag to signal that PCIe configs are now populated.
     *
     * To allow full initialization to complete, we can yield the thread once
     * initial rpss configuration is complete.
     */
    send_sync_rpss_initial_config((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);
    tx_event_flags_set(ctx->event_ptr, (1 << ctx->rpss_idx), TX_OR);
    tx_thread_sleep(worker_yield_ticks);

    /*
     * Send pre root port initialization request to setup the RPSS.
     *
     * To allow full initialization to complete, we can yield the thread once
     * initial rpss configuration is complete.
     */
    send_sync_rpss_pre_rp_init_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);
    tx_thread_sleep(worker_yield_ticks);

    /* RPSS is ready, initialize all enabled root ports on it */
    send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);
}

void send_async_wait_for_event(pcie_manager_context_t* ctx, uint8_t rp_idx, uint8_t num_event)
{
    pcie_ss_entity_t* rpss = send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);

    FPFW_RUNTIME_ASSERT(num_event <= MAX_PENDING_WAIT_FOR_EVENT_PER_RP);
    if (rpss->rps[rp_idx].enabled)
    {
        for (uint8_t req_per_rp = 0; req_per_rp < num_event; req_per_rp++)
        {
            pcie_async_request_t* async_req =
                &(ctx->async_req[(rp_idx * MAX_PENDING_WAIT_FOR_EVENT_PER_RP) + req_per_rp]);
            DfwkAsyncRequestInitialize(&(async_req->header), sizeof(pcie_async_request_t));
            async_req->rpss_index = ctx->rpss_idx;
            async_req->rp_index = rp_idx;
            async_req->rp_op = WAIT_FOR_EVENT;
            pcie_sched_async_op(&(ctx->iface->header), &(async_req->header), rpss_req_completion_cb, ctx);
        }
    }
}

void rpss_service_thread_fn(ULONG thread_input)
{
    /* Process the thread context block and obtain the device and interface handles */
    pcie_manager_context_t* ctx = (pcie_manager_context_t*)(thread_input);
    pciess_device_t* d = ctx->dev;
    pciess_device_interface_t* iface = ctx->iface;

    /* Initialize and open the interface for this root port subsystem */
    pcie_dfwk_interface_init(d, iface);
    DfwkClientInterfaceOpen(&(iface->header));

    /* Wait for the PCIe phy firmware load event - set within the AP core firmware load module */
    FPFW_DBGPRINT_INFO("RPSS[%d]: Waiting for phy firmware load\n", ctx->rpss_idx);
    pcie_phyfw_wait_load_event(ctx->phyfw_load_event_ptr);

    /* Start Initial PCIESS/RP Init */
    send_full_pciess_init(ctx);

    /* Send MAX_PENDING_WAIT_FOR_EVENT_PER_RP ASYNC Requests */
    send_async_wait_for_event_for_pciess(ctx);

    /* Send Start LT */
    initiate_link_training_on_rpss(ctx);

    while (true)
    {
        /* Check if there is a completion request waiting for us */
        pciess_completion_request_t cmpl_req = {0};
        (void)tx_queue_receive(&(ctx->work_queue), (void*)&cmpl_req, TX_WAIT_FOREVER);

        /* Handle completion based on request type */
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: WAIT_FOR_EVENT completed | Op: %d | Status: %d |\n",
                           ctx->rpss_idx,
                           cmpl_req.rp_index,
                           cmpl_req.op,
                           (int)cmpl_req.async_data.data);

        process_wait_for_event_data(ctx, &cmpl_req);
    }
}
