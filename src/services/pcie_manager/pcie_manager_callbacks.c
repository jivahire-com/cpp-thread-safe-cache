//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements callbacks which handle PCIe request completions send by the
 * PCIe driver.
 */

/*------------- Includes -----------------*/
#include <ErrorHandler.h>
#include <FpFwAssert.h>
#include <pcie_manager_i.h>
#include <scp_pcie_manager.h>
#include <stdint.h>
#include <stdio.h>
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
        FPFwErrorRaise(status);
    }
}
