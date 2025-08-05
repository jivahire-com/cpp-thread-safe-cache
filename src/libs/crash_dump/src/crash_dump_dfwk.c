/**
 * @file crash_dump_dfwk.c
 * Implement driver framework interface for crash dump
 */

/*------------- Includes -----------------*/
#include "crash_dump_pldm.h"
#include "crash_dump_status.h"

#include <DfwkClient.h>
#include <DfwkHost.h>
#include <bug_check.h>
#include <crash_dump.h>
#include <crash_dump_dfwk.h>
#include <startup_shutdown_ssi.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SLEEP_TICKS (10)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static TX_TIMER cd_quiesce_timer;

/*------------- Functions ----------------*/
static void crash_dump_quiesce_callback(ULONG input)
{
    pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)input;

    // Wait until dump status is not CRASH_DUMP_IN_TRANSFER and
    // all core states are CRASH_DUMP_STATE_NOT_AVAILABLE or CRASH_DUMP_STATE_READY.
    if (!crash_dump_get_is_dump_transferring(NULL) && crash_dump_get_is_dump_complete(NULL))
    {
        UINT ret = tx_timer_deactivate(&cd_quiesce_timer);
        BUG_ASSERT(ret == TX_SUCCESS);

        DfwkAsyncRequestComplete(&ssi_request->header);
    }
}

static void crash_dump_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    pcrash_dump_device_t dev = (pcrash_dump_device_t)context;
    (void)dev;

    switch (request->RequestType)
    {
    case SSI_QUIESCE_ASYNC: {
        UINT ret = tx_timer_create(&cd_quiesce_timer,           // timer_ptr
                                   "cd_quiesce",                // name_ptr
                                   crash_dump_quiesce_callback, // expiration_function
                                   (ULONG)request,              // input
                                   SLEEP_TICKS,                 // initial_ticks
                                   SLEEP_TICKS,                 // reschedule_ticks
                                   TX_AUTO_ACTIVATE);           // auto_activate
        BUG_ASSERT(ret == TX_SUCCESS);
    }
    break;
    case CRASH_DUMP_START_TRANSFER_ASYNC: {
        pcrash_dump_request_t cd_request = (pcrash_dump_request_t)request;
        cd_request->status = crash_dump_pldm_transfer_dump();

        DfwkAsyncRequestComplete(request);
    }
    break;
    default:
        // Unsupported request type. Complete immediately.
        DfwkAsyncRequestComplete(request);
        break;
    }
}

void crash_dump_device_initialize(pcrash_dump_device_t device, PDFWK_SCHEDULE schedule)
{
    DfwkDeviceInitialize(&device->Header, schedule);

    // Initialize crash dump driver request queue.
    DfwkQueueInitialize(&device->Queue, &device->Header, crash_dump_dispatch, device, DfwkQueueType_SerializedDispatch);
}

void crash_dump_interface_initialize(pcrash_dump_interface_t intf, pcrash_dump_device_t device)
{
    // Initialize interface with device.
    intf->Device = device;
    DfwkInterfaceInitialize(&intf->Header, &intf->Device->Header, &intf->Device->Queue, NULL); // Synchonous request is not supported.
}

uint32_t crash_dump_start_transfer_async(pcrash_dump_interface_t iface,
                                         pcrash_dump_request_t request,
                                         DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                         void* context)
{
    // Validate input arguments.
    if (iface == NULL || request == NULL || callback == NULL)
    {
        return KNG_E_INVALIDARG;
    }

    if (iface->Device == NULL)
    {
        // Crash dump interface is not initialized properly.
        return KNG_E_HANDLE;
    }

    if (request->Header.AllocatedSize != sizeof(crash_dump_request_t))
    {
        // Request is not properly initialized. Initialize the request.
        DfwkAsyncRequestInitialize(&request->Header, sizeof(crash_dump_request_t));
    }

    // Configure the request.
    request->Header.RequestType = CRASH_DUMP_START_TRANSFER_ASYNC;
    DfwkAsyncRequestSetCompletionRoutine(&request->Header, callback, context);

    // Send the request to Crash dump driver.
    DfwkInterfaceSendAsync(&iface->Header, &request->Header);

    return KNG_SUCCESS;
}