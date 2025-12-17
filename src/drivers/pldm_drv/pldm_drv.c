//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file pldm_drv.c
 *  PLDM driver public API implementation.
 */

/*--------------- Includes ---------------*/
#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <pldm_drv.h>
#include <pldm_events.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static pldm_device_t pldm_device = {.is_ready = false, .has_pending_request = false};
static pldm_interface_t pldm_interface = {};

/*------------- Functions ----------------*/
/**
 * @brief PLDM platform event ready notification callback
 *        This function is called when the PLDM platform event is ready.
 *        It will complete the original request to invoke client ready callback.
 */
static void pldm_platform_event_ready_callback(uint16_t event_id, void* context)
{
    FPFW_UNUSED(event_id);
    FPFW_UNUSED(context);

    pldm_device.is_ready = true;

    PDFWK_ASYNC_REQUEST_HEADER pending_request = NULL;
    while (DfwkQueueDequeueRequest(&pldm_device.ready_queue, &pending_request))
    {
        pldm_request_t* pldm_request = (pldm_request_t*)pending_request;
        pldm_request->status = FPFW_STATUS_SUCCESS;

        DfwkAsyncRequestComplete(pending_request);
    }
}

/**
 * @brief Register PLDM platform event ready notification
 *
 * @param request PLDM request object
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_drv_register_platform_event_ready_notification(pldm_request_t* request)
{
    if (request->header.AllocatedSize != sizeof(pldm_request_t))
    {
        // Request is not properly initialized. Initialize the request.
        DfwkAsyncRequestInitialize(&request->header, sizeof(pldm_request_t));
    }

    // Configure the request.
    request->header.RequestType = PLDM_GET_READY_ASYNC;
    request->status = FPFW_STATUS_SUCCESS;
    request->pe_config = NULL;
    request->pe_notification = NULL;

    DfwkInterfaceSendAsync(&pldm_interface.header, &request->header);

    return FPFW_STATUS_SUCCESS;
}

/**
 * @brief PLDM platform event ready notification callback
 *        This function is called when the PLDM platform event is ready.
 *        It will complete the original request to invoke client raise platform event callback.
 */
static void pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* ctx)
{
    pldm_request_t* pldm_request = (pldm_request_t*)ctx;
    pldm_request->completion_code = completionCode;

    // Complete the original request
    DfwkAsyncRequestComplete(&pldm_request->header);
}

/**
 * @brief Raise PLDM platform event
 *
 * @param request PLDM request object
 */
static void pldm_platform_event_async(PDFWK_ASYNC_REQUEST_HEADER request)
{
    pldm_device.has_pending_request = true;

    pldm_request_t* pldm_request = (pldm_request_t*)request;

    static pldm_platform_event_notification notification = {.CallBack = pldm_on_ppe_complete};
    notification.context = request;

    pldm_request->status = fpfw_pldm_service_raise_platform_event(pldm_request->pe_config, &notification);

    if (FPFW_STATUS_FAILED(pldm_request->status))
    {
        PLDM_ET_ERROR_PARAM(PLDM_ET_TYPE_RAISE_PLATFORM_EVENT_FAILED, pldm_request->status);
        FPFW_DBGPRINT_ERROR("PLDM: Failed to raise platform event: status = 0x%08lx\n", pldm_request->status);
        pldm_device.has_pending_request = false;

        // Complete the request with failure
        DfwkAsyncRequestComplete(&pldm_request->header);
    }
}

/**
 * @brief PLDM_SEND_PLATFORM_EVENT_ASYNC request completion callback.
 *        Request will be completed when PLDM platform event is completed (pldm_on_ppe_complete)
 *        or when there is an error raising the platform event.
 */
static void pldm_req_cb(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);

    pldm_request_t* pldm_request = (pldm_request_t*)Request;

    if (pldm_request->pe_notification && pldm_request->pe_notification->CallBack)
    {
        // Call the user callback
        pldm_request->pe_notification->CallBack(pldm_request->completion_code, pldm_request->pe_notification->context);
    }

    // Mark that there is no pending request
    pldm_device.has_pending_request = false;

    // Check if there are any queued service requests
    PDFWK_ASYNC_REQUEST_HEADER next_request = NULL;
    if (DfwkQueueDequeueRequest(&pldm_device.svc_queue, &next_request))
    {
        // Dispatch the next request
        DfwkQueueEnqueueRequest(&pldm_device.dispatch_queue, next_request);
    }
}

