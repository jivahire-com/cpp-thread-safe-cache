//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_manager.c
 * Manage data collection service interaction.
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "mts_manager_i.h"
#include "package_creation_i.h"

#include <FpFwUtils.h>
#include <data_collection_protocol.h>
#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt.h>
#include <message_transfer_service.h>
#include <pwr_tlm_core_exchange.h>
#include <telemetry_events_i.h>

/*-- Symbolic Constant Macros (defines) --*/

#define PWR_TLM_MAX_TRP_MESSAGES (MAX_PENDING_PACKAGES * 2)
#define PWR_TLM_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + MAX_TRP_MSG_BLOCK_SIZE) * PWR_TLM_MAX_TRP_MESSAGES)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
tlm_package_t mts_active_pkg_buffer[MAX_PENDING_PACKAGES];

// Lists are only modified from telemetry service thread, so thread safety is not a concern.
// list of free tlm_package_t to allocate for a new package
FPFW_LIST_HANDLE pkg_free_list;

// list usage depends on which die tlm service is running on.
// On Die 0 MCP, this list will contain active packages from local as well as secondary MCP's.
// On Die 1 MCP, this list will contain active packages from local MCP only. Packages are immediately sent
// to Die 0, but the list is maintained on the local MCP to clear all packages on a DCP reset command.
FPFW_LIST_HANDLE pkg_active_list;

p_tlm_package_t in_flight_tlm_pkg;

// for dcs client subscription
p_trp_msg_t pwr_tlm_client_queue_mem[PWR_TLM_MAX_TRP_MESSAGES];
uint8_t pwr_tlm_client_pool_mem[PWR_TLM_CLIENT_BLOCK_POOL_SIZE];
bool mpam_vm_mem_reporting_knob_enable = false;

/*-- Declarations (Statics and globals) --*/
static mts_client_t s_pwr_tlm_mts_client = {
    .notify_from_drv_frmwk = exec_tlm_cmpnt_notify_new_in_band_mts_message,
};

/*------------- Functions ----------------*/
void mts_manager_init(bool mpam_vm_mem_enable)
{
    mpam_vm_mem_reporting_knob_enable = mpam_vm_mem_enable;
    // create a list of free packages
    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    UINT tx_status = tx_queue_create(&s_pwr_tlm_mts_client.rx_queue,
                                     "Pwr Tlm MTS client queue",
                                     1, // number of uint32_t elements
                                     pwr_tlm_client_queue_mem,
                                     sizeof(pwr_tlm_client_queue_mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_mts_client.rx_queue, 0, 0);

    tx_status = tx_block_pool_create(&s_pwr_tlm_mts_client.rx_pool,    // pool_ptr
                                     "Pwr Tlm client pool",            // name_ptr
                                     MAX_TRP_MSG_BLOCK_SIZE,           // block_size
                                     pwr_tlm_client_pool_mem,          // pool_start
                                     sizeof(pwr_tlm_client_pool_mem)); // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_mts_client.rx_pool, 0, 0);

    mts_client_register(MTS_CLIENT_ID_PWR_INST_TELEM, &s_pwr_tlm_mts_client);
}

void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void)
{
    p_trp_msg_t trp_msg = NULL;
    UINT queue_status = 0;

    // may be multiple messages in the queue
    do
    {
        queue_status = tx_queue_receive(&s_pwr_tlm_mts_client.rx_queue, &trp_msg, TX_NO_WAIT);
        if ((queue_status == TX_SUCCESS) && (trp_msg != NULL))
        {
            // debug level trace
            FPFW_ET_LOG(TlmSvcDebugIncomingTrpMsg,
                        GET_BASE_SEQ_NUM(trp_msg->hdr.source_seq_num.as_uint16),
                        trp_msg->hdr.src_node.die_id,
                        trp_msg->hdr.src_node.core_id,
                        trp_msg->hdr.dest_node.die_id,
                        trp_msg->hdr.dest_node.core_id,
                        trp_msg->hdr.mts_client_id,
                        trp_msg->hdr.trp_msg_id,
                        trp_msg->hdr.trp_msg_status,
                        GET_INT_CMD_BIT(trp_msg->hdr.source_seq_num.as_uint16)); // remove init_cmd bit

            if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
            {
                // debug level trace
                FPFW_ET_LOG(TlmSvcDebugIncomingDcpMsg,
                            trp_msg->payload.dcp_msg.hdr.client_id,
                            trp_msg->payload.dcp_msg.hdr.msg_id,
                            trp_msg->payload.dcp_msg.hdr.seq_num);

                mts_manager_handle_dcp_msg(trp_msg);
            }
            else
            {
                mts_manager_handle_trp_msg(trp_msg);
            }

            UINT block_status = tx_block_release(trp_msg);
            if (block_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(MtsMgrClientFreeFail, (uintptr_t)trp_msg, block_status);
            }
        }
        else if (queue_status != TX_QUEUE_EMPTY)
        {
            FPFW_ET_LOG(MtsMgrClientQueueFail, (uintptr_t)&s_pwr_tlm_mts_client.rx_queue, queue_status);
        }
    } while (queue_status == TX_SUCCESS);
}

