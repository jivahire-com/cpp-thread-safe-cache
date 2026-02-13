//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements callbacks which handle PCIe request completions send by the
 * PCIe driver.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkPtrTypes.h>
#include <ErrorHandler.h>
#include <FpFwAssert.h>
#include <pcie_async_requests_i.h>
#include <pcie_dfwk.h>
#include <pcie_manager_i.h>
#include <pciess_int.h>
#include <scp_pcie_manager.h>
#include <string.h>
#include <tx_api.h>

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
    memcpy(&(cmpl.async_data), &(async_req->async_data), sizeof(pcie_async_data_t));

    /*
     * Free the async request that just completed and requeue it.
     * We cannot use this async request again in the service as it will
     * now be pended in the driver work queue for the next event.
     * If any data from the request is needed, copy it to the service level
     * completion request before this.
     */
    memset(async_req, 0, sizeof(pcie_async_request_t));
    send_async_rp_wait_for_event(ctx, async_req, cmpl.rp_index);
    async_req = NULL;

    /* If LINK_UP is received, then we stop the link training expiration timer */
    if (cmpl.async_data.int_mask & (0x1 << PCIESS_RP_INT_LINK_UP))
    {
        // FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: LINK_UP received!\n", ctx->rpss_idx, cmpl.rp_index);
        tx_timer_delete(&(ctx->expiry_timer[cmpl.rp_index]));
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
        FPFW_DBGPRINT_INFO("RPSS[%d] RP[%d]: tx_queue_send error - %d\n", ctx->rpss_idx, cmpl.rp_index, status); // Error  Condition: Should not encounter tx_queue_send failure in production.
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
}
