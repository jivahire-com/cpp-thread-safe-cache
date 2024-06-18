//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe subsystem service threads.
 */

/*------------- Includes -----------------*/
#include "FpFwAssert.h" // for FPFW_RUNTIME_ASSERT

#include <DfwkClient.h> // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <idsw.h>
#include <pcie_dfwk.h>        // for pcie_async_request_t, pcie_dfwk_interf...
#include <pcie_manager_i.h>   // for rpss_req_completion_cb, rpss_service_t...
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdbool.h>          // for true
#include <stdint.h>           // for uint8_t
#include <stdio.h>            // for fflush, printf, stdout
#include <tx_api.h>           // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

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

void send_start_link_training_requests(pcie_manager_context_t* ctx)
{
    /* The number of root ports to be enabled will change based on PCIe configuration */
    for (uint8_t i = 0; i < ROOT_PORTS_PER_RPSS; i++)
    {
        pcie_async_request_t* async_req = &(ctx->async_req[i]);
        DfwkAsyncRequestInititalize(&(async_req->header), sizeof(pcie_async_request_t));
        async_req->rpss_index = ctx->rpss_idx;
        async_req->rp_index = i;
        async_req->rp_op = INITIATE_LINK_TRAINING;
        pcie_sched_async_op(&(ctx->iface->header), &(async_req->header), rpss_req_completion_cb, ctx);
    }
}

void rpss_service_thread_fn(ULONG thread_input)
{
    /* Process the thread context block and obtain the device and interface handles */
    pcie_manager_context_t* ctx = (pcie_manager_context_t*)(thread_input);
    pciess_device_t* d = ctx->dev;
    pciess_device_interface_t* iface = ctx->iface;
    unsigned long worker_yield_ticks =
        (idsw_get_platform_sdv() >= PLATFORM_RVP_EVT_SILICON) ? POST_SI_WORKER_YIELD_TICKS : PRE_SI_WORKER_YIELD_TICKS;

    /* Initialize and open the interface for this root port subsystem */
    pcie_dfwk_interface_init(d, iface);
    DfwkClientInterfaceOpen(&(iface->header));

    /* Send synchronous requests for pre-link training RPSS setup */
    pcie_sync_request_t sync_req = {
        .header = {.RequestType = INITIAL_CONFIG_REQUEST},
        .rpss_index = ctx->rpss_idx,
        .rp_index = 0xF,
        .req_type = INITIAL_CONFIG_REQUEST,
    };
    int32_t status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
    tx_thread_sleep(worker_yield_ticks);

    sync_req.header.RequestType = PRE_RP_INIT_REQUEST;
    sync_req.req_type = PRE_RP_INIT_REQUEST;
    status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
    tx_thread_sleep(worker_yield_ticks);

    sync_req.header.RequestType = POST_RP_INIT_REQUEST;
    sync_req.req_type = POST_RP_INIT_REQUEST;
    status = DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(ctx->iface), &sync_req.header);
    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);

    /* Begin link training on all enabled root ports on this subsystem */
    send_start_link_training_requests(ctx);

    while (true)
    {
        /* Check if there is a completion request waiting for us */
        pciess_completion_request_t cmpl_req = {0};
        (void)tx_queue_receive(&(ctx->work_queue), (void*)&cmpl_req, TX_WAIT_FOREVER);

        /* Handle completion based on request type */
        printf("%s: PCIe SS: %d | Root port: %d | Op: %d | Status: %d |\n",
               __func__,
               ctx->rpss_idx,
               cmpl_req.rp_index,
               cmpl_req.op,
               (int)cmpl_req.status);
        fflush(stdout);
    }
}