void mts_manager_handle_dcp_msg(p_trp_msg_t trp_msg)
{
    // the telemetry service runs on all MCP's.  The architecture is such that the Host only interacts
    // with Die 0 MCP. Any commands that change operating state, need to be forwarded to the secondary MCP.
    // Status requests only need to be replied from the Primary MCP since the secondary MCP tracks the state
    // of the system. DCP reads are only handled by the primary MCP, so those also are not forwarded.
    bool primary_instance = mts_is_primary_instance();
    bool forward = false;
    uint16_t response_dcp_payload_size = 0;
    // overwrite if supported. NOTE: if not primary instance, no reply is sent
    trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_E_UNSUPPORTED_MSG;

    switch (trp_msg->payload.dcp_msg.hdr.msg_id)
    {
    case DCP_MSG_ID_GET_CAPABILITIES:
        if (primary_instance)
        {
            p_dcp_msg_get_caps_t get_caps = &trp_msg->payload.dcp_msg.payload.get_caps;
            get_caps->caps.as_uint32 = 0;
            get_caps->caps.DCP_MSG_ID_GET_CAPABILITIES = 1;
            get_caps->caps.DCP_MSG_ID_GET_STATE = 1;
            get_caps->caps.DCP_MSG_ID_EVENTS_ENABLE_DISABLE = 1;
            get_caps->caps.DCP_MSG_ID_START_STOP = 1;
            get_caps->caps.DCP_MSG_ID_READ_DATA = 1;
            get_caps->caps.DCP_MSG_ID_READ_DATA_COMPLETE = 1;
            get_caps->caps.DCP_MSG_ID_RESET = 1;

            response_dcp_payload_size = sizeof(dcp_msg_get_caps_t);
            trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        }
        break;

    case DCP_MSG_ID_GET_STATE:

        if (primary_instance)
        {
            trp_msg->payload.dcp_msg.payload.get_state.state =
                exec_tlm_cmpnt_is_telemetry_publishing_enabled() ? DCP_CLIENT_STATE_RUNNING : DCP_CLIENT_STATE_STOPPED;
            response_dcp_payload_size = sizeof(dcp_msg_get_client_state_t);
            trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        }
        break;

    case DCP_MSG_ID_EVENTS_ENABLE_DISABLE:
        forward = true;
        // trp_msg->payload.dcp_msg.hdr.msg_status is set in the function below
        mts_manager_handle_record_enable_disable(trp_msg);
        break;

    case DCP_MSG_ID_START_STOP:
        forward = false; // will not forward command to secondary MCP, will forward state change instead

        if (trp_msg->payload.dcp_msg.payload.start_stop.state == DCP_START_STOP_STATE_START)
        {
            exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_PUBLISHING);
        }
        else
        {
            exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_COLLECTING_DATA);
        }

        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    case DCP_MSG_ID_READ_DATA:
        if (primary_instance)
        {
            // trp_msg->payload.dcp_msg.hdr.msg_status is set in the function below
            mts_manager_handle_read_msg(trp_msg);

            if (trp_msg->payload.dcp_msg.hdr.msg_status >= DATA_COLLECTION_RD_DATA_VALID_LAST)
            {
                response_dcp_payload_size = sizeof(dcp_msg_read_data_t);
            }
            else
            {
                response_dcp_payload_size = 0;
            }
        }
        break;

    case DCP_MSG_ID_READ_DATA_COMPLETE:
        // trp_msg->payload.dcp_msg.hdr.msg_status is set in the function below
        mts_manager_handle_read_complete_msg(trp_msg);
        break;

    case DCP_MSG_ID_RESET:
        forward = false; // will not forward command to secondary MCP, will forward state change instead

        exec_tlm_cmpnt_change_telemetry_mode(TLM_OP_MODE_COLLECTING_DATA);
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
        break;

    default:
        FPFW_ET_LOG(MtsMgrClientUnexpectedDcpMsg, trp_msg->payload.dcp_msg.hdr.msg_id);

        break;
    };

    // reply back to the host if primary instance, otherwise let forwarded messages drop
    if (primary_instance)
    {
        if (trp_msg->payload.dcp_msg.hdr.msg_status < 0)
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_E_DCP_ERROR;
            trp_msg->payload.dcp_msg.hdr.payload_size = 0; // no payload for errors and not forwarding
        }
        else
        {
            trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
            if (forward)
            {
                // forward to secondary MCP
                mts_client_forward_trp_msg(trp_msg, TRP_BROADCAST_PRIM_TO_SEC_PEER_ONLY);
            }
        }

        // if incoming message was validated and required to be forwarded, the forward message above needs to
        // retain the original payload size.
        trp_msg->payload.dcp_msg.hdr.payload_size = response_dcp_payload_size;
        trp_msg->hdr.payload_size = sizeof(dcp_msg_hdr_t) + trp_msg->payload.dcp_msg.hdr.payload_size;

        mts_client_send_trp_response(trp_msg);
    }
}

