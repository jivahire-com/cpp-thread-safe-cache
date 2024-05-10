//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main.c
 * Implements the primary driver interface.
 */

/*------------- Includes -----------------*/

#include <DfwkDriver.h> // for DfwkInterfaceInitialize, DfwkQueueInitia...
#include <DfwkHost.h>   // for DfwkDeviceInitialize
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <power_dfwk.h> // for ppower_service_t, ppower_service_interfa...
#include <power_init.h> // for power_init, power_interface_init
#include <stdbool.h>    // for false
#include <stdint.h>     // for int32_t
#include <stdio.h>      // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void power_service_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static int32_t power_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return DFWK_SUCCESS;
}

void power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, power_service_dispatch_sync);
    p_interface->p_device = p_device;
}

void power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    printf("\npower_init\n");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, power_service_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);
}
