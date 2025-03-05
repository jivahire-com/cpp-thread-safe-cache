//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mhu_icc_transport.c
 *
 * @brief This is a mhu icc transport driver that implements the ICC transport interface
 */

// TODO: Add Event Trace - https://azurecsi.visualstudio.com/Dev/_workitems/edit/2154865

/*------------- Includes -----------------*/

#include "mhu_icc_transport_i.h"

#include <DfwkHost.h>
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <fpfw_icc_transport_interface.h>
#include <fpfw_status.h>
#include <fpfw_timer.h>
#include <fpfw_timer_port.h>
#include <fpfw_timer_types.h>
#include <icc_mhu.h>
#include <kng_icc_shared.h>
#include <mhu_icc_transport.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// PRIVATE FUNCTIONS
//

//
// The silibs icc_mhu library has its own status, while this driver returns
// the FPFW_ICC_TRANSPORT_STATUS_* statuses to adhere to the ICC transport
// interface.
//
static fpfw_status_t icc_packet_status_to_icc_transport_status(uint32_t status)
{
    switch (status)
    {
    case ICC_MHU_STATUS_S_SUCCESS:
        return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
    case ICC_MHU_STATUS_E_INVALID_PARAM:
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_ARG_ERR;
    case ICC_MHU_STATUS_E_PACKET_TOO_LARGE:
    case ICC_MHU_STATUS_E_PACKET_TOO_SMALL:
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    case ICC_MHU_STATUS_E_CHANNEL_BUSY:
        return FPFW_ICC_TRANSPORT_STATUS_BUSY;
    case ICC_MHU_STATUS_E_CHANNEL_NOT_BUSY:
    default:
        return FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR;
    }
}

//
// We attempt to send an async request when:
//   1. A request is dequeued from the serialized dispatch queue
//   2. The timer expires
//
static void async_send_attempt(void* Context, fpfw_dur_t latency)
{
    FPFW_UNUSED(latency);

    mhu_icc_transport_device_t* device = (mhu_icc_transport_device_t*)Context;

    // Send the packet
    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST* async_req =
        (FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST*)device->async_send_ctx.req;

    icc_mhu_packet_t* p_mhu_packet = (icc_mhu_packet_t*)(async_req->Input.PayloadBuffer);
    uint32_t icc_packet_status = icc_mhu_send_packet(&(device->send_channel), p_mhu_packet);

    //
    // We complete the request if:
    //   - The packet was sent successfully OR
    //   - The timer has hit the retry limit
    //
    // We start the timer if:
    //  - The packet was not sent successfully AND
    //  - The timer is not active

    // Update the timer retry count, if timer active
    if (device->async_send_ctx.timer_active)
    {
        device->async_send_ctx.timer_retry_count++;
    }

    if (icc_packet_status == ICC_MHU_STATUS_S_SUCCESS ||
        device->async_send_ctx.timer_retry_count == device->async_send_ctx.timer_retry_max)
    {
        // Update the request and complete it
        async_req->Output.Status = icc_packet_status_to_icc_transport_status(icc_packet_status);
        DfwkAsyncRequestComplete(device->async_send_ctx.req);

        // Set the current request to NULL and stop the timer if active
        device->async_send_ctx.req = NULL;
        if (device->async_send_ctx.timer_active)
        {
            device->async_send_ctx.timer_active = false;
            fpfw_status_t timer_status = fpfw_timer_reset(&(device->async_send_ctx.timer));
            FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCESS == timer_status, timer_status, 0, 0, 0);
        }
    }
    else if (icc_packet_status != ICC_MHU_STATUS_S_SUCCESS && !device->async_send_ctx.timer_active)
    {
        // Activate the timer and clear the reties
        device->async_send_ctx.timer_retry_count = 0;
        device->async_send_ctx.timer_active = true;

        fpfw_status_t timer_status =
            fpfw_timer_enable(&(device->async_send_ctx.timer), device->async_send_ctx.timer_period);
        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCESS == timer_status, timer_status, 0, 0, 0);
    }
}

//
// PRIVATE HEADER FUNCTIONS
//

/**
 * @TODO: Decide if we want to do anything with the open / close https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2008065
 *        Current design has necessary logic in device init. Open / Close only for ICC Base Support.
 */

int32_t mhu_icc_transport_interface_open(DFWK_INTERFACE_HEADER* p_interface)
{
    FPFW_UNUSED(p_interface);

    return DFWK_SUCCESS;
}

void mhu_icc_transport_interface_close(DFWK_INTERFACE_HEADER* p_interface)
{
    FPFW_UNUSED(p_interface);
}

