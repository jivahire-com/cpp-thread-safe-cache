//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_link_recovery.c
 * Functions to handle events that necessitate link recovery (or re-train)
 * scenarios.
 */

/*------------- Includes -----------------*/
#include "DfwkPtrTypes.h"

#include <DbgPrint.h>
#include <DfwkClient.h>
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <pcie_rp_event_handler.h>
#include <pcie_ss_common.h>
#include <pcie_sync_requests_i.h>
#include <scp_pcie_manager.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <system_info.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_index, rpss_index) \
    (((rp_index & 0xFF) << 8) | (rpss_index & 0xFF))
#define GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val) (cb_val & 0xFF)
#define GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val)   ((cb_val >> 8) & 0xFF)
#define RP_LINK_TRAINING_TIMEOUT_TICKS             (500) /* = 5 seconds */
#define RP_TIMER_ONE_SHOT                          (0)
#define OVL_LT_RETRIES_MAX                         (3)
#define WAIT_SBR_MS                                (FPFW_MAX((TX_TIMER_TICKS_PER_SECOND / (200UL)), (1UL))) // 5ms

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void pcie_rp_timer_expiry_callback(unsigned long cb_val);
static void start_link_training_timer_for_rp(pcie_manager_context_t* ctx, uint8_t rp_idx);
static void initiate_link_training_for_rp(pcie_manager_context_t* ctx, uint8_t rp_index);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void pcie_rp_timer_expiry_callback(unsigned long cb_val)
{
    /*
     * The callback context is a combination of rpss index (first byte)
     * and root port index (second byte)
     */
    uint8_t rpss_idx = GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val);
    uint8_t rp_idx = GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val);
    pcie_manager_context_t* ctx = scp_pcie_get_manager_context(rpss_idx);

    FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Link training timer expired!\n", rpss_idx, rp_idx);

    /*
     * Even if the link training timer expires, we still need to check and log
     * the link status. Also initialize any capabilities that are required to
     * be initialized after link-up.
     */
    send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx);
    send_sync_rp_post_link_up_init((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx);
}

static void start_link_training_timer_for_rp(pcie_manager_context_t* ctx, uint8_t rp_idx)
{
    unsigned int status;

    unsigned long timer_ctx = SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_idx, ctx->rpss_idx);
    tx_timer_delete(&(ctx->expiry_timer[rp_idx]));
    status = tx_timer_create(&(ctx->expiry_timer[rp_idx]),
                             "pcie_link_training_timer",
                             pcie_rp_timer_expiry_callback,
                             timer_ctx,
                             RP_LINK_TRAINING_TIMEOUT_TICKS,
                             RP_TIMER_ONE_SHOT,
                             TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Failed to create link training timer! TX_STATUS: %d\n", ctx->rpss_idx, rp_idx, status);
        BUG_ASSERT_PARAM((status == TX_SUCCESS), status, 0);
    }
}

static void initiate_link_training_for_rp(pcie_manager_context_t* ctx, uint8_t rp_index)
{
    /* Start the link training timer on this RP */
    start_link_training_timer_for_rp(ctx, rp_index);

    /* Initiate link training on the RP */
    send_sync_rp_initiate_link_training((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx, rp_index);
}

static bool rp_is_overlake(uint8_t rpss_idx, uint8_t rp_idx)
{
    if (rp_idx != 0)
    {
        return false;
    }

    /* Check for mirrored config for 2S */
    uint8_t soc_position = system_info_get_soc_position();
    if (soc_position == 0x01)
    {
        return (config_get_overlake_rpss_index_secondary_soc() == rpss_idx);
    }

    return (config_get_overlake_rpss_index_primary_soc() == rpss_idx);
}

void initiate_link_training_on_rpss(pcie_manager_context_t* ctx)
{
    pcie_ss_entity_t* rpss = send_sync_rpss_get_entity((PDFWK_INTERFACE_HEADER)(ctx->iface), ctx->rpss_idx);

    /* The number of root ports to be enabled will change based on PCIe configuration */
    for (uint8_t rp_idx = 0; rp_idx < PCIESS_NUM_PORTS; rp_idx++)
    {
        if (rpss->rps[rp_idx].enabled)
        {
            initiate_link_training_for_rp(ctx, rp_idx);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Skip link training!\n", ctx->rpss_idx, rp_idx);
        }
    }
}

void handle_pcie_link_down_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    RPSS_INSTANCE rpss_idx = ctx->rpss_idx;
    uint8_t rp_index = cmpl->rp_index;

    /*
     * If the rp is not in ready state after link down, we cannot re-enable the
     * ltssm. Return directly.
     *
     * Do we need to log any errors here?
     */
    if (send_sync_rp_is_ready((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_index) == false)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: root port not ready after linkdown - cannot re-train!\n", rpss_idx, rp_index);
        return;
    }

    /* The root port is ready, initiate link re-training and return */
    initiate_link_training_for_rp(ctx, rp_index);
}

void handle_pcie_link_up_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    uint8_t rpss_idx = ctx->rpss_idx;
    uint8_t rp_idx = cmpl->rp_index;
    static uint8_t ovl_lt_retries = 0;
    silibs_status_t status = SILIBS_SUCCESS;

    status = send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);

    /* Deal with overlake re-training if this is an overlake root port */
    if (rp_is_overlake(rpss_idx, rp_idx) && config_get_enable_overlake_sbr_workaround())
    {
        if (status == SILIBS_E_OVERWRITTEN && ovl_lt_retries < OVL_LT_RETRIES_MAX)
        {
            /* Send the SBR, this will result in link-down flow being triggered */
            FPFW_DBGPRINT_INFO("RPSS[%d] OVL2: Link training failed, issue SBR\n", rpss_idx);
            send_sync_rp_set_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);
            tx_thread_sleep(WAIT_SBR_MS);
            send_sync_rp_clear_secondary_bus_reset((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);
            ovl_lt_retries++;
        }
        else if (status == SILIBS_SUCCESS)
        {
            /*
             * We should only retry on cold reset so set it to the max
             * to ensure any future link down events don't trigger SBR
             * flow
             */
            ovl_lt_retries = OVL_LT_RETRIES_MAX;
        }
    }

    send_sync_rp_post_link_up_init((PDFWK_INTERFACE_HEADER)ctx->iface, rpss_idx, rp_idx);
}

void handle_aer_force_link_down_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    /* We do not wait for the link-down event to occur, we simply issue the request and continue */
    send_sync_rp_force_aer_internal_uncorr_error((PDFWK_INTERFACE_HEADER)ctx->iface, ctx->rpss_idx, cmpl->rp_index);
}

void handle_tx_rekey_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    /* Note: We do not wait for the link-down event to occur, we simply issue the request and continue */
    send_sync_rp_tx_rekey((PDFWK_INTERFACE_HEADER)ctx->iface, ctx->rpss_idx, cmpl->rp_index);
}
void handle_rx_rekey_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl)
{
    /* Note: We do not wait for the link-down event to occur, we simply issue the request and continue */
    send_sync_rp_rx_rekey((PDFWK_INTERFACE_HEADER)ctx->iface, ctx->rpss_idx, cmpl->rp_index);
}