void mts_manager_handle_trp_msg(p_trp_msg_t trp_msg)
{
    switch (trp_msg->hdr.trp_msg_id)
    {
    case TRP_MSG_ID_PACKAGE_NOTIFICATION: {
        p_tlm_package_t tlm_pkg = mts_manager_get_pkg_from_free_list();
        if (tlm_pkg != NULL)
        {
            tlm_pkg->pkg = trp_msg->payload.package_notification;
            mts_manager_add_tlm_package_to_active_list(tlm_pkg);
            break;
        }

        // unable to store, send complete back to sender to free memory
        tlm_package_t rsp_pkg;
        rsp_pkg.pkg = trp_msg->payload.package_notification;
        mts_manager_send_trp_read_complete(&rsp_pkg);
        break;
    }

    case TRP_MSG_ID_READ_PACKAGE_COMPLETE: {
        tlm_package_t remove_pkg;
        remove_pkg.pkg = trp_msg->payload.package_notification;
        mts_manager_free_tlm_package_from_secondary_mcp(&remove_pkg);
        break;
    }

    case TRP_MSG_ID_CLIENT_DEFINED: {
        p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg->payload.client_msg;
        switch (tlm_client_msg->cmd)
        {
        case TLM_CLIENT_CMD_SET_MODE_PUSH:
            exec_tlm_cmpnt_change_telemetry_mode(tlm_client_msg->payload.mode);
            break;

        case TLM_CLIENT_CMD_GEN_PWR_PACKAGE_PRIM_MCP_2_SEC_MCP_PUSH:
            data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core();
            break;

        default:
            FPFW_ET_LOG(MtsMgrClientUnexpectedCmd, tlm_client_msg->cmd);
        }
        break;
    }

    default:
        FPFW_ET_LOG(MtsMgrClientUnexpectedTrpMsg, trp_msg->hdr.trp_msg_id);
        break;
    }
}

void mts_manager_queue_tlm_package(uintptr_t atu_mapped_location, size_t pkg_size)
{
    p_tlm_package_t tlm_pkg = mts_manager_get_pkg_from_free_list();

    if (tlm_pkg == NULL)
    {
        ddr_manager_deallocate_mem(&atu_mapped_location);
        return;
    }

    uint8_t die_id = mts_get_this_die_id();
    tlm_pkg->pkg.source_die_id = die_id;
    tlm_pkg->pkg.source_core_id = mts_get_this_core_id();

    tlm_pkg->pkg.local_mmap_addr = atu_mapped_location;
    tlm_pkg->pkg.phy_addr_offset = TELEMETRY_GET_DDR_OFFSET(die_id, atu_mapped_location);
    tlm_pkg->pkg.pkg_size = pkg_size;
    tlm_pkg->pkg.crc = fpfw_crc32(0, (void*)atu_mapped_location, pkg_size);

    mts_manager_add_tlm_package_to_active_list(tlm_pkg);
}

