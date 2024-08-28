//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_service.c
 * Implements the primary driver interface for ACCEL Interrupt Service.
 */

/*------------- Includes -----------------*/
#include "accel_intr_service.h"

#include <DfwkDriver.h> // for DfwkAsyncRequestComplete, DfwkInterfaceIniti...
#include <DfwkHost.h>   // for DfwkDeviceInitialize
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <stdbool.h>    // for false
#include <string.h>     // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Static Functions ----------------*/

static void accel_intr_service_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    DfwkAsyncRequestComplete(p_request);
}

static int32_t accel_intr_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {
    case ACCEL_INTR_SERVICE_FATAL_INTR_RECVD:
    // Placeholder to service `FATAL Interrupt` sync command
    case ACCEL_INTR_SERVICE_SDM_MSG_RECVD:
    // Placeholder to service `SDM MSG Interrupt` sync command
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return DFWK_SUCCESS;
}
/*------------- Functions ----------------*/

void accel_intr_service_interface_init(paccel_intr_service_t p_device, paccel_intr_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, accel_intr_service_dispatch_sync);
    p_interface->p_device = p_device;
}

void accel_intr_service_init(paccel_intr_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue,
                        &p_device->header,
                        accel_intr_service_dispatch_async,
                        &p_device->header,
                        DfwkQueueType_SerializedDispatch);
}
