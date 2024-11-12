//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_trans_dfwk.c
 * Driver framework implementations for ICC MHU Transports.
 */

// TODO: Refactor sync commands - https://azurecsi.visualstudio.com/Dev/_workitems/edit/2154859
// TODO: Remove ICC MHU Trans Prim layer - https://azurecsi.visualstudio.com/Dev/_workitems/edit/2154862
// TODO: Add Event Trace - https://azurecsi.visualstudio.com/Dev/_workitems/edit/2154865
// TODO: Timer implementation - https://azurecsi.visualstudio.com/Dev/_workitems/edit/2159240

/*------------- Includes -----------------*/
#include "icc_mhu_trans_prim_i.h"

#include <DfwkHost.h>                     // for DfwkDeviceInitialize
#include <FPFwInterrupts.h>               // for FPFwCoreInterruptRegisterCallback
#include <FpFwAssert.h>                   // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>                    // for FPFW_UNUSED
#include <bug_check.h>                    // for BUG_ASSERT
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_mhu.h>                      // for silibs icc mhu low level drivers
#include <icc_mhu_trans_dfwk.h>           // for icc mhu driver framework
#include <icc_mhu_trans_prim.h>           // for icc mhu transport primitives
#include <stdbool.h>                      // for false
#include <stdint.h>                       // for int32_t
#include <string.h>                       // for NULL, memcpy_s
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MS_TO_TX_TICKS(ms)       (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)
#define ASYNC_SEND_START_TICK    (TX_TIMER_TICKS_PER_SECOND / 2)
#define ASYNC_SEND_PERIODIC_TICK (MS_TO_TX_TICKS(100))
#define ASYNC_SEND_RETRY_COUNT   (5)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// PRIVATE FUNCTIONS
//

static fpfw_status_t mhu_transport_status_to_icc_transport_status(int32_t status)
{
    switch (status)
    {
    case ICC_MHU_STATUS_SUCCESS:
        return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
    case ICC_MHU_INVALID_PARAMETER:
    case ICC_MHU_INVALID_INTERFACE:
    case ICC_MHU_INVALID_INDEX:
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_ARG_ERR;
    case ICC_MHU_MESSAGE_SIZE_ERROR:
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    case ICC_MHU_INTERFACE_BUSY:
    case ICC_MHU_NO_MSG_RECEIVED:
    case ICC_MHU_NOT_CONFIGURED:
    case ICC_MHU_NO_MESSAGE:
    default:
        return FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR;
    }
}

static void async_send_attempt(ULONG Context)
{

    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)Context;

    // Send the message
    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST* async_req =
        (FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST*)device->async_send_ctx.request_ref;

    icc_mhu_request_t* p_mhu_request = (icc_mhu_request_t*)(async_req->Input.PayloadBuffer);
    int send_status = icc_mhu_trans_send_message(device->config.send_core_2_core_id,
                                                 p_mhu_request->header.msg_header.command,
                                                 p_mhu_request->payload,
                                                 p_mhu_request->header.msg_header.payload_size);

    // Grab the current timer status
    UINT timer_active = TX_FALSE;
    UINT tx_status = tx_timer_info_get(&device->async_send_ctx.timer, NULL, &timer_active, NULL, NULL, NULL);
    BUG_ASSERT(TX_SUCCESS == tx_status);

    //
    // We complete the request if:
    //   - The message was sent successfully OR
    //   - The timer has hit the retry limit
    //
    // We start the timer if:
    //  - The message was not sent successfully AND
    //  - The timer is not active

    // Update the timer retry count, if timer active
    if (timer_active == TX_TRUE)
    {
        device->async_send_ctx.timer_retry_count++;
    }

    if (send_status == ICC_MHU_STATUS_SUCCESS || device->async_send_ctx.timer_retry_count == ASYNC_SEND_RETRY_COUNT)
    {
        // Update the request and complete it
        async_req->Output.Status = mhu_transport_status_to_icc_transport_status(send_status);
        DfwkAsyncRequestComplete(device->async_send_ctx.request_ref);

        // Set the current request to NULL and stop the timer if active
        device->async_send_ctx.request_ref = NULL;
        if (TX_TRUE == timer_active)
        {
            tx_status = tx_timer_deactivate(&device->async_send_ctx.timer);
            BUG_ASSERT(TX_SUCCESS == tx_status);
        }
    }
    else if (send_status != ICC_MHU_STATUS_SUCCESS && timer_active == TX_FALSE)
    {
        // Activate the timer and clear the reties
        tx_status = tx_timer_activate(&device->async_send_ctx.timer);
        BUG_ASSERT(TX_SUCCESS == tx_status);
        device->async_send_ctx.timer_retry_count = 0;
    }
}

//
// PRIVATE HEADER FUNCTIONS
//

