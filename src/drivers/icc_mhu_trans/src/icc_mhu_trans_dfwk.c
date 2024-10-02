//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Driver framework implentations for ICC MHU Transports.
 */

/*------------- Includes -----------------*/
#include "icc_mhu_trans_prim_i.h"

#include <DfwkHost.h>                     // for DfwkDeviceInitialize
#include <FpFwAssert.h>                   // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>                    // for FPFW_UNUSED
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_mhu.h>                      // for silibs icc mhu low level drivers
#include <icc_mhu_trans_dfwk.h>           // for icc mhu driver framework
#include <icc_mhu_trans_prim.h>           // for icc mhu transport primitives
#include <stdbool.h>                      // for false
#include <stdint.h>                       // for int32_t
#include <string.h>                       // for NULL, memcpy_s

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int32_t icc_mhu_transport_dfwk_interface_open(DFWK_INTERFACE_HEADER* interface)
{
    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)interface->OwningDevice;
    device->ref_count++;

    //! Only init for 1st open
    if (device->ref_count == 1)
    {
        // TBD - need to check if we need additional logic here in the future
        // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2008065
    }
    return DFWK_SUCCESS;
}

void icc_mhu_transport_dfwk_interface_close(DFWK_INTERFACE_HEADER* interface)
{
    icc_mhu_transport_device_t* device = (icc_mhu_transport_device_t*)interface->OwningDevice;
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
    icc_mhu_transport_intrf_t* interface = (icc_mhu_transport_intrf_t*)Request->OwningInterface;
    if (NULL == interface)
    {
        ICC_MHU_LOG_INFO("Sync Request NULL Interface ");
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    icc_mhu_transport_device_t* device = interface->device;
    if (NULL == device)
    {
        ICC_MHU_LOG_INFO("Sync Request NULL Device ");
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Handler for each transport request
    ICC_MHU_LOG_INFO("Request Proc: %d", (int)Request->RequestType);

    switch (Request->RequestType)
    {
    case ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID:
        //! get the request
        ICC_MHU_LOG_INFO("ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID ");

        PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST max_size_req =
            (PFPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST)Request;

        // Check for the interface used
        size_t size = icc_mhu_trans_get_buffer_size(device->config.receive_core_2_core_id);
        max_size_req->Output.MaxMesgSize = size;
        return DFWK_SUCCESS;

    case ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID: {
        //! get the request
        PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST recv_try_req = (PFPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST)Request;

        // Initialize that there is no message
        recv_try_req->Output.ReceivedBytes = 0;

        // Currently the receive format for the ICC primtive MSCP doesnt align well with silibs so we will have to
        //  fix the silibs to make sure the data structures are the same.
        //  Once fixed this will simplify the implementations below
        //  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2029706

        uint8_t recv_data[SIZE_OF_TEMP_BUFFER];
        icc_msg_receive_t message;
        message.data = recv_data;

        ICC_MHU_LOG_INFO("id: %x", device->config.receive_core_2_core_id);
        int status = icc_mhu_trans_get_message(device->config.receive_core_2_core_id, &message);

        if (status == ICC_MHU_STATUS_SUCCESS)
        {
            ICC_MHU_LOG_INFO("ICC Command: %x\n", (int)message.command);
            ICC_MHU_LOG_INFO("ICC Payload Size: %d\n", (int)message.size);

            // Check if the client's buffer is enough to store what has been received as command payload
            size_t size_of_transport_header = sizeof(icc_mhu_transport_message_t);
            if ((recv_try_req->Input.BufferSizeBytes - size_of_transport_header) >= message.size)
            {
                icc_mhu_transport_message_t* icc_mhu_trans_buffer =
                    (icc_mhu_transport_message_t*)recv_try_req->Input.PayloadBuffer;

                if (message.data != NULL)
                {
                    // Copy the received buffer
                    memcpy(icc_mhu_trans_buffer->data, message.data, message.size);

                    // Copy the command and the cmd payload size
                    icc_mhu_trans_buffer->command = message.command;
                    icc_mhu_trans_buffer->size = message.size;
                    recv_try_req->Output.ReceivedBytes = message.size + size_of_transport_header;
                    status = ICC_MHU_TRANS_INTF_MSG_AVAILABLE;
                }
                else
                {
                    status = ICC_MHU_TRANS_NO_RECEIVE_MESSAGE;
                }
            }
            else
            {
                status = ICC_MHU_TRANS_NO_RECEIVE_MESSAGE;
            }
        }
        else
        {
            status = ICC_MHU_TRANS_NO_RECEIVE_MESSAGE;
        }
        return status;
    }
    case ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID:
        //! Process request for sending a message
        ICC_MHU_LOG_INFO("ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID ");
        FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST* send_try_req = (FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST*)Request;
        icc_mhu_transport_sync_message_t* send_request =
            (icc_mhu_transport_sync_message_t*)send_try_req->Input.PayloadBuffer;

        // Process the request
        return icc_mhu_trans_send_message(device->config.send_core_2_core_id,
                                          send_request->command,
                                          send_request->payload,
                                          send_request->size);
        break;
    default:
        return FPFW_ICC_TRANSPORT_STATUS_UNSUPPORTED_REQ_ERR;
    }
    return DFWK_SUCCESS;
}

void icc_mhu_transport_driver_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);

    if (Request == NULL)
    {
        // Handle null pointer error
        return;
    }

    icc_mhu_transport_intrf_t* interface = (icc_mhu_transport_intrf_t*)Request->OwningInterface;
    icc_mhu_transport_device_t* device = interface->device;
    FPFW_UNUSED(device);

    //! push the requests to their respective queues
    switch (Request->RequestType)
    {
    case ICC_TRANSPORT_RECV_ASYNC_REQUEST_ID:
        // TBD
        break;

    case ICC_TRANSPORT_SEND_ASYNC_REQUEST_ID:
        // TBD
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

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

    //! initialize the default queue as immediate dispatch, takes in requests before the prev is completed

    DfwkDeviceInitialize(&icc_mhu_dev->header, schedule);
    DfwkQueueInitialize(&icc_mhu_dev->default_queue,
                        &icc_mhu_dev->header,
                        icc_mhu_transport_driver_dispatch_async,
                        icc_mhu_dev,
                        DfwkQueueType_ImmediateDispatch);

    return DFWK_SUCCESS;
}

int32_t icc_mhu_trans_dfwk_interface_init(icc_mhu_transport_device_t* icc_mhu_dev, icc_mhu_transport_intrf_t* interface)
{
    if ((NULL == icc_mhu_dev) || (NULL == interface))
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    //! initialize the interface
    DfwkInterfaceInitialize(&interface->header, &icc_mhu_dev->header, &icc_mhu_dev->default_queue, icc_mhu_transport_driver_dispatch_sync);
    //! Set open close
    DfwkInterfaceSetOpenClose(&interface->header, icc_mhu_transport_dfwk_interface_open, icc_mhu_transport_dfwk_interface_close);
    interface->device = icc_mhu_dev;
    return DFWK_SUCCESS;
}

fpfw_status_t icc_mhu_trans_dfwk_send_sync_message(DFWK_INTERFACE_HEADER* interface, uint32_t command, uint8_t* payload, size_t payload_size)
{
    // Check for NULL Interface
    if (NULL == interface)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Check for NULL data with non zero size
    if (NULL == payload && payload_size != 0)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    if (payload_size > (SIZE_OF_TEMP_BUFFER - sizeof(icc_mhu_trans_header_t)))
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    }

    // Create the buffer payload
    icc_mhu_transport_sync_message_t send_buffer;
    send_buffer.command = command;
    send_buffer.payload = payload;
    send_buffer.size = payload_size;
    return fpfw_icc_transport_try_send_sync_req(interface, &send_buffer, sizeof(send_buffer));
}

