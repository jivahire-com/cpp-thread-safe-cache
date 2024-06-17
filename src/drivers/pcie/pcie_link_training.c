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
#include <DfwkDriver.h>    // for PDFWK_ASYNC_REQUEST_HEADER, DfwkAsyncRequ...
#include <ErrorHandler.h>  // for FPFwErrorRaise
#include <FpFwAssert.h>    // for FPFW_RUNTIME_ASSERT
#include <pcie_dfwk.h>     // for pcie_async_request_t, pciess_device_inter...
#include <silibs_status.h> // for SILIBS_E_TIMEOUT
#include <stdio.h>         // for printf, NULL
#include <tx_api.h>        // for TX_AUTO_ACTIVATE, TX_SUCCESS, tx_timer_cr...

/*-- Symbolic Constant Macros (defines) --*/
#define RP_LINK_TRAINING_TIMEOUT_TICKS (2)
#define RP_TIMER_ONE_SHOT              (0)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void pcie_rp_timer_expiry_cb(unsigned long cb_ptr);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void pcie_rp_timer_expiry_cb(unsigned long cb_ptr)
{
    PDFWK_ASYNC_REQUEST_HEADER req = (PDFWK_ASYNC_REQUEST_HEADER)cb_ptr;
    pcie_async_request_t* r = (pcie_async_request_t*)req;

    /* Signal request completion */
    r->status = SILIBS_E_TIMEOUT;
    DfwkAsyncRequestComplete(req);
}

void begin_link_training(PDFWK_ASYNC_REQUEST_HEADER req)
{
    if (req == NULL)
    {
        printf("Received a NULL request, cannot initiate link training!\n");
        FPFW_RUNTIME_ASSERT(req != NULL);
        return;
    }

    pciess_device_interface_t* iface = (pciess_device_interface_t*)(req->OwningInterface);
    pciess_device_t* dev = (pciess_device_t*)(iface->dev);
    pcie_async_request_t* r = (pcie_async_request_t*)req;

    /* Call silibs API to begin link training for this RP here */

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
    tx_timer_delete(&(dev->per_rp_timer[r->rp_index]));
    int status = tx_timer_create(&(dev->per_rp_timer[r->rp_index]),
                                 "pcie_link_training_timer",
                                 pcie_rp_timer_expiry_cb,
                                 (unsigned long)req,
                                 RP_LINK_TRAINING_TIMEOUT_TICKS,
                                 RP_TIMER_ONE_SHOT,
                                 TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS)
    {
        printf("Failed to create link training timer! TX_STATUS: %d\n", status);
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
}
