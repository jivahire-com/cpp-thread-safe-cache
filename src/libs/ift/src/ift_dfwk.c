//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift_dfwk.c
 *  IFT driver framework related API definitions
 */

/*------------------------- Includes ------------------------*/

#define __NO_LARGE_ADDRMAP_TYPEDEFS__

#include "ift_fw.h"
#include "ift_i.h"

#include <DbgPrint.h>      // for FPFW_DBGPRINT_INFO
#include <DfwkClient.h>    // for DfwkClientInterfaceOpen
#include <DfwkCommon.h>    // for DfwkAsyncRequestSetCompletionRoutine
#include <DfwkDriver.h>    // for DfwkQueueInitialize, DfwkInterfaceInitialize
#include <DfwkHost.h>      // for DfwkDeviceInitialize
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <bug_check.h>     // for BUG_CHECK
#include <ift.h>           // for ift_setup_clocks
#include <scp_top_regs.h>  // for SCP_TOP_DBGR_CSR_ADDRESS
#include <silibs_status.h> // for silibs_status_t, SILIBS_SUCCESS

/*-------------------- Structure Definitions -----------------*/

/*------------------------- Typedefs -------------------------*/

/*--------------------- Static Declarations ------------------*/

/*--------------------- Global Declarations ------------------*/

/*--------------------- Static Declarations ------------------*/

static pift_interface_t s_ift_interface = NULL;

/*-------------------------- Defines -------------------------*/

/*---------------------- Static Functions --------------------*/

static void ift_dfwk_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    FPFW_UNUSED(context);

    /**
     * HSP die 1 does not has access to Flash 1 where IFT binaries reside
     * therefore SCP die 1 cannot request HSP die 1 to load IFT binaries.
     * Workaround here is SCP die 0 will copy the IFT binaries from
     * MSCP EXP RAM die 0 to MSCP EXP RAM die 1.
     *
     * This co-ordination is manage using ICC msg. SCP Die 0 will send a
     * mailbox message to SCP Die 1 once it has copied the IFT binaries
     * in MSCP EXP RAM die 1.
     */

    FPFW_DBGPRINT_INFO("IFT: Dispatching request of type %d\n", request->RequestType);

    switch (request->RequestType)
    {
    case IFT_MEM_TESTS_FW_LOAD_ASYNC:
    case IFT_CORE_TESTS_FW_LOAD_ASYNC:
        ift_load_fw_sos(request);
        break;
    case IFT_EXECUTE_FW_ASYNC:
        ift_execute_test(request);
        break;
    default:
        /* Unsupported request type. Complete immediately. */
        DfwkAsyncRequestComplete(request);
        break;
    }
}

static void ift_dfwk_test_complete_cb(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);
    FPFW_UNUSED(CompletionContext);
}

static pift_interface_t ift_dfwk_get_interface(void)
{
    BUG_ASSERT(s_ift_interface != NULL);

    return s_ift_interface;
}

/*---------------------- Global Functions --------------------*/

void ift_dfwk_set_interface(pift_interface_t p_interface)
{
    s_ift_interface = p_interface;
}

void ift_dfwk_device_initialize(pift_device_t device, PDFWK_SCHEDULE schedule)
{
    DfwkDeviceInitialize(&device->Header, schedule);

    // Initialize the IFT device's request queue.
    DfwkQueueInitialize(&device->Queue, &device->Header, ift_dfwk_dispatch, device, DfwkQueueType_SerializedDispatch);
}

void ift_dfwk_interface_initialize(pift_interface_t intf, pift_device_t device)
{
    // Initialize interface with device.
    intf->Device = device;

    // Synchronize API is not supported, so set it to NULL.
    DfwkInterfaceInitialize(&intf->Header, &intf->Device->Header, &intf->Device->Queue, NULL);

    // Open the interface to allow sending requests.
    // This is necessary to ensure that the interface is ready to process requests.
    DfwkClientInterfaceOpen(&intf->Header);
}

void ift_dfwk_send_async_req(ift_request_t* request, uint32_t request_type)
{
    // Create and send an asynchronous request.
    request->Header.RequestType = request_type;
    pift_interface_t p_interface = ift_dfwk_get_interface();

    DfwkAsyncRequestInitialize(&request->Header, sizeof(request->Header));
    DfwkAsyncRequestSetCompletionRoutine(&request->Header, ift_dfwk_test_complete_cb, NULL);

    DfwkInterfaceSendAsync(&p_interface->Header, &request->Header);
}