fpfw_status_t icc_mhu_trans_dfwk_recv_sync_message(DFWK_INTERFACE_HEADER* interface, uint8_t* payload, size_t payload_size)
{
    // Check for NULL Interface
    if (NULL == interface)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Check for NULL data with non zero size
    if (NULL == payload && payload_size != 0)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    if (payload_size > (SIZE_OF_TEMP_BUFFER - sizeof(icc_mhu_trans_header_t)))
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    }

    // Create the buffer payload
    size_t output_size;
    fpfw_status_t status = fpfw_icc_transport_try_recv_sync_req(interface, payload, payload_size, &output_size);
    return status;
}

fpfw_status_t icc_mhu_trans_dfwk_chk_recv_cmd_sync(DFWK_INTERFACE_HEADER* interface, uint32_t command, uint8_t* payload, size_t payload_size)
{
    // Check for NULL Interface
    if (NULL == interface)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    // Check for NULL data with non zero size
    if (NULL == payload && payload_size != 0)
    {
        return FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR;
    }

    if (payload_size > (SIZE_OF_TEMP_BUFFER - sizeof(icc_mhu_trans_header_t)))
    {
        return FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR;
    }

    // Create the buffer payload
    size_t output_size;
    uint8_t temp_payload[SIZE_OF_TEMP_BUFFER];

    fpfw_status_t status =
        fpfw_icc_transport_try_recv_sync_req(interface, temp_payload, sizeof(temp_payload), &output_size);
    if (status == ICC_MHU_TRANS_INTF_MSG_AVAILABLE)
    {
        icc_mhu_transport_message_t* icc_msg = (icc_mhu_transport_message_t*)temp_payload;

        if (icc_msg->command == command)
        {
            // Copy the size available provided by the client
            if (payload_size >= icc_msg->size)
            {
                memcpy(payload, icc_msg->data, icc_msg->size);
                status = ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE;
            }
        }
    }
    return status;
}