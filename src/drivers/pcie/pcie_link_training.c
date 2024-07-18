//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This file implements wrappers for the silicon libs PCIe link training APIs.
 * Additionally it also handlers for cases where a root port link fails to
 * train.
 */

/*------------- Includes -----------------*/
#include "pcie_dfwk_i.h"

#include <DfwkDriver.h>
#include <ErrorHandler.h>
#include <FpFwAssert.h>
#include <pcie_dfwk.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define RP_LINK_TRAINING_TIMEOUT_TICKS (10)
#define RP_TIMER_ONE_SHOT              (0)
#define SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(rp_index, rpss_index) \
    (((rp_index & 0xFF) << 8) | (rpss_index & 0xFF))
#define GET_RPSS_INDEX_FROM_TIMER_CALLBACK(cb_val) (cb_val & 0xFF)
#define GET_RP_INDEX_FROM_TIMER_CALLBACK(cb_val)   ((cb_val >> 8) & 0xFF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void pcie_rp_timer_expiry_callback(unsigned long cb_val);

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

    pcie_async_request_t* req = get_pending_async_req_for_this_rp(rpss_idx, rp_idx, INITIATE_LINK_TRAINING);
    if (req == NULL)
    {
        return;
    }

    /*
     * Check this here irrespectively just in case link training is done when
     * the timer expires.
     */
    pcie_ss_entity_t* rpss = pciess_get_entity(req->rpss_index);
    req->status = pciess_rp_get_link_train_done(&(rpss->rps[req->rp_index]));

    /* Signal request completion */
    complete_async_req_for_this_rp(req);
}

void begin_link_training(PDFWK_ASYNC_REQUEST_HEADER req)
{
    if (req == NULL)
    {
        printf("Received a NULL request, cannot initiate link training!\n");
        FPFW_RUNTIME_ASSERT(req != NULL);
        return;
    }

    pcie_async_request_t* r = (pcie_async_request_t*)req;
    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);

    /* Do not start timers for disabled root ports, complete the request immediately */
    if (rpss->rps[r->rp_index].enabled == false)
    {
        r->status = SILIBS_E_SUPPORT;
        complete_async_req_for_this_rp(r);
        return;
    }

    /*
     * timer_ctx will be used from within the expiration timer
     * callback to infer the rpss and rp combination to which the timer belongs
     */
    unsigned long timer_ctx = SET_RPSS_AND_RP_INDEX_FOR_TIMER_CALLBACK(r->rp_index, r->rpss_index);
    pciess_rp_initiate_link_training(&(rpss->rps[r->rp_index]));

    /*
     * Create and start a one shot timer to handle cases where a root port fails
     * to train. The timer will be deactivated when we receive a link training
     * done interrupt on that root port OR when link training times out.
     *
     * NOTE:
     * Assume that a timer was previously created and call delete before
     * creating a new timer - this handles cases where link re-training is
     * necessary.
     */
    tx_timer_delete(&(r->expiry_timer));
    unsigned int status = tx_timer_create(&(r->expiry_timer),
                                          "pcie_link_training_timer",
                                          pcie_rp_timer_expiry_callback,
                                          timer_ctx,
                                          RP_LINK_TRAINING_TIMEOUT_TICKS,
                                          RP_TIMER_ONE_SHOT,
                                          TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS)
    {
        printf("Failed to create link training timer! TX_STATUS: %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
}

void pcie_link_up_interrupt_callback(pcie_ss_entity_t* ss, uint8_t rp_idx)
{
    pcie_async_request_t* req = get_pending_async_req_for_this_rp(ss->id, rp_idx, INITIATE_LINK_TRAINING);
    if (req == NULL)
    {
        return;
    }

    req->status = pciess_rp_get_link_train_done(&(ss->rps[rp_idx]));
    if (req->status == SILIBS_SUCCESS)
    {
        tx_timer_deactivate(&(req->expiry_timer));
        complete_async_req_for_this_rp(req);
    }
}
