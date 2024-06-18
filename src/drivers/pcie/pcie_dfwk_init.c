//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#include "DfwkHost.h"

#include <DfwkDriver.h>
#include <FpFwAssert.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_rpss_init_i.h>
#include <silibs_status.h>
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
    silibs_status_t sts = SILIBS_SUCCESS;

    switch (r->header.RequestType)
    {
    case (INITIAL_CONFIG_REQUEST):
        printf("Begin initial RPSS: %1d programming!\n", r->rpss_index);
        sts = begin_rpss_init(incoming);
        break;
    case (PRE_RP_INIT_REQUEST):
        printf("Begin RPSS: %1d pre-rp ready programming!\n", r->rpss_index);
        sts = begin_rpss_pre_rp_ready_init(incoming);
        break;
    case (POST_RP_INIT_REQUEST):
        printf("Begin RPSS: %1d post-rp ready programming!\n", r->rpss_index);
        sts = begin_rpss_post_rp_ready_init(incoming);
        break;
    default:
        printf("Bad sync req type on RPSS: %d!\n", r->rpss_index);
        sts = SILIBS_E_PARAM;
        break;
    }
    r->status = sts;

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
