//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcp_service_client.c
 * In the DCP protocol, there are some messages that are handled by DCS and not one of the specific telemetry
 * clients. This file contains the handling of those messages.
 */

/*------------- Includes -----------------*/
#include "data_collection_service_i.h"
#include "dcp_service_client_i.h"
#include "dcs_events_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DCS_MAX_DCS_SVC_ClIENT_MESSAGES (DCP_MSG_ID_MAX)
#define DCS_SVC_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + DCS_TRP_MSG_BLOCK_SIZE) * DCS_MAX_DCS_SVC_ClIENT_MESSAGES)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

trp_msg_t* dcs_svc_client_queue_mem[DCS_MAX_DCS_SVC_ClIENT_MESSAGES];
uint8_t dcs_svc_client_pool_mem[DCS_SVC_CLIENT_BLOCK_POOL_SIZE];

/*-- Declarations (Statics and globals) --*/
static dcs_client_t s_dcs_client = {
    .notify_from_drv_frmwk = dcs_service_client_notification_from_drv_frmwk,
};

/*------------- Functions ----------------*/

void dcp_svc_client_init(void)
{

    UINT tx_status = tx_queue_create(&s_dcs_client.rx_queue,
                                     "DCS client queue",
                                     1, // number of uint32_t elements
                                     dcs_svc_client_queue_mem,
                                     sizeof(dcs_svc_client_queue_mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_client.rx_queue, 0, 0);

    tx_status = tx_block_pool_create(&s_dcs_client.rx_pool,            // pool_ptr
                                     "DCS client pool",                // name_ptr
                                     DCS_TRP_MSG_BLOCK_SIZE,           // block_size
                                     dcs_svc_client_pool_mem,          // pool_start
                                     sizeof(dcs_svc_client_pool_mem)); // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_client.rx_pool, 0, 0);

    dcs_register_client(DCP_CLIENT_ID_DCS, &s_dcs_client);
}

void dcp_svc_client_handle_incoming_msgs(void)
{
    p_trp_msg_t trp_msg = NULL;
    UINT queue_status = 0;

    // may be multiple messages in the queue
    do
    {
        queue_status = tx_queue_receive(&s_dcs_client.rx_queue, &trp_msg, TX_NO_WAIT);
        if ((queue_status == TX_SUCCESS) && (trp_msg != NULL))
        {
            if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
            {
                dcp_svc_client_handle_dcp_msg(trp_msg);
            }
            else
            {
                FPFW_ET_LOG(DcsSvcClientUnexpectedMsg, trp_msg->hdr.trp_msg_id);
            }

            UINT block_status = tx_block_release(trp_msg);
            if (block_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(DcsSvcClientFreeFail, (uintptr_t)trp_msg, block_status);
            }
        }
        else if (queue_status != TX_QUEUE_EMPTY)
        {
            FPFW_ET_LOG(DcsSvcClientQueueFail, (uintptr_t)&s_dcs_client.rx_queue, queue_status);
        }
    } while (queue_status == TX_SUCCESS);
}

void dcp_svc_client_handle_dcp_msg(p_trp_msg_t trp_msg)
{
    printf("DCP Service Client: Handling DCP message 0x%x\n", trp_msg->payload.dcp_msg.hdr.msg_id);
    switch (trp_msg->payload.dcp_msg.hdr.msg_id)
    {

    case DCP_MSG_ID_GET_CAPABILITIES:

        break;

    case DCP_MSG_ID_GET_STATE:

        break;

    case DCP_MSG_ID_GET_SCHEMA:

        break;

    case DCP_MSG_ID_EVENTS_ENABLE_DISABLE:

        break;

    case DCP_MSG_ID_START_STOP:

        break;

    case DCP_MSG_ID_READ_DATA:

        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:

        break;

    case DCP_MSG_ID_RESET:

        break;

    case DCP_MSG_ID_NOTIFICATION:

        break;

    case DCP_MSG_ID_GET_PLAT_INFO:

        break;

    default:
        FPFW_ET_LOG(DcsSvcClientUnexpectedMsg, trp_msg->payload.dcp_msg.hdr.msg_id);
        break;
    }
}