void icc_mhu_isr(void* context)
{
    //
    // 1. Disable the IRQ (a new request will re-enable it)
    // 2. Get the message from the receive channel (using the payload from the request)
    // 3. Complete the request via DFWK
    //

    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)context;
    FPFwCoreInterruptDisableVector(device->config.irq_num);

    // Validate that doorbell for the receive channel is set. It should be the only
    // thing that triggers this ISR.
    int status = icc_mhu_check_message_received(device->config.receive_core_2_core_id);
    BUG_ASSERT(ICC_MHU_STATUS_SUCCESS == status);

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST* p_req =
        (FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST*)(device->async_recv_ctx.request_ref);

    // If the mhu request from the payload doesn't have a size AND the input payload size is
    // greater than the size of the mhu request, update the mhu request size.
    icc_mhu_request_t* p_mhu_req = (icc_mhu_request_t*)(p_req->Input.PayloadBuffer);

    // These can occur if a user doesn't pre-populate the payload
    if (p_mhu_req->header.msg_header.payload_size == 0 && p_req->Input.BufferSizeBytes > sizeof(icc_mhu_header_t))
    {
        p_mhu_req->header.msg_header.payload_size = p_req->Input.BufferSizeBytes - sizeof(icc_mhu_header_t);
    }

    status = icc_mhu_trans_get_msg_from_index(device->config.receive_core_2_core_id, p_mhu_req);
    p_req->Output.ReceivedBytes = p_mhu_req->header.msg_header.payload_size + sizeof(icc_mhu_header_t);
    if (status != ICC_MHU_STATUS_SUCCESS)
    {
        p_req->Output.ReceivedBytes = 0;
    }
    p_req->Output.Status = mhu_transport_status_to_icc_transport_status(status);

    DfwkAsyncRequestComplete(device->async_recv_ctx.request_ref);

    device->async_recv_ctx.request_ref = NULL;
}

void dispatch_async_send(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{

    //
    // When handling an async send request we need to:
    //     1. Determine what channel this is for (from the context)
    //     2. Set the request as the active async send request
    //     3. Try to send the request
    //

    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)Context;
    device->async_send_ctx.request_ref = Request;

    async_send_attempt((ULONG)device);
}

void dispatch_async_recv(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)Context;
    device->async_recv_ctx.request_ref = Request;
    BUG_ASSERT(0 == FPFwCoreInterruptEnableVector(device->config.irq_num));
}

int32_t icc_mhu_transport_dfwk_interface_open(DFWK_INTERFACE_HEADER* p_interface)
{
    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)p_interface->OwningDevice;
    device->ref_count++;

    return DFWK_SUCCESS;
}

void icc_mhu_transport_dfwk_interface_close(DFWK_INTERFACE_HEADER* p_interface)
{
    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)p_interface->OwningDevice;
    device->ref_count--;

    //! Only reset for last close
    if (device->ref_count == 0)
    {
        // TBD - need to check if we need additional logic here in the future
        // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2008065
    }
}

fpfw_status_t icc_mhu_transport_driver_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request)
{
    //! Verify request
    if (NULL == Request)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    //! get interface & device ref from request
    icc_mhu_transport_intrf_t* p_interface = (icc_mhu_transport_intrf_t*)Request->OwningInterface;
    if (NULL == p_interface)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    icc_mhu_transport_device_t* device = p_interface->device;
    if (NULL == device)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Handler for each transport request
    switch (Request->RequestType)
    {
    case ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID: {
        //! get the request
        PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST max_size_req =
            (PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST)Request;

        // Check for the interface used
        size_t size = icc_mhu_trans_get_buffer_size(device->config.receive_core_2_core_id);
        max_size_req->Output.MaxMesgSize = size;
        return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
    }

    case ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID: {
        //! get the request
        PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST recv_try_req = (PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST)Request;

        // Currently the receive format for the ICC primtive MSCP doesnt align well with silibs so we will have to
        //  fix the silibs to make sure the data structures are the same.
        //  Once fixed this will simplify the implementations below
        //  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2029706

        // Grab the request from the payload
        icc_mhu_request_t* p_mhu_req = (icc_mhu_request_t*)(recv_try_req->Input.PayloadBuffer);

        // These can occur if a user doesn't pre-populate the payload
        if (p_mhu_req->header.msg_header.payload_size == 0 && recv_try_req->Input.BufferSizeBytes > sizeof(icc_mhu_header_t))
        {
            p_mhu_req->header.msg_header.payload_size = recv_try_req->Input.BufferSizeBytes - sizeof(icc_mhu_header_t);
        }
        int status = icc_mhu_trans_get_msg_from_index(device->config.receive_core_2_core_id, p_mhu_req);

        recv_try_req->Output.ReceivedBytes = p_mhu_req->header.msg_header.payload_size + sizeof(icc_mhu_header_t);
        if (status != ICC_MHU_STATUS_SUCCESS)
        {
            recv_try_req->Output.ReceivedBytes = 0;
        }

        return mhu_transport_status_to_icc_transport_status(status);
    }
    case ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID: {
        //! Process request for sending a message
        FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST* send_try_req = (FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST*)Request;
        icc_mhu_request_t* send_request = (icc_mhu_request_t*)send_try_req->Input.PayloadBuffer;

        // Process the request
        int status = icc_mhu_trans_send_message(device->config.send_core_2_core_id,
                                                send_request->header.msg_header.command,
                                                send_request->payload,
                                                send_request->header.msg_header.payload_size);

        return mhu_transport_status_to_icc_transport_status(status);
    }
    default:
        return FPFW_ICC_TRANSPORT_STATUS_UNSUPPORTED_REQ_ERR;
    }
    return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
}

