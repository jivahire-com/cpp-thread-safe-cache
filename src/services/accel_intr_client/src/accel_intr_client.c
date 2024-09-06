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

#include <DfwkClient.h> // for DfwkAsyncRequestInitialize
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <accel_intr.h> // for accel_intr_handle_fatal_intr_recvd, accel_in...
#include <stddef.h>     // for NULL

/*-- Symbolic Constant Macros (defines) --*/
static accel_intr_service_cmd_context_t accel_intr_service_cmd_context;

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-------------- Functions ---------------*/
/**
 * Moving actual interrupt ptocessing to the callback function here.
 * Keeping this before DfwkAsyncRequestComplete was causing a crash.
 * This needs to be reviewed and modifie if necessary.
 * TODO: Task 2000244: [SCP] Accel IP Fatal Interrupt DWFK Usage
 */
static void accel_intr_async_request_complete(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);

    switch (p_request->RequestType)
    {
    /**
     * Handle Accel IP FATAL interrupt
     */
    case ACCEL_INTR_SERVICE_FATAL_INTR_RECVD: {
        paccel_intr_service_request_t p_accel_intr_service_request = (paccel_intr_service_request_t)p_request;

        // Call handler
        accel_intr_handle_fatal_intr_recvd(p_accel_intr_service_request->IRQnum);
    }
    break;

    /**
     * Handle Accel IP SDM_MSG interrupt
     */
    case ACCEL_INTR_SERVICE_SDM_MSG_RECVD: {
        paccel_intr_service_request_t p_accel_intr_service_request = (paccel_intr_service_request_t)p_request;

        // Call handler
        accel_intr_handle_sdm_msg_recvd(p_accel_intr_service_request->IRQnum);
    }
    break;

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static void dispatch_accel_intr_async_request(uint32_t IRQnum,
                                              e_accel_intr_service_command_id_t command,
                                              DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine)
{
    accel_intr_service_cmd_context.request.IRQnum = IRQnum;

    accel_intr_service_cmd_context.request.header.RequestType = command;

    DfwkAsyncRequestSetCompletionRoutine(&accel_intr_service_cmd_context.request.header, CompletionRoutine, NULL);
    DfwkInterfaceSendAsync(&accel_intr_service_cmd_context.p_interface->header,
                           &accel_intr_service_cmd_context.request.header);
}

void send_sdm_msg_async_request(uint32_t IRQnum)
{
    dispatch_accel_intr_async_request(IRQnum, ACCEL_INTR_SERVICE_SDM_MSG_RECVD, accel_intr_async_request_complete);
}

void send_fatal_intr_async_request(uint32_t IRQnum)
{
    dispatch_accel_intr_async_request(IRQnum, ACCEL_INTR_SERVICE_FATAL_INTR_RECVD, accel_intr_async_request_complete);
}

void accel_intr_client_init(paccel_intr_service_interface_t p_interface)
{
    // Cache the power interface pointer
    accel_intr_service_cmd_context.p_interface = p_interface;

    // Open Driver Framework Interface for Power CLI, initialize async requests
    DfwkClientInterfaceOpen(&accel_intr_service_cmd_context.p_interface->header);
    DfwkAsyncRequestInitialize(&accel_intr_service_cmd_context.request.header,
                                sizeof(accel_intr_service_cmd_context.request.header));
}
