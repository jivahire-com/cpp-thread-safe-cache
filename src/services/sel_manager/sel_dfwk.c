//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_dfwk.c
 * Implements the primary driver interface of sel manager.
 */

/*------------- Includes -----------------*/
#include "sel_i.h"

#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <bug_check.h>
#include <idsw_kng.h>
#include <sel.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
sel_svc_device_t g_sel_device;
sel_svc_interface_t g_sel_interface;

/*------------- Functions ----------------*/
static void sel_svc_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    FPFW_UNUSED(context);

    switch (request->RequestType)
    {
    case SEL_TRANSFER_ASYNC:
        static sel_event_record_t event_record = {};
        psel_svc_request_t sel_request = (psel_svc_request_t)request;

        if (sel_pop(&event_record))
        {
            KNG_STATUS status = sel_send_event_async(&event_record, request);
            if (KNG_FAILED(status))
            {
                FPFW_DBGPRINT_ERROR("SEL send event failed: status = 0x%08lx\n", status);
                sel_request->status = status;

                // Push back the event record to the head of the queue.
                sel_push_head(&event_record);

                // Failed to send. Complete request.
                DfwkAsyncRequestComplete(request);
            }
        }
        else
        {
            psel_svc_request_t sel_request = (psel_svc_request_t)request;
            sel_request->status = KNG_E_SEL_EMPTY;

            // Flushed all. Complete request.
            DfwkAsyncRequestComplete(request);
        }
        break;
    default:
        // Unsupported request type. Complete immediately.
        DfwkAsyncRequestComplete(request);
        break;
    }
}

void sel_init_dfwk_device(PDFWK_SCHEDULE schedule)
{
    DfwkDeviceInitialize(&g_sel_device.Header, schedule);

    // Initialize crash dump driver request queue.
    DfwkQueueInitialize(&g_sel_device.Queue, &g_sel_device.Header, sel_svc_dispatch, &g_sel_device, DfwkQueueType_SerializedDispatch);
}

void sel_init_dfwk_interface()
{
    // Initialize interface with device.
    g_sel_interface.Device = &g_sel_device;
    DfwkInterfaceInitialize(&g_sel_interface.Header, &g_sel_interface.Device->Header, &g_sel_interface.Device->Queue, NULL); // Synchonous request is not supported.

    // Open Interface
    int32_t status = DfwkInterfaceOpen(&g_sel_interface.Header, &g_sel_device.Header);
    BUG_ASSERT_PARAM(DFWK_SUCCEEDED(status), status, 0);
}

static void sel_transfer_req_cb(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);

    sel_svc_request_t* sel_request = (sel_svc_request_t*)Request;
    sel_request->is_completed = true;

    switch (sel_request->status)
    {
    case KNG_SUCCESS:
        // Successfully transferred. Try to flush next item.
        sel_flush_queue();
        break;
    case KNG_E_SEL_EMPTY:
        // Queue is empty. Nothing to do.
        break;
    default:
        // Failed to transfer
        FPFW_DBGPRINT_ERROR("SEL transfer failed: status = 0x%08lx\n", sel_request->status);
        break;
    }
}

static bool sel_is_ready_to_flush()
{
    bool ret = false;

    if (idsw_get_die_id() == DIE_0) // DIE 0
    {
        if (idsw_get_cpu_type() == CPU_MCP)
        {
            // Flush event to the BMC if PLDM is ready
            ret = sel_is_PLDM_ready();
        }
        else
        {
            // SCP0 needs ICC MSCP2MSCP to relay this request to the MCP0
            ret = !!sel_icc_ctx(SEL_ICC_MSCP2MSCP);
        }
    }
    else // DIE 1
    {
        // MCP1 and SCP1 needs ICC DIE2DIE to relay this request to the Primary core
        ret = !!sel_icc_ctx(SEL_ICC_DIE2DIE);
    }

    return ret;
}

/**
 * @brief Flush SEL event queue
 *
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_flush_queue()
{
    static sel_svc_request_t request = {.status = KNG_SUCCESS, .is_completed = true};

    if (sel_is_ready_to_flush() == false || !request.is_completed)
    {
        FPFW_DBGPRINT_WARNING("SEL MGR not ready to flush\n");
        return KNG_E_NOT_READY;
    }

    if (request.Header.AllocatedSize != sizeof(sel_svc_request_t))
    {
        // Request is not properly initialized. Initialize the request.
        DfwkAsyncRequestInitialize(&request.Header, sizeof(sel_svc_request_t));
    }

    // Configure the request.
    request.Header.RequestType = SEL_TRANSFER_ASYNC;
    request.status = KNG_SUCCESS;
    DfwkAsyncRequestSetCompletionRoutine(&request.Header, sel_transfer_req_cb, NULL);

    // Send the request to DFWK.
    request.is_completed = false;
    DfwkInterfaceSendAsync(&g_sel_interface.Header, &request.Header);

    return KNG_SUCCESS;
}