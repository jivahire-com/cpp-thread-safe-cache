//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe subsystem service threads.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>
#include <bug_check.h>
#include <idsw_kng.h>
#include <pcie_async_requests_i.h>
#include <pcie_config_variable.h>
#include <pcie_link_management_i.h>
#include <pcie_lt_events.h>
#include <pcie_manager_i.h>
#include <pcie_phy_load_events.h>  // PhyFW load event
#include <pcie_rp_event_handler.h> // Process Events/misc RP helper functions
#include <pcie_sync_requests_i.h>
#include <pciess.h>
#include <scp_pcie_manager.h>
#include <silibs_kng_soc.h>
#include <silibs_status.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for fflush, printf, stdout
#include <tx_api.h>  // for TX_WAIT_FOREVER, ULONG, tx_queue_receive

/*-- Symbolic Constant Macros (defines) --*/
/*
 * Service initialization sleep durations to yield worker threads while
 * they are waiting for phy sram load and reset events to complete
 * within an rpss.
 */
#define PRE_RPSS_INIT_WORKER_YIELD_TICKS (500)
#define WORKER_YIELD_TICKS               (100)  /* = 1 second  */
#define PCIESS_RPSS_READY_TIMEOUT_TICKS  (8000) /* = 8 seconds */

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void send_full_pciess_init(pcie_manager_context_t* ctx)
{
    unsigned long elapsed_ticks = 0;
    silibs_status_t status = SILIBS_SUCCESS;

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

    /*
     * TODO WI: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2777075/
     * Timing impact of aux_clk_active never going low on the PHYs is being
     * investigated.
     *
     * Yielding the thread here for some time allow aux_clk_active to go low.
     */
    tx_thread_sleep(PRE_RPSS_INIT_WORKER_YIELD_TICKS);

    /*
     * Send pre root port initialization request to setup the RPSS.
     *
     * To allow full initialization to complete, we can yield the thread once
     * initial rpss configuration is complete.
     */
    send_sync_rpss_pre_rp_init_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);

    /* Wait for the rpss to get ready */
    do
    {
        status = send_sync_rpss_get_ready_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);
        if (status != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_INFO("RPSS[%d]: not ready yet, status (%d), waiting..\n", ctx->rpss_idx, status);
            tx_thread_sleep(WORKER_YIELD_TICKS);
            elapsed_ticks += WORKER_YIELD_TICKS;
        }
    } while ((status != SILIBS_SUCCESS) && (elapsed_ticks < PCIESS_RPSS_READY_TIMEOUT_TICKS));

    /*
     * If an RPSS never becomes ready, we need to crash the system, this is
     * not expected to happen on an healthy RPSS with all PHYs configured
     * correctly.
     */
    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Timed out waiting for RPSS to get ready!\n", ctx->rpss_idx);
        BUG_ASSERT_PARAM((status == SILIBS_SUCCESS), status, ctx->rpss_idx);
    }
    else
    {
        FPFW_DBGPRINT_INFO("RPSS[%d]: RPSS is ready! Elapsed ticks: %lu\n", ctx->rpss_idx, elapsed_ticks);
    }

    /* RPSS is ready, initialize all enabled root ports on it */
    send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);
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

    /* Initialize async request queue on the rpss to monitor events */
    init_wait_for_event_queue_on_rpss(ctx);

    /* Send Start LT */
    initiate_link_training_on_rpss(ctx);

    /* Complete the link training startup ssi phase */
    complete_ssi_ltssm_startup_req(ctx->rpss_idx);

    while (true)
    {
        /* Check if there is a completion request waiting for us */
        pciess_completion_request_t cmpl_req = {0};
        (void)tx_queue_receive(&(ctx->work_queue), (void*)&cmpl_req, TX_WAIT_FOREVER);

        /* Handle completion based on request type */
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: WAIT_FOR_EVENT completed | Op: %d | Interrupt Mask: %d |\n",
                           ctx->rpss_idx,
                           cmpl_req.rp_index,
                           cmpl_req.op,
                           (int)cmpl_req.async_data.int_mask);

        process_wait_for_event_data(ctx, &cmpl_req);
        fflush(stdout);

/* Break out of the loop for unit tests */
#ifdef _WIN32
        break;
#endif
    }
}
