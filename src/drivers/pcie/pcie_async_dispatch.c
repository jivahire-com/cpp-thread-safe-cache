//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>
#include <DfwkPtrTypes.h>
#include <FpFwUtils.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <silibs_status.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void pcie_per_rp_dispatch(PDFWK_ASYNC_REQUEST_HEADER req, void* context)
{
    FPFW_UNUSED(context);

    pcie_async_request_t* r = (pcie_async_request_t*)req;

    switch (r->rp_op)
    {
    case INITIATE_LINK_TRAINING:
        begin_link_training(req);
        break;
    case WAIT_FOR_EVENT:
        r->status = SILIBS_SUCCESS;
        DfwkAsyncRequestComplete(req);
        break;
    default:
        printf("Received an unknown request for RPSS: %d | RP Index: %d!\n", r->rpss_index, r->rp_index);
        r->status = SILIBS_E_PARAM;
        DfwkAsyncRequestComplete(req);
        break;
    }
}

void pcie_default_dispatch(PDFWK_ASYNC_REQUEST_HEADER incoming, void* context)
{
    FPFW_UNUSED(context);

    pciess_device_interface_t* iface = (pciess_device_interface_t*)(incoming->OwningInterface);
    pciess_device_t* dev = (pciess_device_t*)(iface->dev);
    pcie_async_request_t* r = (pcie_async_request_t*)incoming;

    switch (r->rp_index)
    {
    /* Fall through for valid root port indices */
    case 0:
    case 1:
    case 2:
    case 3:
        DfwkQueueEnqueueRequest(&(dev->per_rp_queue[r->rp_index]), incoming);
        break;
    default:
        printf("Root port invalid for RPSS: %d | RP Index: %d!\n", r->rpss_index, r->rp_index);
        r->status = SILIBS_E_PARAM;
        DfwkAsyncRequestComplete(incoming);
        break;
    }
}
