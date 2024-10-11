//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_mbox_icc_transport.c
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h> // for DFWK_INTERFACE_HEADER, DFWK_S...
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <MboxPrimitives.h> // for FPFW_MBX_PAYLOAD, FpFwMailbox...
#include <accel_mbox_icc_transport.h>
#include <fpfw_icc_transport_interface.h>
#include <fpfw_status.h>      // for fpfw_status_t
#include <fpfw_timer.h>       // for fpfw_timer_create, fpfw_timer...
#include <fpfw_timer_types.h> // for fpfw_dur_t
#include <stdbool.h>          // for false
#include <stdint.h>           // for int32_t
#include <stdio.h>
#include <string.h> // for NULL, memcpy_s

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Static Functions ----------------*/

static fpfw_status_t convert_mbx_status_to_icc_status(enum FPFW_MBX_STATUS mbx_status)
{
    fpfw_status_t icc_status = FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR;
    //! check for valid mbx status
    if ((mbx_status > FPFW_MBX_SUCCESS) || (mbx_status < FPFW_MBX_FIFO_NO_CAPACITY))
    {
        return icc_status;
    }
    //! match mbx status to icc transport status
    switch (mbx_status)
    {
    case FPFW_MBX_SUCCESS:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_SUCCESS;
        break;
    case FPFW_MBX_UNINITIALIZED:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_UNINITIALIZED_ERR;
        break;
    case FPFW_MBX_E_NULL_ERROR:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
        break;
    case FPFW_MBX_E_INVALID_ARGS:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_INVALID_ARG_ERR;
        break;
    case FPFW_MBX_E_SIZE:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
        break;
    case FPFW_MBX_E_SUPPORT:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_NO_SUPPORT_ERR;
        break;
    case FPFW_MBX_FLUSH_IS_REQUESTED:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FLUSH_IS_REQUESTED;
        break;
    case FPFW_MBX_FLUSH_NOT_COMPLETE:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FLUSH_NOT_COMPLETE;
        break;
    case FPFW_MBX_NO_FLUSH_REQUEST:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_NO_FLUSH_REQUEST;
        break;
    case FPFW_MBX_FIFO_NOT_FULL:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_NOT_FULL;
        break;
    case FPFW_MBX_FIFO_NOT_EMPTY:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_NOT_EMPTY;
        break;
    case FPFW_MBX_FIFO_VALID_SET_MBX_BUSY:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_SEND_BUSY;
        break;
    case FPFW_MBX_FIFO_VALID_NOT_SET_NO_DATA:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_NO_RECV_DATA;
        break;
    case FPFW_E_MBX_FIFO_CNT_ERROR:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_CNT_ERR;
        break;
    case FPFW_MBX_FIFO_NO_CAPACITY:
        icc_status = FPFW_ICC_TRANSPORT_STATUS_MBX_FIFO_SEND_NO_CAPACITY;
        break;
    default:
        break;
    }
    return icc_status;
}

static enum FPFW_MBX_STATUS mbox_remote_flush_handle(fpfw_mbox_icc_transport_device_t* mbox_device)
{
    //! Before pushing/popping the fifo, check if there is an active flush request from remote
    if (FPFW_MBX_SUCCESS == FpFwMailboxIsFlushRequested(&mbox_device->mbox_prim_ctx))
    {
        //! only flush incoming fifo if the remote requested flush
        enum FPFW_MBX_STATUS status = FpFwMailboxFlushFIFO(&mbox_device->mbox_prim_ctx);
        if (FPFW_MBX_SUCCESS != status)
        {
            return status;
        }
    }
    return FPFW_MBX_SUCCESS;
}

static bool mbox_icc_transport_recv_request_handler(PDFWK_ASYNC_REQUEST_HEADER recv_request,
                                                    fpfw_mbox_icc_transport_device_t* mbox_device)
{
    bool is_request_complete = false;
    enum FPFW_MBX_STATUS mbox_status = FPFW_MBX_SUCCESS;
    FPFW_MBX_PAYLOAD mbox_payload = {0, 0};
    //! 1. check if err bit is set, handle flush request from remote
    mbox_remote_flush_handle(mbox_device);
    //! 2. check if valid bit is set, attempt receiving data then
    //! a. fetch recv async request from param
    PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST async_recv_req = (PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST)(recv_request);
    //! b. populate the mbox API params from recv req & update output
    mbox_payload.payloadBuffer = (void*)async_recv_req->Input.PayloadBuffer; // NOLINT
    mbox_payload.payloadSize = async_recv_req->Input.BufferSizeBytes;
    async_recv_req->Output.ReceivedBytes = 0;
    //! c. attempt to recv data over mailbox primitive
    mbox_status = FpFwMailboxReceive(&mbox_device->mbox_prim_ctx, &mbox_payload);
    /**
     * @note complete req for success or error status
     * success -> FPFW_MBX_SUCCESS
     * or errors -> FPFW_MBX_UNINITIALIZED, FPFW_MBX_E_SIZE, FPFW_E_MBX_FIFO_CNT_ERROR
     * or operational status (neither success nor error) -> for FPFW_MBX_FIFO_VALID_NOT_SET_NO_DATA or FPFW_MBX_FLUSH_IS_REQUESTED, do nothing
     */
    if ((mbox_status != FPFW_MBX_FIFO_VALID_NOT_SET_NO_DATA) && (mbox_status != FPFW_MBX_FLUSH_IS_REQUESTED))
    {
        if (mbox_status == FPFW_MBX_SUCCESS)
        {
            async_recv_req->Output.ReceivedBytes = mbox_payload.payloadSize;
        }
        async_recv_req->Output.Status = convert_mbx_status_to_icc_status(mbox_status);
        DfwkAsyncRequestComplete((PDFWK_ASYNC_REQUEST_HEADER)async_recv_req);
        is_request_complete = true;
    }
    return is_request_complete;
}

/*------------- Global Functions ----------------*/

void accel_mbox_sw_intr_cb(void* mbox_dev_ctx)
{
    //! Get the mailbox device context & stored ref to request from the device ctx
    // uint32_t stat = MbxRegRead32((STAT_ADDR), 0);
    fpfw_mbox_icc_transport_device_t* mbox_device = (fpfw_mbox_icc_transport_device_t*)mbox_dev_ctx;
    PDFWK_ASYNC_REQUEST_HEADER async_recv_req =
        (PDFWK_ASYNC_REQUEST_HEADER)(mbox_device->async[ICC_MBX_ASYNC_RECV].request.request_ref);
    //! handle the recv request
    bool is_recv_req_complete = mbox_icc_transport_recv_request_handler(async_recv_req, mbox_device);
    if ((is_recv_req_complete == true) && (FPFwCoreInterruptIsEnabled(mbox_device->mbx_irq_num) == true))
    {
        FPFwCoreInterruptDisableVector(mbox_device->mbx_irq_num);
    }
}
