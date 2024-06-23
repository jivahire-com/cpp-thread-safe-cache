//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_main.c
 * Implements the primary driver interface of sds service.
 */

/*------------- Includes -----------------*/
#include "sds_api.h"
#include "sds_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void sds_service_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    default:
        SDS_LOG_WARN("SDS service doesn't support async request");
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static int32_t sds_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {
    case SDS_IO_REQUEST_READ_SYNC: {
        // TO DO: Add request data
        SDS_LOG_INFO("SDS_READ Request Received");
        break;
    }
    case SDS_IO_REQUEST_WRITE_SYNC: {
        // TO DO: Add request data
        SDS_LOG_INFO("SDS_write Request Received");
        break;
    }
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    return DFWK_SUCCESS;
}

void sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, sds_service_dispatch_sync);
    p_interface->p_device = p_device;
}

void sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    SDS_LOG_INFO("SDS Service Initialize Starting...");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, sds_service_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);
}