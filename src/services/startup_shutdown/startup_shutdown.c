//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown.c
 * Implements the startup or shutdown driver interface.
 */

/*------------- Includes -----------------*/
#include "startup_shutdown.h"

#include "startup_shutdown_i.h"
#include "startup_shutdown_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <debug.h>
#include <fpfw_init.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SOS_MAX_SSI_REGISTRATIONS (32)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sos_service_context_t s_sos_ctx = {0};

/*------------- Functions ----------------*/

// asynchronous dispatch function
void sos_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    FPFW_UNUSED(context);

    switch (request->RequestType)
    {

    /* these are requests used to initiate start phases or a shutdown */
    case STARTUP_REQUEST_START_PHASE_ASYNC: {
        pstartup_start_phase_request_t p_startup_request = (pstartup_start_phase_request_t)request;
        sos_queue_start_phase(p_startup_request->boot_type, p_startup_request->stage, request);
    }
    break;
    case STARTUP_REQUEST_SHUTDOWN_ASYNC: {
        pstartup_shutdown_request_t p_shutdown_request = (pstartup_shutdown_request_t)request;
        sos_queue_shutdown(p_shutdown_request->shutdown_type, request);
    }
    break;

    /* boot and shutdown requests */
    /* these are here as test, this interface was registered as an SSI */
    case SSI_STARTUP_STAGE_START_ASYNC: {
        pssi_startup_notification_request_t ssi_request = (pssi_startup_notification_request_t)request;
        FPFW_UNUSED(ssi_request)
        SOS_LOG_TRACE("SSI stage %d start, boot type %d", ssi_request->stage, ssi_request->boot_type);
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(request);
    }
    break;
    case SSI_STARTUP_STAGE_COMPLETE_ASYNC: {
        pssi_startup_notification_request_t ssi_request = (pssi_startup_notification_request_t)request;
        FPFW_UNUSED(ssi_request)
        SOS_LOG_TRACE("SSI stage %d complete, boot type %d", ssi_request->stage, ssi_request->boot_type);
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(request);
    }
    break;
    case SSI_SHUTDOWN_QUIESCE_ASYNC: {
        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)request;
        FPFW_UNUSED(ssi_request)
        SOS_LOG_TRACE("SSI shutdown, shutdown type %d", ssi_request->shutdown_type);
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(request);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT((false));
        break;
    }
}

// synchronous dispatch function
int32_t sos_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    psos_interface_t p_interface = (psos_interface_t)p_request->OwningInterface;
    psos_device_t p_device = p_interface->p_device;

    switch (p_request->RequestType)
    {

    case STARTUP_REGISTER_SSI_SYNC: {
        pstartup_register_ssi_request p_sos_request = (pstartup_register_ssi_request)p_request;

        pstartup_ssi_registration_t p_registration = p_sos_request->p_registration;
        // all interfaces need to be opened
        DfwkInterfaceOpen(p_registration->p_ssi_interface, &p_device->header);
        // need to store the request structure to linked list for future use
        DfwkAsyncRequestInitialize((void*)&p_registration->ssi_request, sizeof(p_registration->ssi_request));
        // add registration to registrations list
        FpFwListInsertTail(&s_sos_ctx.ssi_registrations, &p_registration->list_entry);
        // generate a unique bitmask for this registration
        p_registration->interface_unique_flag = 1 << s_sos_ctx.registration_count;
        SOS_LOG_INFO("SSI registration - identifier assigned: %x", (unsigned)p_registration->interface_unique_flag);
        s_sos_ctx.registration_count++;
        FPFW_RUNTIME_ASSERT(s_sos_ctx.registration_count < SOS_MAX_SSI_REGISTRATIONS);
    }
    break;
    case STARTUP_RESET_TIMEOUT_SYNC: {
        pstartup_reset_timeout_request_t p_sos_request = (pstartup_reset_timeout_request_t)p_request;
        SOS_LOG_INFO("SSI timeout reset received, %lums", (long unsigned int)p_sos_request->timeout_ms);
        // TODO:  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1821526/
        // if timeout > current remaining timeout, update to value passed in.  useful for retries, etc.
    }
    break;
    /* Synchronous request to initiate a start phase (no callback when complete)*/
    case STARTUP_REQUEST_START_PHASE_SYNC: {
        pstartup_start_phase_request_t p_startup_request = (pstartup_start_phase_request_t)p_request;
        sos_queue_start_phase(p_startup_request->boot_type, p_startup_request->stage, NULL);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return 0; //(DFWK_SUCCESS);
}

void sos_interface_init(psos_device_t p_device, psos_interface_t p_interface, bool shared)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_interface != NULL);

    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, sos_dispatch_sync);
    p_interface->p_device = p_device;

    if (shared)
    {
        // open the interface for use by any driver
        int32_t status = DfwkInterfaceOpen(&p_interface->header, &p_device->header);
        FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
    }
}

void sos_init(psos_device_t p_device, PDFWK_SCHEDULE p_schedule)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_schedule != NULL);

    SOS_LOG_INFO("Startup/shutdown init");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, sos_dispatch, &p_device->header, DfwkQueueType_ImmediateDispatch);

    // initialize the registrations list
    FpFwListInitialize(&s_sos_ctx.ssi_registrations);

    // create thread, queue, and event flags for handling requests
    sos_thread_init(&s_sos_ctx);
    remote_die_sync_init();
}
