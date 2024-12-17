//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_collection_service.c
 * This file contains the implementation for the Data Collection Service
 */

/*------------- Includes -----------------*/
#include "data_collection_service.h"

#include "data_collection_protocol.h"
#include "data_collection_service_i.h"
#include "dcp_service_client_i.h"
#include "dcs_events_i.h"
#include "tlm_relay_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ALL_EVENT_GROUP_BITS (0xFFFFFFFF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static dcs_context_t s_dcs_context;
/*------------- Functions ----------------*/
static void dcs_thread(ULONG thread_input);

void dcs_init(p_dcs_config_t config)
{
    for (uint16_t id = 0; id < DCP_CLIENT_ID_MAX; id++)
    {
        s_dcs_context.clients[id] = NULL;
    }

    UINT tx_status = tx_thread_create(&s_dcs_context.thread,
                                      "dcs_thread",
                                      dcs_thread,
                                      (ULONG)NULL,
                                      config->thread_config.p_stack,
                                      config->thread_config.stack_size,
                                      config->thread_config.priority,
                                      config->thread_config.priority,
                                      config->thread_config.time_slice_option,
                                      TX_AUTO_START);
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, 0, 0, 0);

    tx_status = tx_queue_create(&s_dcs_context.trp_outbound_queue.queue,
                                "trp outbound queue",
                                1, // number of uint32_t elements
                                s_dcs_context.trp_outbound_queue.mem,
                                sizeof(s_dcs_context.trp_outbound_queue.mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_context.trp_outbound_queue.queue, 0, 0);

    tx_status = tx_block_pool_create(&s_dcs_context.msg_pool.pool,        // pool_ptr
                                     "dcs msg pool",                      // name_ptr
                                     DCS_TRP_MSG_BLOCK_SIZE,              // block_size
                                     s_dcs_context.msg_pool.mem,          // pool_start
                                     sizeof(s_dcs_context.msg_pool.mem)); // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_dcs_context.msg_pool.pool, 0, 0);

    tx_status = tx_event_flags_create(&s_dcs_context.thread_ctrl, "DCS Thread Control");
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, 0, 0, 0);

    tlm_relay_init(&config->trp_icc_config);

    dcp_svc_client_init();
}

fpfw_status_t dcs_register_client(dcp_client_id_t id, p_dcs_client_t client)
{
    if (id >= DCP_CLIENT_ID_MAX)
    {
        FPFW_ET_LOG(DcsInvalidClientId, id);
        return FPFW_STATUS_UNEXPECTED;
    }
    if (s_dcs_context.clients[id] != NULL)
    {
        FPFW_ET_LOG(DcsMultipleRegistrationNotAllowed, id);
        return FPFW_STATUS_ALREADY_INITIALIZED;
    }

    s_dcs_context.clients[id] = client;
    return FPFW_STATUS_SUCCESS;
}

static void dcs_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);
    static ULONG current_bits;

    while (1)
    {
        UINT txStatus =
            tx_event_flags_get(&s_dcs_context.thread_ctrl, ALL_EVENT_GROUP_BITS, TX_OR_CLEAR, &current_bits, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

        if (current_bits & NEW_OUTBOUND_MSG)
        {
            dcs_handle_outbound_msgs();
        }

        if (current_bits & NEW_DCS_CLIENT_MSG)
        {
            dcp_svc_client_handle_incoming_msgs();
        }
    }
}

void dcs_handle_outbound_msgs(void)
{
    p_trp_msg_t trp_msg = NULL;
    UINT queue_status = 0;

    // may be multiple messages in the queue
    do
    {
        queue_status = tx_queue_receive(&s_dcs_context.trp_outbound_queue.queue, &trp_msg, TX_NO_WAIT);
        if ((queue_status == TX_SUCCESS) && (trp_msg != NULL))
        {
            tlm_relay_send_outgoing_msg(trp_msg);

            UINT block_status = tx_block_release(trp_msg);
            if (block_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(DcsSvcClientFreeFail, (uintptr_t)trp_msg, block_status);
            }
        }
        else if (queue_status != TX_QUEUE_EMPTY)
        {
            FPFW_ET_LOG(DcsSvcClientQueueFail, (uintptr_t)&s_dcs_context.trp_outbound_queue.queue, queue_status);
        }
    } while (queue_status == TX_SUCCESS);
}

