//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_client_interface.c
 * Implements the startup/shutdown service client interface.
 */

/*------------- Includes -----------------*/

#include <DfwkCommon.h> // for DfwkInterfaceSendSync, DfwkAsyncRe...
#include <DfwkStatus.h>
#include <FpFwAssert.h>       // for FPFW_RUNTIME_ASSERT
#include <startup_shutdown.h> // for startup_start_phase_request_t, _st...
#include <startup_shutdown_events_i.h>
#include <startup_shutdown_ssi.h> // for ssi_shutdown_type_t, ssi_startup_s...
#include <stddef.h>               // for NULL
#include <stdint.h>               // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int32_t sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface, pstartup_ssi_registration_t p_registration, PDFWK_INTERFACE_HEADER p_ssi_interface)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_registration != NULL);
    FPFW_RUNTIME_ASSERT(p_ssi_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_ssi_interface->DispatchQueue != NULL); // must be a valid async interface
    startup_register_ssi_request request;
    request.header.RequestType = STARTUP_REGISTER_SSI_SYNC;
    request.p_registration = p_registration;
    // set the registered interface to the one passed in
    p_registration->p_ssi_interface = p_ssi_interface;
    return DfwkInterfaceSendSync(p_interface, &request.header);
}

int32_t sos_reset_timeout(PDFWK_INTERFACE_HEADER p_interface, sos_stage_timeout_t timeout)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    startup_reset_timeout_request_t request;
    request.header.RequestType = STARTUP_RESET_TIMEOUT_SYNC;
    request.timeout = timeout;
    return DfwkInterfaceSendSync(p_interface, &request.header);
}

void sos_start_phase(PDFWK_INTERFACE_HEADER p_interface,
                     pstartup_start_phase_request_t p_request,
                     ssi_startup_type_t boot_type,
                     ssi_startup_stage_t startup_stage,
                     DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                     void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);

    SOS_ET_BOOT_PROFILE_INFO(BOOT_STAGE, boot_type, SOS_ET_TYPE_BOOT_OPERATION_START);

    // synchronous version
    if (p_request == NULL)
    {
        startup_start_phase_request_t sync_request;
        sync_request.header.sync.RequestType = STARTUP_REQUEST_START_PHASE_SYNC;
        sync_request.stage = startup_stage;
        sync_request.boot_type = boot_type;
        int status = DfwkInterfaceSendSync(p_interface, &sync_request.header.sync);
        FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
        return;
    }
    // asynchronous version
    FPFW_RUNTIME_ASSERT(p_request->header.async.AllocatedSize >= sizeof(startup_start_phase_request_t));

    p_request->header.async.RequestType = STARTUP_REQUEST_START_PHASE_ASYNC;
    p_request->stage = startup_stage;
    p_request->boot_type = boot_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->header.async, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void sos_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                  pstartup_shutdown_request_t p_request,
                  ssi_shutdown_type_t shutdown_type,
                  DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                  void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->header.AllocatedSize >= sizeof(startup_shutdown_request_t));

    SOS_ET_BOOT_PROFILE_INFO(SHUTDOWN_STAGE, shutdown_type, SOS_ET_TYPE_BOOT_OPERATION_START);

    p_request->header.RequestType = STARTUP_REQUEST_SHUTDOWN_ASYNC;
    p_request->shutdown_type = shutdown_type;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->header, completion_routine, p_completion_context);

    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void sos_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                 pstartup_shutdown_request_t p_request,
                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                 void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->header.AllocatedSize >= sizeof(startup_shutdown_request_t));
    FPFW_RUNTIME_ASSERT(p_request->header.RequestType == STARTUP_REQUEST_QUIESCE_ASYNC);

    SOS_ET_BOOT_PROFILE_INFO(QUIESCE_STAGE, p_request->shutdown_type, SOS_ET_TYPE_BOOT_OPERATION_START);

    DfwkAsyncRequestSetCompletionRoutine(&p_request->header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}