/**
 * @brief Raise PLDM platform event
 *
 * @param request PLDM request object
 * @param pe_config Platform event configuration
 * @param pe_notification Platform event notification
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_drv_raise_platform_event(pldm_request_t* request,
                                            pldm_platform_event_config_t* pe_config,
                                            pldm_platform_event_notification* pe_notification)
{
    if (!pldm_device.is_ready)
    {
        return FPFW_PLDM_SERVICE_E_NOT_READY;
    }

    if (request->header.AllocatedSize != sizeof(pldm_request_t))
    {
        // Request is not properly initialized. Initialize the request.
        DfwkAsyncRequestInitialize(&request->header, sizeof(pldm_request_t));
    }

    // Configure the request.
    request->header.RequestType = PLDM_SEND_PLATFORM_EVENT_ASYNC;
    request->status = FPFW_STATUS_SUCCESS;
    request->pe_config = pe_config;
    request->pe_notification = pe_notification;
    request->completion_code = FPFW_PLDM_CC_SUCCESS;
    DfwkAsyncRequestSetCompletionRoutine(&request->header, pldm_req_cb, NULL);

    // Send the request to DFWK.
    DfwkInterfaceSendAsync(&pldm_interface.header, &request->header);

    return FPFW_STATUS_SUCCESS;
}

/**
 * @brief PLDM driver dispatch routine
 *
 * @param request Async request header
 * @param context Context pointer (pldm_device_t)
 */
static void pldm_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    pldm_device_t* dev = (pldm_device_t*)context;

    switch (request->RequestType)
    {
    case PLDM_GET_READY_ASYNC:
        // Handle PLDM get ready request
        if (dev->is_ready)
        {
            // Complete the request immediately
            pldm_request_t* pldm_request = (pldm_request_t*)request;
            pldm_request->status = FPFW_STATUS_SUCCESS;
            DfwkAsyncRequestComplete(request);
        }
        else
        {
            // Queue the request until PLDM is ready
            DfwkQueueEnqueueRequest(&dev->ready_queue, request);
        }
        break;
    case PLDM_SEND_PLATFORM_EVENT_ASYNC:
        // Handle PLDM send platform event request
        BUG_ASSERT(dev->is_ready);

        if (dev->has_pending_request)
        {
            // Another request is in progress, re-queue this request
            DfwkQueueEnqueueRequest(&dev->svc_queue, request);
        }
        else
        {
            // PLDM is free, process the request
            pldm_platform_event_async(request);
        }
        break;
    default:
        PLDM_ET_ERROR_PARAM(PLDM_ET_TYPE_UNKNOWN_REQUEST_TYPE, request->RequestType);
        FPFW_DBGPRINT_ERROR("PLDM: Unknown request type: 0x%04lx\n", request->RequestType);
        break;
    }
}

/**
 * @brief Initialize PLDM device
 *
 * @param schedule DFWK schedule object
 */
void pldm_device_initialize(PDFWK_SCHEDULE schedule)
{
    DfwkDeviceInitialize(&pldm_device.header, schedule);

    // Initialize crash dump driver request queue.
    DfwkQueueInitialize(&pldm_device.dispatch_queue, &pldm_device.header, pldm_dispatch, &pldm_device, DfwkQueueType_SerializedDispatch);

    // Initialize pldm ready request queue.
    DfwkQueueInitialize(&pldm_device.ready_queue, &pldm_device.header, pldm_dispatch, &pldm_device, DfwkQueueType_ManualDispatch);

    // Initialize pldm service request queue.
    DfwkQueueInitialize(&pldm_device.svc_queue, &pldm_device.header, pldm_dispatch, &pldm_device, DfwkQueueType_ManualDispatch);
}

/**
 * @brief Initialize PLDM interface
 *
 */
void pldm_interface_initialize()
{
    // Initialize interface with device.
    DfwkInterfaceInitialize(&pldm_interface.header, &pldm_device.header, &pldm_device.dispatch_queue, NULL); // Synchonous request is not supported.

    // Open Interface
    int32_t status = DfwkInterfaceOpen(&pldm_interface.header, &pldm_device.header);
    BUG_ASSERT_PARAM(DFWK_SUCCEEDED(status), status, 0);
}

/**
 * @brief Initialize PLDM driver
 *
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_driver_initialize()
{
    pldm_platform_event_ready_notification ready_notification = {
        .CallBack = pldm_platform_event_ready_callback,
        .context = NULL, // No context needed for this callback
    };

    fpfw_status_t status = fpfw_pldm_service_register_platform_event_ready_notification(&ready_notification);

    return status;
}
