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

/*-- Symbolic Constant Macros (defines) --*/
static accel_intr_service_cmd_context_t accel_intr_service_cmd_context;

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-------------- Functions ---------------*/

static void accel_intr_async_request_complete(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);

    paccel_intr_service_request_t p_accel_intr_service_request = (paccel_intr_service_request_t)p_request;

    /**
     * Enable Interrupt was earlier being done as part of request handler.
     * With that implementation there is a possibility that new interrupt can be triggered before first one is
     * processed. As per DFWK recommendation, enabling only after complete processing is done.
     */
    if (p_request->RequestType == ACCEL_INTR_SERVICE_FATAL_INTR_RECVD)
    {
        FPFwCoreInterruptEnableVector(p_accel_intr_service_request->IRQnum);
    }
}

static void dispatch_accel_intr_async_request(uint32_t IRQnum,
                                              e_accel_intr_service_command_id_t command,
                                              DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine)
{
    int idx = ACCEL_INTR_SERVICE_GET_IDX_FROM_COMMAND(command);
    eACCELERATOR_TYPE accel_type = accel_intr_get_accel_type_from_irq_num(IRQnum);

    accel_intr_service_cmd_context.requests[accel_type][idx].IRQnum = IRQnum;

    accel_intr_service_cmd_context.requests[accel_type][idx].header.RequestType = command;

    DfwkAsyncRequestSetCompletionRoutine(&accel_intr_service_cmd_context.requests[accel_type][idx].header, CompletionRoutine, NULL);
    DfwkInterfaceSendAsync(&accel_intr_service_cmd_context.p_interface->header,
                           &accel_intr_service_cmd_context.requests[accel_type][idx].header);
}

void send_sdm_msg_async_request(uint32_t IRQnum)
{
    dispatch_accel_intr_async_request(IRQnum, ACCEL_INTR_SERVICE_SDM_MSG_RECVD, accel_intr_async_request_complete);
}

void send_fatal_intr_async_request(uint32_t IRQnum)
{
    dispatch_accel_intr_async_request(IRQnum, ACCEL_INTR_SERVICE_FATAL_INTR_RECVD, accel_intr_async_request_complete);
}

void send_mailbox_async_request(uint32_t IRQnum)
{
    dispatch_accel_intr_async_request(IRQnum, ACCEL_INTR_SERVICE_MBOX_RECVD, accel_intr_async_request_complete);
}

void accel_intr_client_init(paccel_intr_service_interface_t p_interface)
{
    // Cache the Accel Client interface pointer
    accel_intr_service_cmd_context.p_interface = p_interface;

    // Open Driver Framework Interface for Accel Client, initialize async requests
    DfwkClientInterfaceOpen(&accel_intr_service_cmd_context.p_interface->header);

    for (eACCELERATOR_TYPE accel_type = 0x0; accel_type < MAX_ACCELERATOR_TYPES; accel_type++)
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