void mts_manager_add_tlm_package_to_active_list(p_tlm_package_t tlm_pkg)
{
    bool notify_host = false;
    if (mts_is_primary_instance() == true)
    {
        if (FpFwListIsEmpty(&pkg_active_list))
        {
            notify_host = true;
        }
    }
    else
    {
        // send to primary MCP
        mts_manager_send_trp_pkg_notification_to_primary(tlm_pkg);
    }

    FPFW_ET_LOG(MtsMgrAddPkgToActiveList,
                tlm_pkg->pkg.local_mmap_addr,
                tlm_pkg->pkg.source_die_id,
                tlm_pkg->pkg.source_core_id);
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg->list_entry);

    if (notify_host)
    {
        mts_client_send_dcp_notification(MTS_CLIENT_ID_PWR_INST_TELEM, DCP_NOTIFICATION_TYPE_DATA_AVAILABLE);
    }
}

void mts_manager_send_trp_pkg_notification_to_primary(p_tlm_package_t tlm_pkg)
{
    mts_manager_send_trp_package_helper(tlm_pkg, TRP_MSG_ID_PACKAGE_NOTIFICATION, MTS_PLATFORM_PRIMARY_DIE_ID, MTS_PLATFORM_PRIMARY_CORE_ID);
}

void mts_manager_send_trp_read_complete(p_tlm_package_t tlm_pkg)
{
    mts_manager_send_trp_package_helper(tlm_pkg,
                                        TRP_MSG_ID_READ_PACKAGE_COMPLETE,
                                        tlm_pkg->pkg.source_die_id,
                                        tlm_pkg->pkg.source_core_id);
}

void mts_manager_send_trp_package_helper(p_tlm_package_t tlm_pkg, trp_msg_id_t msg_id, uint8_t dest_die_id, uint8_t dest_core_id)
{
    trp_msg_t trp_msg = {0};
    trp_msg.hdr.src_node.die_id = mts_get_this_die_id();
    trp_msg.hdr.src_node.core_id = mts_get_this_core_id();
    trp_msg.hdr.dest_node.die_id = dest_die_id;
    trp_msg.hdr.dest_node.core_id = dest_core_id;
    trp_msg.hdr.mts_client_id = MTS_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = msg_id;
    trp_msg.hdr.trp_msg_status = TRP_STATUS_SUCCESS;
    trp_msg.hdr.broadcast_type = TRP_BROADCAST_NONE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_pkg_rsp_t);
    trp_msg.payload.package_notification = tlm_pkg->pkg;

    // debug level trace
    FPFW_ET_LOG(TlmSvcDebugOutgoingTrpMsg,
                GET_BASE_SEQ_NUM(trp_msg.hdr.source_seq_num.as_uint16),
                trp_msg.hdr.src_node.die_id,
                trp_msg.hdr.src_node.core_id,
                trp_msg.hdr.dest_node.die_id,
                trp_msg.hdr.dest_node.core_id,
                trp_msg.hdr.mts_client_id,
                trp_msg.hdr.trp_msg_id,
                trp_msg.hdr.trp_msg_status,
                GET_INT_CMD_BIT(trp_msg.hdr.source_seq_num.as_uint16)); // remove init_cmd bit

    // api copies message, ok to use stack memory
    mts_client_send_new_trp_msg(&trp_msg);
}