void dcs_service_client_notification_from_drv_frmwk(void)
{
    UINT txStatus = tx_event_flags_set(&s_dcs_context.thread_ctrl, NEW_DCS_CLIENT_MSG, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void dcs_forward_trp_msg_to_client_from_drv_frmwk(p_trp_msg_t trp_msg)
{
    // leave for debugging
    // printf("dcs svc client source_die_id: %d, source_cpu_id: %d, dest_die_id: %d, dest_cpu_id:%d, "
    //        "dcp_client_id: %d, "
    //        "trp_msg_id: %d\n",
    //        trp_msg->hdr.source_die_id,
    //        trp_msg->hdr.source_cpu_id,
    //        trp_msg->hdr.dest_die_id,
    //        trp_msg->hdr.dest_cpu_id,
    //        trp_msg->hdr.dcp_client_id,
    //        trp_msg->hdr.trp_msg_id);

    bool forward_to_client = true;

    if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
    {
        forward_to_client = dcs_is_valid_dcp_msg_from_drv_frmwk(&trp_msg->payload.dcp_msg);
    }
    else
    {
        forward_to_client = dcs_is_valid_trp_msg_from_drv_frmwk(trp_msg);
    }

    if (forward_to_client)
    {
        fpfw_status_t status =
            dcs_queue_msg_from_drv_frmwk(&s_dcs_context.clients[trp_msg->hdr.dcp_client_id]->rx_pool,
                                         &s_dcs_context.clients[trp_msg->hdr.dcp_client_id]->rx_queue,
                                         trp_msg);

        if (FPFW_STATUS_SUCCEEDED(status))
        {
            if (s_dcs_context.clients[trp_msg->hdr.dcp_client_id]->notify_from_drv_frmwk != NULL)
            {
                s_dcs_context.clients[trp_msg->hdr.dcp_client_id]->notify_from_drv_frmwk();
            }
        }
        else
        {
            forward_to_client = false;

            if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
            {
                trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_INCOMPLETE_HANDLER;
            }
            trp_msg->hdr.trp_msg_status = TRP_STATUS_E_INCOMPLETE_HANDLER;
        }
    }

    if (!forward_to_client)
    {
        // DCP and/or TRP error status values have already been set
        // to reply with error message need to swap source and dest
        dcs_queue_for_outbound_from_drv_frmwk(trp_msg, true);
    }
}

bool dcs_is_valid_dcp_msg_from_drv_frmwk(p_dcp_msg_t dcp_msg)
{
    bool valid = true;
    dcp_msg->hdr.msg_status = DCP_STATUS_SUCCESS;

    if ((dcp_msg->hdr.client_id >= DCP_CLIENT_ID_MAX) || (s_dcs_context.clients[dcp_msg->hdr.client_id] == NULL))
    {
        dcp_msg->hdr.msg_status = DCP_STATUS_E_UNK_CLIENT;
        valid = false;
    }

    if (valid)
    {
        switch (dcp_msg->hdr.msg_id)
        {
        case DCP_MSG_ID_GET_CAPABILITIES:
        case DCP_MSG_ID_GET_STATE:
        case DCP_MSG_ID_GET_SCHEMA:
        case DCP_MSG_ID_READ_DATA:
        case DCP_MSG_ID_RESET:
        case DCP_MSG_ID_GET_PLAT_INFO:
            if (dcp_msg->hdr.payload_size != 0)
            {
                dcp_msg->hdr.msg_status = DCP_STATUS_E_SIZE;
                valid = false;
            }
            break;

        case DCP_MSG_ID_EVENTS_ENABLE_DISABLE:
            if ((dcp_msg->payload.events_enable_disable.number_of_events *
                 sizeof(dcp_msg->payload.events_enable_disable.events[0])) !=
                (dcp_msg->hdr.payload_size - sizeof(dcp_msg->payload.events_enable_disable.number_of_events)))
            {
                dcp_msg->hdr.msg_status = DCP_STATUS_E_SIZE;
                valid = false;
            }
            break;

        case DCP_MSG_ID_START_STOP:
            if (dcp_msg->hdr.payload_size != sizeof(dcp_msg_start_stop_t))
            {
                dcp_msg->hdr.msg_status = DCP_STATUS_E_SIZE;
                valid = false;
            }
            break;

        case DCP_MSG_ID_READ_DATA_COMPLETE:
            if (dcp_msg->hdr.payload_size != sizeof(dcp_msg_read_data_complete_t))
            {
                dcp_msg->hdr.msg_status = DCP_STATUS_E_SIZE;
                valid = false;
            }
            break;

        case DCP_MSG_ID_EVENTS_DISABLE:
        case DCP_MSG_ID_STOP:
        case DCP_MSG_ID_UTC_SYNC:
            dcp_msg->hdr.msg_status = DCP_STATUS_E_DEPRECATED_MSG;
            valid = false;
            break;

        default:
            dcp_msg->hdr.msg_status = DCP_STATUS_E_UNK_MSG;
            valid = false;
            break;
        };
    }

    // invalid messages have the status set and the payload is 0
    // valid message handling update the response header per message
    if (!valid)
    {
        dcp_msg->hdr.payload_size = 0;
    }

    return valid;
}

bool dcs_is_valid_trp_msg_from_drv_frmwk(p_trp_msg_t trp_msg)
{
    bool valid = true;
    trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;

    if ((trp_msg->hdr.dcp_client_id >= DCP_CLIENT_ID_MAX) || (s_dcs_context.clients[trp_msg->hdr.dcp_client_id] == NULL))
    {
        trp_msg->hdr.trp_msg_status = TRP_STATUS_E_UNK_CLIENT;
        valid = false;
    }

    if (valid)
    {
        switch (trp_msg->hdr.trp_msg_id)
        {
        case TRP_MSG_ID_READ_PACKAGE:
        case TRP_MSG_ID_READ_PACKAGE_COMPLETE:
        case TRP_MSG_ID_READ_INTERCORE_BLOCK:
        case TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE:
        case TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE:
            if (trp_msg->hdr.payload_size != sizeof(trp_msg_read_data_t))
            {
                trp_msg->hdr.trp_msg_status = TRP_STATUS_E_SIZE;
                valid = false;
            }
            break;

        default:
            trp_msg->hdr.trp_msg_status = TRP_STATUS_E_UNK_MSG;
            valid = false;
            break;
        }
    }

    // invalid messages have the status set and the payload is 0
    // valid message handling update the response header per message
    if (!valid)
    {
        trp_msg->hdr.payload_size = 0;
    }

    return valid;
}

fpfw_status_t dcs_queue_msg_from_drv_frmwk(TX_BLOCK_POOL* client_block_pool, TX_QUEUE* client_rx_queue, p_trp_msg_t incoming_trp_msg)
{
    p_trp_msg_t client_dest_msg = NULL;
    fpfw_status_t fpfw_status = FPFW_STATUS_FAIL;

    UINT tx_status = tx_block_allocate(client_block_pool, (void**)&client_dest_msg, TX_NO_WAIT);
    if ((tx_status == TX_SUCCESS) && (client_dest_msg != NULL))
    {
        *client_dest_msg = *incoming_trp_msg;
        tx_status = tx_queue_send(client_rx_queue, &client_dest_msg, TX_NO_WAIT);
        if (tx_status == TX_SUCCESS)
        {
            fpfw_status = FPFW_STATUS_SUCCESS;
        }
        else
        {
            fpfw_status = FPFW_STATUS_BUFFER_TOO_SMALL;
            FPFW_ET_LOG(DcsSvcClientQueueFail, (uintptr_t)client_rx_queue, tx_status);

            tx_status = tx_block_release(client_dest_msg);
            if (tx_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(DcsSvcClientFreeFail, (uintptr_t)client_dest_msg, tx_status);
            }
        }
    }
    else
    {
        fpfw_status = FPFW_STATUS_OUT_OF_MEMORY;
        FPFW_ET_LOG(DcsSvcClientAllocateFail, (uintptr_t)client_block_pool, tx_status);
    }

    return fpfw_status;
}

void dcs_queue_for_outbound_from_drv_frmwk(p_trp_msg_t trp_msg, bool swap_source_dest)
{
    if (swap_source_dest)
    {
        uint8_t temp = trp_msg->hdr.source_die_id;
        trp_msg->hdr.source_die_id = trp_msg->hdr.dest_die_id;
        trp_msg->hdr.dest_die_id = temp;

        temp = trp_msg->hdr.source_cpu_id;
        trp_msg->hdr.source_cpu_id = trp_msg->hdr.dest_cpu_id;
        trp_msg->hdr.dest_cpu_id = temp;
    }

    dcs_queue_msg_from_drv_frmwk(&s_dcs_context.msg_pool.pool, &s_dcs_context.trp_outbound_queue.queue, trp_msg);

    // wakeup the DCS thread to handle the new message
    // do so even if queue fails to wake up thread for recovery purposes
    // errors have already been logged in dcs_queue_msg_from_drv_frmwk
    UINT txStatus = tx_event_flags_set(&s_dcs_context.thread_ctrl, NEW_OUTBOUND_MSG, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}