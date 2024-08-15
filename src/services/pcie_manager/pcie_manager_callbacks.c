//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements callbacks which handle PCIe request completions send by the
 * PCIe driver.
 */

/*------------- Includes -----------------*/
#include "DfwkPtrTypes.h" // for PDFWK_ASYNC_REQUEST_HEADER
#include "pcie_dfwk.h"    // for pcie_async_request_t

#include <ErrorHandler.h>   // for FPFwErrorRaise
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <pcie_manager_i.h> // for rpss_req_completion_cb
#include <pciess_int.h>
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdio.h>            // for printf, NULL
#include <tx_api.h>           // for TX_NO_WAIT, TX_SUCCESS, tx_queue_send

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void rpss_req_completion_cb(PDFWK_ASYNC_REQUEST_HEADER req, void* ctx_ref)
{
    FPFW_RUNTIME_ASSERT(ctx_ref != NULL);

    pcie_manager_context_t* ctx = (pcie_manager_context_t*)ctx_ref;
    pcie_async_request_t* async_req = (pcie_async_request_t*)req;

    pciess_completion_request_t cmpl = {0};
    cmpl.rp_index = async_req->rp_index;
    cmpl.op = async_req->rp_op;
    cmpl.status = async_req->status;
    cmpl.async_data = async_req->async_data;

    // If LINK_UP is received, then we stop the timer
    if (async_req->async_data.data & (0x1 << PCIESS_RP_INT_LINK_UP))
    {
        printf("%s LINK_UP received \n", __func__);
        tx_timer_delete(&(ctx->expiry_timer[async_req->rp_index]));
    }
    /*
     * Because of how async requests are pended by the service, we should
     * never run out of worker queue space. In case it happens, it is indicative
     * of a stalled pcie worker thread or a separate error incurred when
     * processing the previous completion. In either of these cases, the current
     * completion has no way to proceed and must fail.
     */
    int status = tx_queue_send(&(ctx->work_queue), &(cmpl), TX_NO_WAIT);
    if (status != TX_SUCCESS)
    {
        printf("%s: PCIe RPSS: %d | RP Index: %d | tx_queue_send error - %d\n", __func__, async_req->rpss_index, cmpl.rp_index, status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }

    /*
     * Send new WAIT_FOR_EVENT async request to the Driver becasue we always have
     * MAX_PENDING_WAIT_FOR_EVENT_PER_RP pending requests
     */
    send_async_wait_for_event(ctx, async_req->rp_index, 1);
}
