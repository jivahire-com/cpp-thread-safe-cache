//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>
#include <DfwkPtrTypes.h>
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int32_t pcie_sched_sync_op(PDFWK_SYNC_REQUEST_HEADER incoming)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)incoming;

    printf("Received synchronous request on RPSS: %d!\n", r->rpss_index);
    r->status = SILIBS_SUCCESS;

    return 0;
}

void pcie_dfwk_interface_init(pciess_device_t* dev, pciess_device_interface_t* iface)
{
    FPFW_RUNTIME_ASSERT(dev != NULL);
    FPFW_RUNTIME_ASSERT(iface != NULL);

    DfwkInterfaceInitialize(&iface->header, &dev->header, &dev->default_queue, pcie_sched_sync_op);
    iface->dev = dev;
}

void pcie_dfwk_init(pciess_device_t* dev, PDFWK_SCHEDULE schedule)
{
    FPFW_RUNTIME_ASSERT(dev != NULL);
    FPFW_RUNTIME_ASSERT(schedule != NULL);

    DfwkDeviceInitialize(&(dev->header), schedule);
    DfwkQueueInitialize(&(dev->default_queue), &(dev->header), pcie_default_dispatch, NULL, DfwkQueueType_ImmediateDispatch);

    for (uint8_t i = 0; i < ROOT_PORTS_PER_RPSS; i++)
    {
        DfwkQueueInitialize(&(dev->per_rp_queue[i]), &(dev->header), pcie_per_rp_dispatch, NULL, DfwkQueueType_SerializedDispatch);
    }
}