void mts_manager_handle_record_enable_disable(p_trp_msg_t trp_msg)
{
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
                FPFW_ET_LOG(MtsMgrInvalidEventEnableDisable, provider_id, event_id);
                return;
            }
        }
        else if (provider_id == EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA)
        {
            if (event_id >= INST_TELEMETRY_ELEMENT_ID_MAX)
            {
                FPFW_ET_LOG(MtsMgrInvalidEventEnableDisable, provider_id, event_id);
                return;
            }
        }
        else
        {
            FPFW_ET_LOG(MtsMgrInvalidEventEnableDisable, provider_id, event_id);
            return;
        }
    }

    trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;

    tlm_scp_record_enables_t scp_enables = {0};
    bool update_scp_records = false;

    for (uint16_t i = 0; i < trp_msg->payload.dcp_msg.payload.events_enable_disable.number_of_events; i++)
    {
        uint16_t provider_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].provider_id;
        uint16_t event_id = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].event_id;
        dcp_events_enable_state_t state = trp_msg->payload.dcp_msg.payload.events_enable_disable.events[i].state;
        bool enable_record = (state == DCP_EVENTS_ENABLE_STATE_DISABLE) ? false : true;

        if (provider_id == EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA)
        {
            // scp needs notification as to whether to process these records
            if (event_id == POWER_TELEMETRY_ELEMENT_CORE_DROOPS)
            {
                scp_enables.record.drop_count_en = enable_record ? 1 : 0;
                update_scp_records = true;
            }
            else if ((event_id == POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER) && mpam_vm_mem_reporting_knob_enable)
            {
                scp_enables.record.vm_memory_pwr_en = enable_record ? 1 : 0;
                update_scp_records = true;
            }
            else if (event_id == POWER_TELEMETRY_ELEMENT_CORE_AGING)
            {
                scp_enables.record.core_aging_en = enable_record ? 1 : 0;
                update_scp_records = true;
            }

            package_create_enable_disable_pwr_record(event_id, enable_record);
        }
        else
        {
            // validation above ensures this is EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA
            package_create_enable_disable_inst_record(event_id, enable_record);
        }
    }

    if (update_scp_records)
    {
        mts_manager_send_record_enables_to_scp(&scp_enables);
    }
}

void mts_manager_handle_read_msg(p_trp_msg_t trp_msg)
{
    trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;

    if (FpFwListIsEmpty(&pkg_active_list))
    {
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_NONE;
        return;
    }

    // if not in flight, get the next package from the active list
    // if it's not null, it could be that the host didn't send a read complete,
    // or that our read response failed to send.  In either case, we will resend the same package
    if (in_flight_tlm_pkg == NULL)
    {
        PFPFW_LIST_ENTRY entry = FpFwListRemoveHead(&pkg_active_list);
        in_flight_tlm_pkg = CONTAINING_RECORD(entry, tlm_package_t, list_entry);
    }

    trp_msg->payload.dcp_msg.payload.read_data.physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
    trp_msg->payload.dcp_msg.payload.read_data.physical_buffer_size = IB_TELEMETRY_DDR_TOTAL_SIZE;
    trp_msg->payload.dcp_msg.payload.read_data.rd_data_addr_offset = in_flight_tlm_pkg->pkg.phy_addr_offset;
    trp_msg->payload.dcp_msg.payload.read_data.rd_data_size = in_flight_tlm_pkg->pkg.pkg_size;
    trp_msg->payload.dcp_msg.payload.read_data.crc = in_flight_tlm_pkg->pkg.crc;

    if (FpFwListIsEmpty(&pkg_active_list))
    {
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_VALID_LAST;
    }
    else
    {
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_VALID_MORE;
    }
}

void mts_manager_handle_read_complete_msg(p_trp_msg_t trp_msg)
{
    if (in_flight_tlm_pkg == NULL)
    {
        // no package to complete
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_BUFFER_DISCARD;
        return;
    }

    if (trp_msg->payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset != in_flight_tlm_pkg->pkg.phy_addr_offset)
    {
        // doesn't match the package in flight
        trp_msg->payload.dcp_msg.hdr.msg_status = DCP_STATUS_E_PARAM;
        return;
    }

    mts_manager_free_tlm_package_from_primary_mcp(in_flight_tlm_pkg);
    in_flight_tlm_pkg = NULL;

    if (FpFwListIsEmpty(&pkg_active_list))
    {
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_NONE;
    }
    else
    {
        trp_msg->payload.dcp_msg.hdr.msg_status = DATA_COLLECTION_RD_DATA_VALID_MORE;
    }
}

void mts_manager_free_tlm_package_from_primary_mcp(p_tlm_package_t tlm_pkg)
{
    FPFW_ET_LOG(MtsMgrRemovePkgFromActive,
                tlm_pkg->pkg.local_mmap_addr,
                tlm_pkg->pkg.source_die_id,
                tlm_pkg->pkg.source_core_id);

    if (tlm_pkg->pkg.source_die_id == mts_get_this_die_id() && tlm_pkg->pkg.source_core_id == mts_get_this_core_id())
    {
        uintptr_t atu_mapped_location = tlm_pkg->pkg.local_mmap_addr;
        ddr_manager_deallocate_mem(&atu_mapped_location);
    }
    else
    {
        // send to secondary MCP
        mts_manager_send_trp_read_complete(tlm_pkg);
    }

    FpFwListInsertTail(&pkg_free_list, &tlm_pkg->list_entry);
}

