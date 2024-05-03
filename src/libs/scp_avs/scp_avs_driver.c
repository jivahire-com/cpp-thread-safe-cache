//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_driver.c
 * This file contains the implementation of the AVS module driver
 * interface and related functionality.
 */

/*------------- Includes -----------------*/

#include "DfwkHost.h"  // for DfwkDeviceInitialize
#include "FpFwUtils.h" // for FPFW_UNUSED

#include <DfwkDriver.h>     // for PDFWK_ASYNC_REQUEST_HEADER, _DFWK_ASYNC_...
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <scp_avs.h>        // for avs_internal_request_type_idx, avs_sync_...
#include <scp_avs_driver.h> // for pscp_avs_device, pscp_avs_interface, psc...
#include <stdbool.h>        // for false
#include <stdint.h>         // for int32_t, uint8_t
#include <stdio.h>          // for printf, fflush, stdout

/*-- Symbolic Constant Macros (defines) --*/

#define UNUSED(x) (void)(x)

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

/*
 * An IRQ is triggered if the transaction has been completed successfully or
 * if the transaction has been aborted.
 */
void scp_avs_isr(void* context)
{ // TODO (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1745551) Update once the interrupt library is in place and working.

    pscp_avs_device device = (pscp_avs_device)context;
    pscp_avs_request avs_request = device->outstanding_request;

    // TODO (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484968) check for errors, and add them to the
    // copy client buffer in real life pscp_avs_request, the client provided a buffer to plop the results in.

    DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)(avs_request));
}

void scp_avs_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    pscp_avs_interface Interface = (pscp_avs_interface)Request->OwningInterface;
    pscp_avs_device device = Interface->Device;               // Device associated with this request
    pscp_avs_request avs_request = (pscp_avs_request)Request; // this has the cmd_type, rail, etc.
    FPFW_UNUSED(Context);

    printf("\n scp_avs_dispatch, RequestType = %x\n", (uint8_t)Request->RequestType);
    fflush(stdout);

    switch (Request->RequestType)
    {
    case AVS_REQUEST_READ_DATA:
        device->outstanding_request = avs_request;
        // This is where the actual read to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_WRITE_DATA:
        device->outstanding_request = avs_request;
        // This is where the actual write to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_READ_ALL_VCT:
        device->outstanding_request = avs_request;
        // This is where the actual read all to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_READ_MULTI:
        device->outstanding_request = avs_request;
        // This is where the actual read multi to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    case AVS_REQUEST_WRITE_MULTI:
        device->outstanding_request = avs_request;
        // This is where the actual write multi to the AVS goes.
        // int avs_send_cmd_frame(uint32_t avs_id, uint32_t cmd_num, avs_master_command_t *cmd_mem)
        break;

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

int32_t scp_avs_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request)
{
    switch (Request->RequestType)
    {
    case AVS_GET_ERROR_COUNTS:
        // TODO: (https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484968) Get error counts for the client.
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    printf("\n scp_avs_dispatch_sync\n");
    return 0; //(DFWK_SUCCESS);
}

void scp_avs_driver_initialize(pscp_avs_device Device)
{
    printf("\nAVS init, avs_bus_num =  %x \n", Device->avs_bus_num);
    fflush(stdout);

    // Set up the queue for each driver, based on the driver config. Any event that is put on the queue will call scp_avs_dispatch.
    DfwkQueueInitialize(&Device->avs_queue, &Device->Header, scp_avs_dispatch, &Device->Header, DfwkQueueType_SerializedDispatch);
}

void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface Interface)
{
    DfwkInterfaceInitialize(&Interface->Header, &Device->Header, &Device->avs_queue, scp_avs_dispatch_sync);
    Interface->Device = Device;
    Interface->Device->avs_response_errors = 0;
}

// Completion routine for an async request.
void scp_request_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);
    FPFW_UNUSED(Request);
}

void scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler)
{
    DfwkDeviceInitialize(&avsDevice->Header, scheduler);
    scp_avs_driver_initialize(avsDevice);
}