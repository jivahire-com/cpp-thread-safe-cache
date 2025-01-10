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
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DCS_MAX_DCS_SVC_ClIENT_MESSAGES (DCP_MSG_ID_MAX)
#define DCS_SVC_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + DCS_TRP_MSG_BLOCK_SIZE) * DCS_MAX_DCS_SVC_ClIENT_MESSAGES)

#define MS_TO_TX_TICKS(ms)     (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)
#define DCP_RESET_CMD_DELAY_MS (MS_TO_TX_TICKS(1000))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

p_trp_msg_t dcs_svc_client_queue_mem[DCS_MAX_DCS_SVC_ClIENT_MESSAGES];
uint8_t dcs_svc_client_pool_mem[DCS_SVC_CLIENT_BLOCK_POOL_SIZE];

/*-- Declarations (Statics and globals) --*/
static dcs_client_t s_dcs_client = {
    .notify_from_drv_frmwk = dcs_service_client_notification_from_drv_frmwk,
};

static p_dcp_msg_ifwi_version_t s_ifwi_version = NULL;
/*------------- Functions ----------------*/

void dcp_svc_client_init(p_dcp_msg_ifwi_version_t ifwi_version)
{
    FPFW_RUNTIME_ASSERT(ifwi_version != NULL);
    s_ifwi_version = ifwi_version;

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

    dcs_client_register(DCP_CLIENT_ID_DCS, &s_dcs_client);
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

                UINT block_status = tx_block_release(trp_msg);
                if (block_status != TX_SUCCESS)
                {
                    FPFW_ET_LOG(DcsSvcClientFreeFail, (uintptr_t)trp_msg, block_status);
                }
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
    switch (trp_msg->payload.dcp_msg.hdr.msg_id)
    {

    case DCP_MSG_ID_GET_CAPABILITIES: {
        p_dcp_msg_get_caps_t get_caps = &trp_msg->payload.dcp_msg.payload.get_caps;
        get_caps->caps.as_uint32 = UINT32_MAX;
        get_caps->caps.reserved = 0;
        get_caps->caps.DCP_MSG_ID_EVENTS_DISABLE = 0;
        get_caps->caps.DCP_MSG_ID_STOP = 0;
        get_caps->caps.DCP_MSG_ID_UTC_SYNC = 0;

        trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_get_caps_t);
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;
    }

    case DCP_MSG_ID_GET_SCHEMA:
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2145303
        break;

    case DCP_MSG_ID_RESET: {
        if (dcs_is_primary_instance())
        {
            // notify off chip secondary instances
            trp_msg->hdr.broadcast_type = TRP_BROADCAST_TO_SEC_MCP_THEN_TO_SCP;
            dcs_send_outgoing_msg(trp_msg, false);
        }
        trp_msg->hdr.broadcast_type = TRP_BROADCAST_NONE;

        // notify on chip clients as reset is intended to propagate to all clients
        dcs_forward_trp_msg_to_all_non_dcp_svc_clients(trp_msg);

        // delay to allow clients to process the reset message
        tx_thread_sleep(DCP_RESET_CMD_DELAY_MS);

        // flushing the queue is going to loop through the queue and free the blocks in the queue
        // the block that this message is in has already been popped off the queue and will be freed below
        // after sending the response
        dcs_client_flush_incoming_queue(DCP_CLIENT_ID_DCS);
        dcs_flush_outgoing_queue();

        // setup reply back to host
        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;
    }

    case DCP_MSG_ID_GET_PLAT_INFO: {
        p_dcp_msg_get_plat_info_t get_plat_info = &trp_msg->payload.dcp_msg.payload.get_plat_info;
        get_plat_info->dcp_ver_major = DCP_PROTOCOL_VERSION_MAJOR;
        get_plat_info->dcp_ver_minor = DCP_PROTOCOL_VERSION_MINOR;
        get_plat_info->dcp_ver_patch = DCP_PROTOCOL_VERSION_PATCH;
        get_plat_info->plat_id = DCP_PLATFORM_COBALT_200;
        get_plat_info->ifwi_ver = *s_ifwi_version;

        trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_get_plat_info_t);
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;
    }

    case DCP_MSG_ID_GET_STATE:
    case DCP_MSG_ID_EVENTS_ENABLE_DISABLE:
    case DCP_MSG_ID_START_STOP:
    case DCP_MSG_ID_READ_DATA:
    case DCP_MSG_ID_READ_DATA_COMPLETE:
    case DCP_MSG_ID_NOTIFICATION:
    default:
        FPFW_ET_LOG(DcsSvcClientUnexpectedMsg, trp_msg->payload.dcp_msg.hdr.msg_id);

        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_E_UNSUPPORTED_MSG;
        break;
    }

    // reply back to the host if primary instance, otherwise let broadcast messages drop
    if (dcs_is_primary_instance())
    {
        if (trp_msg->payload.dcp_msg.hdr.msg_status < 0)
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
        }
        else
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
        }

        trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + trp_msg->payload.dcp_msg.hdr.payload_size;

        dcs_send_outgoing_msg(trp_msg, true);
    }

    UINT block_status = tx_block_release(trp_msg);
    if (block_status != TX_SUCCESS)
    {
        FPFW_ET_LOG(DcsSvcClientFreeFail, (uintptr_t)trp_msg, block_status);
    }
}
