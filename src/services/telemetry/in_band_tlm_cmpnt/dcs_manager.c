//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_manager.c
 * Manage data collection service interaction.
 */

/*------------- Includes -----------------*/
#include "dcs_manager_i.h"
#include "ddr_manager_i.h"
#include "package_creation_i.h"

#include <data_collection_protocol.h>
#include <data_collection_service.h>
#include <exec_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt.h>
#include <telemetry_events_i.h>

/*-- Symbolic Constant Macros (defines) --*/

#define PWR_TLM_MAX_TRP_MESSAGES (MAX_PENDING_PACKAGES * 2)
#define PWR_TLM_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + DCS_TRP_MSG_BLOCK_SIZE) * PWR_TLM_MAX_TRP_MESSAGES)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
tlm_package_t dcs_pkg_pending_buffer[MAX_PENDING_PACKAGES];
TX_QUEUE dcs_pkg_pending_queue;

p_trp_msg_t pwr_tlm_client_queue_mem[PWR_TLM_MAX_TRP_MESSAGES];
uint8_t pwr_tlm_client_pool_mem[PWR_TLM_CLIENT_BLOCK_POOL_SIZE];

/*-- Declarations (Statics and globals) --*/
static dcs_client_t s_pwr_tlm_dcs_client = {
    .notify_from_drv_frmwk = exec_tlm_cmpnt_notify_new_in_band_dcs_message,
};

/*------------- Functions ----------------*/

void dcs_manager_init(void)
{
    UINT tx_status = tx_queue_create(&dcs_pkg_pending_queue,
                                     "dcs_pkg_pending_queue",
                                     (sizeof(tlm_package_t) + 3) / sizeof(uint32_t), // number of uint32_t elements
                                     dcs_pkg_pending_buffer,
                                     sizeof(dcs_pkg_pending_buffer));

    FpFwAssertWithArgs(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&dcs_pkg_pending_queue, 0, 0);

    tx_status = tx_queue_create(&s_pwr_tlm_dcs_client.rx_queue,
                                "Pwr Tlm DCS client queue",
                                1, // number of uint32_t elements
                                pwr_tlm_client_queue_mem,
                                sizeof(pwr_tlm_client_queue_mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_dcs_client.rx_queue, 0, 0);

    tx_status = tx_block_pool_create(&s_pwr_tlm_dcs_client.rx_pool,    // pool_ptr
                                     "Pwr Tlm client pool",            // name_ptr
                                     DCS_TRP_MSG_BLOCK_SIZE,           // block_size
                                     pwr_tlm_client_pool_mem,          // pool_start
                                     sizeof(pwr_tlm_client_pool_mem)); // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_dcs_client.rx_pool, 0, 0);

    fpfw_status_t status = dcs_client_register(DCP_CLIENT_ID_PWR_INST_TELEM, &s_pwr_tlm_dcs_client);
    FPFW_RUNTIME_ASSERT_EXT(status == FPFW_STATUS_SUCCESS, status, 0, 0, 0);
}

void in_band_tlm_cmpnt_handle_incoming_dcs_msgs(void)
{
    p_trp_msg_t trp_msg = NULL;
    UINT queue_status = 0;

    // may be multiple messages in the queue
    do
    {
        queue_status = tx_queue_receive(&s_pwr_tlm_dcs_client.rx_queue, &trp_msg, TX_NO_WAIT);
        if ((queue_status == TX_SUCCESS) && (trp_msg != NULL))
        {
            if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
            {
                dcs_manager_handle_dcp_msg(trp_msg);
            }
            else
            {
                dcs_manager_handle_trp_msg(trp_msg);
            }

            UINT block_status = tx_block_release(trp_msg);
            if (block_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(DcsMgrClientFreeFail, (uintptr_t)trp_msg, block_status);
            }
        }
        else if (queue_status != TX_QUEUE_EMPTY)
        {
            FPFW_ET_LOG(DcsMgrClientQueueFail, (uintptr_t)&s_pwr_tlm_dcs_client.rx_queue, queue_status);
        }
    } while (queue_status == TX_SUCCESS);
}

void dcs_manager_handle_dcp_msg(p_trp_msg_t trp_msg)
{
    // the telemetry service runs on all MCP's.  The architecture is such that the Host only interacts
    // with Die 0 MCP. Any commands that change operating state, need to be forwarded to the secondary MCP.
    // Status requests only need to be replied from the Primary MCP since the secondary MCP tracks the state
    // of the system. DCP reads are only handled by the primary MCP, so those also are not forwarded.
    bool forward = false;

    // uncomment for debug
    // printf("pwrt tlm client source_die_id: %d, source_cpu_id: %d, dest_die_id: %d, dest_cpu_id:%d, "
    //        "dcp_client_id: %d, "
    //        "trp_msg_id: %d\n",
    //        trp_msg->hdr.source_die_id,
    //        trp_msg->hdr.source_cpu_id,
    //        trp_msg->hdr.dest_die_id,
    //        trp_msg->hdr.dest_cpu_id,
    //        trp_msg->hdr.dcp_client_id,
    //        trp_msg->hdr.trp_msg_id);

    switch (trp_msg->payload.dcp_msg.hdr.msg_id)
    {
    case DCP_MSG_ID_GET_STATE:
        trp_msg->payload.dcp_msg.payload.get_state.state =
            exec_tlm_cmpnt_is_telemetry_enabled() ? DCP_CLIENT_STATE_RUNNING : DCP_CLIENT_STATE_STOPPED;
        trp_msg->payload.dcp_msg.hdr.payload_size = sizeof(dcp_msg_get_client_state_t);
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_EVENTS_ENABLE_DISABLE:
        dcs_manager_handle_record_enable_disable(trp_msg);

        if (trp_msg->payload.dcp_msg.hdr.msg_status == DCP_STATUS_SUCCESS)
        {
            forward = true;
        }
        break;

    case DCP_MSG_ID_START_STOP:
        forward = true;

        bool enable = (trp_msg->payload.dcp_msg.payload.start_stop.state == DCP_START_STOP_STATE_START) ? true : false;
        exec_tlm_cmpnt_enable_disable_telemetry(enable);

        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_READ_DATA:
        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_INCOMPLETE_HANDLER;

        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:
        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_INCOMPLETE_HANDLER;
        break;

    case DCP_MSG_ID_RESET:

        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_INCOMPLETE_HANDLER;

        // forward = true;
        // // flushing the queue is going to loop through the queue and free the blocks in the queue
        // // the block that this message is in has already been popped off the queue and will be freed below
        // // after sending the response
        // dcs_client_flush_incoming_queue(DCP_CLIENT_ID_PWR_INST_TELEM);

        // trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        // trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    default:
        FPFW_ET_LOG(DcsMgrClientUnexpectedMsg, trp_msg->payload.dcp_msg.hdr.msg_id);

        trp_msg->payload.dcp_msg.hdr.payload_size = 0;
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_E_UNSUPPORTED_MSG;
        break;
    };

    // reply back to the host if primary instance, otherwise let forwarded messages drop
    if (dcs_is_primary_instance())
    {
        if (trp_msg->payload.dcp_msg.hdr.msg_status < 0)
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
        }
        else
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
            if (forward)
            {
                // forward to secondary MCP
                dcs_client_send_trp_msg(trp_msg, TRP_BROADCAST_PRIM_MCP_TO_SEC_MCP_ONLY);
            }
        }

        trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + trp_msg->payload.dcp_msg.hdr.payload_size;

        dcs_client_send_trp_response(trp_msg);
    }
}