fpfw_status_t mhu_icc_transport_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request)
{
    // Verify request
    if (NULL == Request)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    //! get interface & device ref from request
    mhu_icc_transport_intrf_t* p_interface = (mhu_icc_transport_intrf_t*)Request->OwningInterface;
    if (NULL == p_interface)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    mhu_icc_transport_device_t* device = p_interface->device;
    if (NULL == device)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    fpfw_status_t status = FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR;
    // Handler for each transport request
    switch (Request->RequestType)
    {
    case ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID: {
        PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST max_size_req =
            (PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST)Request;

        // recv and send channels having the same size is validated on device init.
        // So we can use either channel size here.
        max_size_req->Output.MaxMesgSize = device->recv_channel.ch_shared_mem_size;
        status = FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
        break;
    }
    case ICC_TRANSPORT_GET_MIN_MESG_SIZE_SYNC_REQUEST_ID: {
        PFPFW_ICC_TRANSPORT_SYNC_GET_MIN_MESG_SIZE_REQUEST min_size_req =
            (PFPFW_ICC_TRANSPORT_SYNC_GET_MIN_MESG_SIZE_REQUEST)Request;

        // The minimum size is the size of the header
        min_size_req->Output.MinMesgSize = sizeof(icc_mhu_header_t);
        return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
    }
    case ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID: {
        PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST recv_try_req = (PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST)Request;

        // Grab the request from the payload
        icc_mhu_packet_t* p_mhu_packet = (icc_mhu_packet_t*)(recv_try_req->Input.PayloadBuffer);

        // This can occur where the user has provided a large enough buffer but not filled in
        // the expected payload size. We will fill in the payload size here.
        if (p_mhu_packet->header.msg_header.payload_size == 0 && recv_try_req->Input.BufferSizeBytes > sizeof(icc_mhu_header_t))
        {
            p_mhu_packet->header.msg_header.payload_size = recv_try_req->Input.BufferSizeBytes - sizeof(icc_mhu_header_t);
        }
        uint32_t icc_packet_status = icc_mhu_get_packet(&(device->recv_channel), p_mhu_packet);

        recv_try_req->Output.ReceivedBytes = p_mhu_packet->header.msg_header.payload_size + sizeof(icc_mhu_header_t);
        if (icc_packet_status != ICC_MHU_STATUS_S_SUCCESS)
        {
            recv_try_req->Output.ReceivedBytes = 0;
        }

        status = icc_packet_status_to_icc_transport_status(icc_packet_status);
        break;
    }
    case ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID: {
        // Process request for sending a packet
        FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST* send_try_req = (FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST*)Request;
        icc_mhu_packet_t* p_mhu_packet = (icc_mhu_packet_t*)send_try_req->Input.PayloadBuffer;

        // Process the request
        uint32_t icc_packet_status = icc_mhu_send_packet(&(device->send_channel), p_mhu_packet);

        status = icc_packet_status_to_icc_transport_status(icc_packet_status);
        break;
    }
    default:
        status = FPFW_ICC_TRANSPORT_STATUS_UNSUPPORTED_REQ_ERR;
    }
    return status;
}

void mhu_icc_transport_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    mhu_icc_transport_device_t* device = (mhu_icc_transport_device_t*)Context;

    // Push the requests to their respective serialized queues
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

void mhu_icc_transport_dispatch_async_recv(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    //
    // When handling an async receive request we need to:
    //    1. Determine what device this is for (from the context)
    //    2. Set the request as the active async receive request
    //    3. Enable the IRQ
    //

    mhu_icc_transport_device_t* device = (mhu_icc_transport_device_t*)Context;
    device->async_recv_ctx.req = Request;
    uint32_t irq_status = FPFwCoreInterruptEnableVector(device->async_recv_ctx.recv_irq_num);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCESS == irq_status, irq_status, device->async_recv_ctx.recv_irq_num, (uintptr_t)device, 0);
}