void mts_manager_free_tlm_package_from_secondary_mcp(p_tlm_package_t tlm_pkg)
{
    PFPFW_LIST_ENTRY iterator = NULL;
    PFPFW_LIST_ENTRY iteratorNext = NULL;
    p_tlm_package_t curr_pkg = NULL;
    bool match = false;

    // likely first entry but will search the list to allow for out of order completion
    FpFwListForEach(pkg_active_list, iterator, iteratorNext)
    {
        curr_pkg = CONTAINING_RECORD(iterator, tlm_package_t, list_entry);

        if (memcmp(&curr_pkg->pkg, &tlm_pkg->pkg, sizeof(trp_msg_read_pkg_rsp_t)) == 0)
        {
            match = true;
            break;
        }
    }

    if (match)
    {
        FPFW_ET_LOG(MtsMgrRemovePkgFromActive,
                    tlm_pkg->pkg.local_mmap_addr,
                    tlm_pkg->pkg.source_die_id,
                    tlm_pkg->pkg.source_core_id);

        FpFwListRemoveEntry(iterator);
        uintptr_t atu_mapped_location = tlm_pkg->pkg.local_mmap_addr;
        ddr_manager_deallocate_mem(&atu_mapped_location);

        FpFwListInsertTail(&pkg_free_list, iterator);
    }
    else
    {
        FPFW_ET_LOG(MtsMgrNoMatchPkgComplete, tlm_pkg->pkg.local_mmap_addr, tlm_pkg->pkg.phy_addr_offset);
    }
}

p_tlm_package_t mts_manager_get_pkg_from_free_list(void)
{
    p_tlm_package_t ret_val = NULL;
    PFPFW_LIST_ENTRY entry = NULL;

    if (FpFwListIsEmpty(&pkg_free_list))
    {
        FPFW_ET_LOG(MtsMgrPkgFreeListEmpty);

        if (mts_is_primary_instance() && !FpFwListIsEmpty(&pkg_active_list))
        {
            entry = FpFwListRemoveHead(&pkg_active_list);
            p_tlm_package_t oldest_pkg = CONTAINING_RECORD(entry, tlm_package_t, list_entry);

            // note: this api will add the package to the free list
            mts_manager_free_tlm_package_from_primary_mcp(oldest_pkg);
        }
        else
        {
            return ret_val;
        }
        // the secondary MCP has sent its packages to the primary MCP, so it can not free the oldest package.
        // needs to be freed by the primary MCP.
    }

    entry = FpFwListRemoveHead(&pkg_free_list);
    ret_val = CONTAINING_RECORD(entry, tlm_package_t, list_entry);

    return ret_val;
}

void mts_manager_free_publish_resources()
{
    // flushing the queue is going to loop through the queue and free the blocks in the queue
    // the block that this message is in has already been popped off the queue and will be freed when
    // this function returns
    mts_client_flush_incoming_queue(MTS_CLIENT_ID_PWR_INST_TELEM);

    if (mts_is_primary_instance())
    {
        if (in_flight_tlm_pkg != NULL)
        {
            uintptr_t atu_mapped_location = in_flight_tlm_pkg->pkg.local_mmap_addr;
            ddr_manager_deallocate_mem(&atu_mapped_location);

            in_flight_tlm_pkg = NULL;
        }

        // free all active packages
        PFPFW_LIST_ENTRY iterator = NULL;
        PFPFW_LIST_ENTRY iteratorNext = NULL;
        p_tlm_package_t curr_pkg = NULL;

        FpFwListForEach(pkg_active_list, iterator, iteratorNext)
        {
            curr_pkg = CONTAINING_RECORD(iterator, tlm_package_t, list_entry);

            // only free packages that are from this MCP, the secondary MCP will free its own packages
            // without having to send trp messages for each package
            if (curr_pkg->pkg.source_die_id == mts_get_this_die_id() &&
                curr_pkg->pkg.source_core_id == mts_get_this_core_id())
            {
                mts_manager_free_tlm_package_from_primary_mcp(curr_pkg);
            }
        }
    }
    else
    {
        // free all active packages
        PFPFW_LIST_ENTRY iterator = NULL;
        PFPFW_LIST_ENTRY iteratorNext = NULL;
        p_tlm_package_t curr_pkg = NULL;

        FpFwListForEach(pkg_active_list, iterator, iteratorNext)
        {
            curr_pkg = CONTAINING_RECORD(iterator, tlm_package_t, list_entry);
            mts_manager_free_tlm_package_from_secondary_mcp(curr_pkg);
        }
    }

    // re-init the lists to recover any lost packages
    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    FPFW_ET_LOG(MtsMgrPackagesFlushed);
}