void dcs_manager_handle_trp_msg(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);

    // uncomment for debug
    // printf("pwrt tlm client source_die_id: %d, source_cpu_id: %d, dest_die_id: %d, dest_cpu_id:%d, "
    //        "dcp_client_id: %d, "
    //        "trp_msg_id: %d\n",
    //        trp_msg->hdr.source_die_id,
    //        trp_msg->hdr.source_cpu_id,
    //        trp_msg->hdr.dest_die_id,
    //        trp_msg->hdr.dest_cpu_id,
    //        trp_msg->hdr.dcp_client_id,
    //        trp_msg->hdr.trp_msg_id);
}

void dcs_manager_queue_tlm_package(uintptr_t pkg_location, size_t pkg_size)
{
    tlm_package_t tlm_pkg;
    UINT tx_status;

    // TODO: dcs manager will be implemented in future task
    // //https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2023646 this temporary implementation just adds
    // the package to the pending queue, and next time around all packages are free'd
    do
    {
        tx_status = tx_queue_receive(&dcs_pkg_pending_queue, &tlm_pkg, TX_NO_WAIT);
        if (tx_status == TX_SUCCESS)
        {
            ddr_manager_deallocate_mem(&tlm_pkg.pkg_location);
        }
    } while (tx_status == TX_SUCCESS);

    tlm_pkg.pkg_location = pkg_location;
    tlm_pkg.pkg_size = pkg_size;

    tx_status = tx_queue_send(&dcs_pkg_pending_queue, &tlm_pkg, TX_NO_WAIT);
    if (tx_status != TX_SUCCESS)
    {
        // failed to pend, deallocate the memory
        ddr_manager_deallocate_mem(&tlm_pkg.pkg_location);
    }
}

void dcs_manager_handle_record_enable_disable(p_trp_msg_t trp_msg)
{
    trp_msg->payload.dcp_msg.hdr.payload_size = 0;
    trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_PARAM;

    if (trp_msg->payload.dcp_msg.payload.events_enable_disable.number_of_events > DCP_MAX_ENABLE_DISABLE_EVENTS)
    {
        return;
    }

    // validate first
    for (uint16_t i = 0; i < trp_msg->payload.dcp_msg.payload.events_enable_disable.number_of_events; i++)
    {
        uint16_t provider_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].provider_id;
        uint16_t event_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].event_id;

        if (provider_id == EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA)
        {
            if (event_id >= POWER_TELEMETRY_ELEMENT_ID_MAX)
            {
                FPFW_ET_LOG(DcsMgrInvalidEventEnableDisable, provider_id, event_id);
                return;
            }
        }
        else if (provider_id == EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA)
        {
            if (event_id >= INST_TELEMETRY_ELEMENT_ID_MAX)
            {
                FPFW_ET_LOG(DcsMgrInvalidEventEnableDisable, provider_id, event_id);
                return;
            }
        }
        else
        {
            FPFW_ET_LOG(DcsMgrInvalidEventEnableDisable, provider_id, event_id);
            return;
        }
    }

    trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;

    for (uint16_t i = 0; i < trp_msg->payload.dcp_msg.payload.events_enable_disable.number_of_events; i++)
    {
        uint16_t provider_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].provider_id;
        uint16_t event_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].event_id;
        dcp_events_enable_state_t state = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].state;
        bool enable_record = (state == DCP_EVENTS_ENABLE_STATE_DISABLE) ? false : true;

        if (provider_id == EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA)
        {
            package_create_enable_disable_pwr_record(event_id, enable_record);
        }
        else
        {
            // validation above ensures this is EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA
            package_create_enable_disable_inst_record(event_id, enable_record);
        }
    }
}