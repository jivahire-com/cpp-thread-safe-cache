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
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_index, rpss_index) \
    (((rp_index & 0xFF) << 8) | (rpss_index & 0xFF))
#define GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val) (cb_val & 0xFF)
#define GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val)   ((cb_val >> 8) & 0xFF)
#define RP_LINK_TRAINING_TIMEOUT_TICKS             (1000)
#define RP_TIMER_ONE_SHOT                          (0)

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
    send_sync_rp_get_link_status((PDFWK_INTERFACE_HEADER)(ctx->iface), rpss_idx, rp_idx);
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