void mhu_icc_transport_dispatch_async_send(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{

    //
    // When handling an async send request we need to:
    //     1. Determine what device this is for (from the context)
    //     2. Set the request as the active async send request
    //     3. Try to send the request
    //

    mhu_icc_transport_device_t* device = (mhu_icc_transport_device_t*)Context;
    device->async_send_ctx.req = Request;

    async_send_attempt(device, 0);
}

void mhu_icc_transport_isr(void* context)
{
    //
    // 1. Disable the IRQ (a new request will re-enable it)
    // 2. Get the packet from the receive channel (using the payload from the request)
    // 3. Complete the request via DFWK
    //

    mhu_icc_transport_device_t* device = (mhu_icc_transport_device_t*)context;
    FPFwCoreInterruptDisableVector(device->async_recv_ctx.recv_irq_num);

    // Validate that doorbell for the receive channel is set. It should be the only
    // thing that triggers this ISR.
    FPFW_RUNTIME_ASSERT_EXT(icc_mhu_check_packet_pending(&(device->recv_channel)), 0, 0, 0, 0);

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST* p_req =
        (FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST*)(device->async_recv_ctx.req);

    // If the mhu request from the payload doesn't have a size AND the input payload size is
    // greater than the size of the mhu request, update the mhu request size.
    icc_mhu_packet_t* p_mhu_req = (icc_mhu_packet_t*)(p_req->Input.PayloadBuffer);

    // This can occur where the user has provided a large enough buffer but not filled in
    // the expected payload size. We will fill in the payload size here.
    if (p_mhu_req->header.msg_header.payload_size == 0 && p_req->Input.BufferSizeBytes > sizeof(icc_mhu_header_t))
    {
        p_mhu_req->header.msg_header.payload_size = p_req->Input.BufferSizeBytes - sizeof(icc_mhu_header_t);
    }

    uint32_t icc_packet_status = icc_mhu_get_packet(&(device->recv_channel), p_mhu_req);

    // Update the user request based on the success of getting a packet
    p_req->Output.ReceivedBytes = 0;
    if (icc_packet_status == ICC_MHU_STATUS_S_SUCCESS)
    {
        if (p_mhu_req->header.msg_header.command == ICC_SIGNAL_CRASH_DUMP_COLLECT)
        {
            // Crash dump collect signal received from the other core.
            // Trigger an external bug check
            BUG_CHECK_EXTERNAL();
        }

        p_req->Output.ReceivedBytes = p_mhu_req->header.msg_header.payload_size + sizeof(icc_mhu_header_t);
    }
    p_req->Output.Status = icc_packet_status_to_icc_transport_status(icc_packet_status);

    DfwkAsyncRequestComplete(device->async_recv_ctx.req);

    device->async_recv_ctx.req = NULL;
}

//
// PUBLIC HEADER FUNCTIONS
//

fpfw_status_t mhu_icc_transport_device_init(mhu_icc_transport_device_t* dev,
                                            DFWK_SCHEDULE* schedule,
                                            mhu_icc_transport_device_config_t* config)
{
    if ((NULL == dev) || (NULL == config) || schedule == NULL)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    if (config->recv_channel.ch_shared_mem_size != config->send_channel.ch_shared_mem_size)
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    }

    if (config->recv_channel.ch_shared_mem_size < sizeof(icc_mhu_header_t))
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    }

    if (!FPFW_TIMER_VALID_TIME(config->async_send_retry_period))
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_ARG_ERR;
    }

    dev->recv_channel = config->recv_channel;
    dev->send_channel = config->send_channel;

    dev->async_recv_ctx.recv_irq_num = config->recv_irq_num;
    dev->async_send_ctx.timer_retry_max = config->async_send_retry_max;
    dev->async_send_ctx.timer_period = config->async_send_retry_period;

    // Initialize the device
    DfwkDeviceInitialize(&(dev->base_device), schedule);

    // Init the top level async queue as immediate dispatch
    DfwkQueueInitialize(&(dev->async_req_queue), &(dev->base_device), mhu_icc_transport_dispatch_async, dev, DfwkQueueType_ImmediateDispatch);

    // Setup the async recv handling
    DfwkQueueInitialize(&(dev->async_recv_ctx.queue), &(dev->base_device), mhu_icc_transport_dispatch_async_recv, dev, DfwkQueueType_SerializedDispatch);
    dev->async_recv_ctx.req = NULL;

    // Register the ISR, IRQ is enabled when processing a receive request
    uint32_t isr_status =
        FPFwCoreInterruptRegisterCallback(dev->async_recv_ctx.recv_irq_num, mhu_icc_transport_isr, dev);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCESS == isr_status, isr_status, dev->async_recv_ctx.recv_irq_num, (uintptr_t)dev, 0);

    // Setup the async send handling
    DfwkQueueInitialize(&(dev->async_send_ctx.queue), &(dev->base_device), mhu_icc_transport_dispatch_async_send, dev, DfwkQueueType_SerializedDispatch);
    dev->async_send_ctx.req = NULL;

    fpfw_status_t timer_status = fpfw_timer_create(&(dev->async_send_ctx.timer),
                                                   FPFW_TIMER_PERIODIC,
                                                   config->async_send_retry_period,
                                                   async_send_attempt,
                                                   (void*)dev);
    FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCESS == timer_status, timer_status, 0, 0, 0);

    return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
}

fpfw_status_t mhu_icc_transport_interface_init(mhu_icc_transport_device_t* dev, mhu_icc_transport_intrf_t* p_interface)
{
    if ((NULL == dev) || (NULL == p_interface))
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Initialize the interface
    DfwkInterfaceInitialize(&(p_interface->base_interface), &(dev->base_device), &(dev->async_req_queue), mhu_icc_transport_dispatch_sync);
    // Set open close functions
    DfwkInterfaceSetOpenClose(&p_interface->base_interface, mhu_icc_transport_interface_open, mhu_icc_transport_interface_close);

    p_interface->device = dev;

    return FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
}

// The default match strategy for the ICC dispatcher does not support commands or sequence numbers
// not in the first 32 bits of a payload. Therefore we provide a custom match callback.
bool mhu_icc_transport_dispatcher_match_cb(PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req,
                                           fpfw_icc_dispatch_table_entry* current_entry,
                                           void* ctx)
{
    FPFW_UNUSED(ctx);
    icc_mhu_packet_t* client_msg = (icc_mhu_packet_t*)(recv_req->Input.PayloadBuffer);
    return client_msg->header.msg_header.command == current_entry->cmd_code;
}
