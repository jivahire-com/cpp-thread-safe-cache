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
#include <ift_fw.h>
#include <pcie_async_requests_i.h>
#include <pcie_config_variable.h>
#include <pcie_link_management_i.h>
#include <pcie_lt_events.h>
#include <pcie_manager_events.h>
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
static void full_pciess_init_scp_cold_boot(pcie_manager_context_t* ctx)
{
    unsigned long elapsed_ticks = 0;
    silibs_status_t status = SILIBS_SUCCESS;
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
            FPFW_DBGPRINT_INFO("RPSS[%d]: not ready yet, status (%d), waiting..\n", ctx->rpss_idx, status); // Print Not removed as only during PCIESS Init.
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
        FPFW_DBGPRINT_ERROR("RPSS[%d]: Timed out waiting for RPSS to get ready!\n",
                            ctx->rpss_idx); // Print Not removed as only during PCIESS Init.
        PCIE_MANAGER_ET_ERROR_PARAM(PCIE_MANAGER_ET_TYPE_RPSS_READY_TIMEOUT, ctx->rpss_idx);
        BUG_ASSERT_PARAM((status == SILIBS_SUCCESS), status, ctx->rpss_idx);
    }
    else
    {
        FPFW_DBGPRINT_INFO("RPSS[%d]: RPSS is ready! Elapsed ticks: %lu\n", ctx->rpss_idx, elapsed_ticks); // Print Not removed as only during PCIESS Init.
    }
}

static bool send_full_pciess_init(pcie_manager_context_t* ctx)
{
    silibs_status_t sts;

    /*
     * Send initial configuration request - this is the first step in pcie init.
     * As this step will also populate all pcie configurations, set a bit in
     * the configuration event flag to signal that PCIe configs are now populated.
     *
     * To allow full initialization to complete, we can yield the thread once
     * initial rpss configuration is complete.
     */

    FPFW_DBGPRINT_INFO("RPSS[%d]: Cold Boot : 0x%d\n", ctx->rpss_idx, (uint8_t)ctx->is_cold_boot); // Print Not removed as only during PCIESS Init.
    sts = send_sync_rpss_initial_config((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx, &ctx->is_cold_boot);
    tx_event_flags_set(ctx->event_ptr, (1 << ctx->rpss_idx), TX_OR);

    /* RPSS is disabled by configuration, nothing more to do */
    if (sts == SILIBS_E_SUPPORT)
    {
        FPFW_DBGPRINT_INFO("RPSS[%d]: Disabled by configuration, skipping init.\n",
                           ctx->rpss_idx); // Print Not removed as only during PCIESS Init.

        /*
         * Complete the SSI LTSSM startup phase to prevent the platform boot
         * from hanging when all RPSS on a die are disabled. No boot race for
         * UEFI to attempt enumeration before link training is possible here
         * because the only production configuration that disables RPSS disables
         * all north-side instances (RPSS4-7) on die 1 together. There is never
         * a combination of enabled and disabled RPSS on the same die (which
         * can cause the race condition mentioned above).
         */
        complete_ssi_ltssm_startup_req(ctx->rpss_idx);
        return false;
    }

    /*
     * TODO WI: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2777075/
     * Timing impact of aux_clk_active never going low on the PHYs is being
     * investigated.
     *
     * Yielding the thread here for some time allow aux_clk_active to go low.
     */
    tx_thread_sleep(PRE_RPSS_INIT_WORKER_YIELD_TICKS);

    /* Pre and Post RPSS Init should be skipped if IFT is enabled */
    if (ift_is_enabled())
    {
        return true;
    }

    // Perform the following only when cold booting
    if (ctx->is_cold_boot)
    {
        full_pciess_init_scp_cold_boot(ctx);
    }

    /* RPSS is ready, initialize all enabled root ports on it */
    send_sync_rpss_post_rp_init_request((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx, &ctx->is_cold_boot);
    return true;
}

static bool rpss_init_scp_cold_boot(pcie_manager_context_t* ctx)
{
    /* Wait for the PCIe phy firmware load event - set within the AP core firmware load module */
    FPFW_DBGPRINT_INFO("RPSS[%d]: Waiting for phy firmware load\n", ctx->rpss_idx); // Print Not removed as only during PCIESS Init.
    pcie_phyfw_wait_load_event(ctx->phyfw_load_event_ptr);

    /* Start Initial PCIESS/RP Init */
    bool rpss_enabled = send_full_pciess_init(ctx);
    if (rpss_enabled == false)
    {
        return false;
    }

    /* Initialize async request queue on the rpss to monitor events */
    init_wait_for_event_queue_on_rpss(ctx);

    /* Send Start LT */
    initiate_link_training_on_rpss(ctx);

    /* Complete the link training startup ssi phase */
    complete_ssi_ltssm_startup_req(ctx->rpss_idx);
    return true;
}

static bool rpss_init_scp_warm_boot(pcie_manager_context_t* ctx)
{
    FPFW_DBGPRINT_INFO("RPSS[%d]: Warm Booting. \n", ctx->rpss_idx);
    bool rpss_enabled = send_full_pciess_init(ctx);
    if (rpss_enabled == false)
    {
        return false;
    }

    /* Initialize async request queue on the rpss to monitor events */
    init_wait_for_event_queue_on_rpss(ctx);
    return true;
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

    bool rpss_enabled = true;

    /* PCIE Link training and PCIE PHY training should be skipped incase of IFT */
    if (ift_is_enabled())
    {
        /* Start Initial PCIESS/RP Init */
        send_full_pciess_init(ctx);
        return;
    }

    if (ctx->is_cold_boot == true)
    {
        rpss_enabled = rpss_init_scp_cold_boot(ctx);
    }
    else
    {
        // SCP Warm Boot
        rpss_enabled = rpss_init_scp_warm_boot(ctx);
    }

    /* If RPSS is disabled, skip the event loop - thread exits */
    if (rpss_enabled == false)
    {
        FPFW_DBGPRINT_INFO("RPSS[%d]: Disabled, exiting service thread.\n", ctx->rpss_idx);
        return;
    }

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
                           (int)cmpl_req.async_data.int_mask); // Print Not removed as this is in Service thread context and is useful to have visibility on events coming from the RPSS.

        process_wait_for_event_data(ctx, &cmpl_req);
        fflush(stdout);

/* Break out of the loop for unit tests */
#ifdef _WIN32
        break;
#endif
    }
}
