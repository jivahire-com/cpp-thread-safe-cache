//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_client.c
 * Source file for Accel Interrupt Client Service (This provides functions to call from ISR)
 */

/*------------- Includes -----------------*/
#include "accel_intr_client.h"

#include "accel_intr_service_interface.h" // for accel_intr_service_cmd_con...

#include <DfwkClient.h>     // for DfwkAsyncRequestInititalize
#include <FPFwInterrupts.h> // for FPFwCoreInterruptEnableVector
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <accel_intr.h>     // for
#include <stddef.h>         // for NULL
#include <stdio.h>          // for printf

/*-- Symbolic Constant Macros (defines) --*/
static accel_intr_service_cmd_context_t accel_intr_service_cmd_context;
static bool async_req_in_progress = false;

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-------------- Functions ---------------*/

static void accel_intr_async_request_complete(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(p_completion_context);
    async_req_in_progress = false;
}

static void dispatch_accel_intr_async_request(ACCEL_ID accel_type,
                                              e_accel_intr_service_command_id_t command,
                                              DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine)
{
    int idx = ACCEL_INTR_SERVICE_GET_IDX_FROM_COMMAND(command);

    accel_intr_service_cmd_context.requests[accel_type][idx].accel_type = accel_type;

    accel_intr_service_cmd_context.requests[accel_type][idx].header.RequestType = command;

    DfwkAsyncRequestSetCompletionRoutine(&accel_intr_service_cmd_context.requests[accel_type][idx].header, CompletionRoutine, NULL);
    DfwkInterfaceSendAsync(&accel_intr_service_cmd_context.p_interface->header,
                           &accel_intr_service_cmd_context.requests[accel_type][idx].header);
}

void send_fatal_intr_async_request(ACCEL_ID accel_type)
{
    // async_req_in_progress is to ensure only one request is in flight at a time
    if (async_req_in_progress)
    {
        return;
    }

    // Since the request is for a fatal error, realistically the callback will never be called
    async_req_in_progress = true;
    dispatch_accel_intr_async_request(accel_type, ACCEL_INTR_SERVICE_FATAL_INTR_RECVD, accel_intr_async_request_complete);
}

void accel_intr_client_init(paccel_intr_service_interface_t p_interface)
{
    // Cache the Accel Client interface pointer
    accel_intr_service_cmd_context.p_interface = p_interface;

    // Open Driver Framework Interface for Accel Client, initialize async requests
    DfwkClientInterfaceOpen(&accel_intr_service_cmd_context.p_interface->header);

    for (ACCEL_ID accel_type = 0x0; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        for (int idx = ACCEL_INTR_SERVICE_GET_IDX_FROM_COMMAND(ACCEL_INTR_SERVICE_COMMANDS_START_ID);
             idx < ACCEL_INTR_SERVICE_GET_IDX_FROM_COMMAND(ACCEL_INTR_SERVICE_MAX_COMMAND);
             idx++)
        {
            DfwkAsyncRequestInitialize(&accel_intr_service_cmd_context.requests[accel_type][idx].header,
                                       sizeof(accel_intr_service_cmd_context.requests[accel_type][idx].header));
        }
    }
}