void icc_mhu_transport_driver_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);

    icc_mhu_transport_intrf_t* p_interface = (icc_mhu_transport_intrf_t*)Request->OwningInterface;
    icc_mhu_transport_device_t* device = p_interface->device;

    //! push the requests to their respective queues
    switch (Request->RequestType)
    {
    case ICC_TRANSPORT_RECV_ASYNC_REQUEST_ID:
        DfwkQueueEnqueueRequest(&device->async_recv_ctx.queue, Request);
        break;
    case ICC_TRANSPORT_SEND_ASYNC_REQUEST_ID:
        DfwkQueueEnqueueRequest(&device->async_send_ctx.queue, Request);
        break;
    default:
        break;
    }
}

//
// PUBLIC HEADER FUNCTIONS
//

int32_t icc_mhu_transport_dfwk_device_init(icc_mhu_transport_device_t* icc_mhu_dev,
                                           DFWK_SCHEDULE* schedule,
                                           icc_mhu_transport_config_t* config)
{
    if ((NULL == icc_mhu_dev) || (NULL == config) || schedule == NULL)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // store the configurations
    icc_mhu_dev->config = *config;

    // initialize the default queue as immediate dispatch, takes in requests before the prev is completed
    DfwkDeviceInitialize(&icc_mhu_dev->header, schedule);
    DfwkQueueInitialize(&icc_mhu_dev->default_queue,
                        &icc_mhu_dev->header,
                        icc_mhu_transport_driver_dispatch_async,
                        icc_mhu_dev,
                        DfwkQueueType_ImmediateDispatch);

    // Setup the async handling
    icc_mhu_dev->async_send_ctx.mhu_device = icc_mhu_dev;
    icc_mhu_dev->async_send_ctx.dispatch_routine = dispatch_async_send;
    DfwkQueueInitialize(&icc_mhu_dev->async_send_ctx.queue,
                        &icc_mhu_dev->header,
                        icc_mhu_dev->async_send_ctx.dispatch_routine,
                        icc_mhu_dev->async_send_ctx.mhu_device,
                        DfwkQueueType_SerializedDispatch);
    icc_mhu_dev->async_send_ctx.request_ref = NULL;

    UINT txStatus = tx_timer_create(&icc_mhu_dev->async_send_ctx.timer,
                                    "mhu_async_send_tmr",
                                    async_send_attempt,
                                    (ULONG)icc_mhu_dev,
                                    ASYNC_SEND_START_TICK,
                                    ASYNC_SEND_PERIODIC_TICK,
                                    TX_NO_ACTIVATE);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    icc_mhu_dev->async_recv_ctx.mhu_device = icc_mhu_dev;
    icc_mhu_dev->async_recv_ctx.dispatch_routine = dispatch_async_recv;
    DfwkQueueInitialize(&icc_mhu_dev->async_recv_ctx.queue,
                        &icc_mhu_dev->header,
                        icc_mhu_dev->async_recv_ctx.dispatch_routine,
                        icc_mhu_dev->async_recv_ctx.mhu_device,
                        DfwkQueueType_SerializedDispatch);
    icc_mhu_dev->async_recv_ctx.request_ref = NULL;

    // Register the ISR, IRQ is enabled when processing a receive request
    BUG_ASSERT(0 == FPFwCoreInterruptRegisterCallback(icc_mhu_dev->config.irq_num, icc_mhu_isr, icc_mhu_dev));

    return DFWK_SUCCESS;
}

int32_t icc_mhu_trans_dfwk_interface_init(icc_mhu_transport_device_t* icc_mhu_dev, icc_mhu_transport_intrf_t* p_interface)
{
    if ((NULL == icc_mhu_dev) || (NULL == p_interface))
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    //! initialize the interface
    DfwkInterfaceInitialize(&p_interface->header, &icc_mhu_dev->header, &icc_mhu_dev->default_queue, icc_mhu_transport_driver_dispatch_sync);
    //! Set open close
    DfwkInterfaceSetOpenClose(&p_interface->header, icc_mhu_transport_dfwk_interface_open, icc_mhu_transport_dfwk_interface_close);
    p_interface->device = icc_mhu_dev;
    return DFWK_SUCCESS;
}

// The default match strategy for the ICC dispatcher does not support commands or sequence numbers
// not in the first 32 bits of a payload
bool icc_mhu_trans_dwfk_icc_dispatcher_match_cb(PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req,
                                                fpfw_icc_dispatch_table_entry* current_entry,
                                                void* ctx)
{
    FPFW_UNUSED(ctx);
    icc_mhu_request_t* client_msg = (icc_mhu_request_t*)(recv_req->Input.PayloadBuffer);
    return client_msg->header.msg_header.command == current_entry->cmd_code;
}