void mts_manager_init_trp_header_for_broadcast(p_trp_msg_t trp_msg, uint16_t payload_size)
{
    trp_msg->hdr.src_node.die_id = mts_get_this_die_id();
    trp_msg->hdr.src_node.core_id = mts_get_this_core_id();
    // dest is assigned during broadcast, this is just a placeholder
    trp_msg->hdr.dest_node.die_id = mts_get_this_die_id() + 1;
    trp_msg->hdr.dest_node.core_id = mts_get_this_core_id();
    trp_msg->hdr.mts_client_id = MTS_CLIENT_ID_PWR_INST_TELEM;
    trp_msg->hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    trp_msg->hdr.trp_msg_status = TRP_STATUS_SUCCESS;
    trp_msg->hdr.broadcast_type = TRP_BROADCAST_PRIM_TO_SEC_PEER_ONLY;
    trp_msg->hdr.payload_size = payload_size;
}

void mts_manager_send_mode_to_sec_cores(tlm_operating_mode_t new_mode)
{
    if (mts_is_primary_instance() == true)
    {
        trp_msg_t trp_msg = {0};
        mts_manager_init_trp_header_for_broadcast(&trp_msg, TLM_CLIENT_SET_MODE_MSG_SIZE);

        p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
        tlm_client_msg->cmd = TLM_CLIENT_CMD_SET_MODE_PUSH;
        tlm_client_msg->payload.mode = new_mode;

        // api copies message, ok to use stack memory
        mts_client_forward_trp_msg(&trp_msg, TRP_BROADCAST_PRIM_TO_SEC_PEER_ONLY);
    }
}

void mts_manager_send_prep_pwr_pkg_notification_to_sec_mcps(void)
{
    if (mts_is_primary_instance() == true)
    {
        trp_msg_t trp_msg = {0};
        mts_manager_init_trp_header_for_broadcast(&trp_msg, MEMBER_SIZE(tlm_client_msg_t, cmd));

        p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
        tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_PRIM_MCP_2_SEC_MCP_PUSH;

        // api copies message, ok to use stack memory
        mts_client_forward_trp_msg(&trp_msg, TRP_BROADCAST_PRIM_TO_SEC_PEER_ONLY);
    }
}

void mts_manager_send_prep_pwr_pkg_notification_to_scp(void)
{
    // this is sent from both primary and secondary MCP, but only needs to be handled by SCPs
    trp_msg_t trp_msg = {0};
    mts_manager_init_trp_header_for_broadcast(&trp_msg, MEMBER_SIZE(tlm_client_msg_t, cmd));
    trp_msg.hdr.dest_node.die_id = mts_get_this_die_id();
    trp_msg.hdr.dest_node.core_id = CPU_SCP;
    trp_msg.hdr.broadcast_type = TRP_BROADCAST_NONE;

    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    // api copies message, ok to use stack memory
    mts_client_send_new_trp_msg(&trp_msg);
}

void mts_manager_send_record_enables_to_scp(p_tlm_scp_record_enables_t enables)
{
    trp_msg_t trp_msg = {0};
    // overwrite broadcast to none below, other fields can be used
    mts_manager_init_trp_header_for_broadcast(&trp_msg, sizeof(tlm_client_msg_t));
    trp_msg.hdr.dest_node.die_id = mts_get_this_die_id();
    trp_msg.hdr.dest_node.core_id = CPU_SCP;
    trp_msg.hdr.broadcast_type = TRP_BROADCAST_NONE;

    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH;

    tlm_client_msg->payload.scp_records = *enables;

    // api copies message, ok to use stack memory
    mts_client_send_new_trp_msg(&trp_msg);
}
