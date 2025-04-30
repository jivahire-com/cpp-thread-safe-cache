//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_ssi.c
 * Implements the startup/shutdown service interface to registered clients.
 */

/*------------- Includes -----------------*/

#include <DfwkCommon.h>
#include <FpFwAssert.h>
#include <startup_shutdown_ssi.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void ssi_startup_stage_start(PDFWK_INTERFACE_HEADER p_interface,
                             pssi_request_t p_request,
                             ssi_startup_stage_t stage,
                             ssi_startup_type_t boot_type,
                             DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                             void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->startup_notification.header.AllocatedSize >= sizeof(ssi_request_t));

    p_request->startup_notification.header.RequestType = SSI_STARTUP_STAGE_START_ASYNC;
    p_request->startup_notification.stage = stage;
    p_request->startup_notification.boot_type = boot_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->startup_notification.header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void ssi_startup_stage_complete(PDFWK_INTERFACE_HEADER p_interface,
                                pssi_request_t p_request,
                                ssi_startup_stage_t stage,
                                ssi_startup_type_t boot_type,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->startup_notification.header.AllocatedSize >= sizeof(ssi_request_t));

    p_request->startup_notification.header.RequestType = SSI_STARTUP_STAGE_COMPLETE_ASYNC;
    p_request->startup_notification.stage = stage;
    p_request->startup_notification.boot_type = boot_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->startup_notification.header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void ssi_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                  pssi_request_t p_request,
                  ssi_shutdown_type_t shutdown_type,
                  DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                  void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->shutdown_notification.header.AllocatedSize >= sizeof(ssi_request_t));

    p_request->shutdown_notification.header.RequestType = SSI_SHUTDOWN_ASYNC;
    p_request->shutdown_notification.shutdown_type = shutdown_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->startup_notification.header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void ssi_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                 pssi_request_t p_request,
                 ssi_shutdown_type_t shutdown_type,
                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                 void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->shutdown_notification.header.AllocatedSize >= sizeof(ssi_request_t));

    p_request->shutdown_notification.header.RequestType = SSI_QUIESCE_ASYNC;
    p_request->shutdown_notification.shutdown_type = shutdown_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->startup_notification.header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}