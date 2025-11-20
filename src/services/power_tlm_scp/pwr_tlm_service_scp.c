//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_service_scp.c
 * High level telemetry service implementation on SCP.
 */
/*------------- Includes -----------------*/
#include "pwr_tlm_service_scp.h"

#include "pwr_tlm_scp_events_i.h"
#include "pwr_tlm_service_scp_i.h"

#include <FpFwAssert.h>                  // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>                   // for FPFW_UNUSED
#include <IFpFwEventTracingGeneration.h> // for FPFW_ET_LOG
#include <fpfw_cfg_mgr.h>
#include <message_transfer_service.h>
#include <power_runconfig.h>
#include <pwr_tlm_core_exchange.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

#define PWR_TLM_MAX_TRP_MESSAGES (2)
#define PWR_TLM_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + MAX_TRP_MSG_BLOCK_SIZE) * PWR_TLM_MAX_TRP_MESSAGES)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// Static assert to ensure structure size consistency
static_assert(sizeof(power_adclk_tel_t) == sizeof(uint64_t) * NUM_AP_CORES_PER_DIE,
              "Mismatch in droop_count array size between local and power service structures");

// for dcs client subscription
p_trp_msg_t pwr_tlm_client_queue_mem[PWR_TLM_MAX_TRP_MESSAGES];
uint8_t pwr_tlm_client_pool_mem[PWR_TLM_CLIENT_BLOCK_POOL_SIZE];

static mts_client_t s_pwr_tlm_mts_client_scp = {
    .notify_from_drv_frmwk = pwr_tlm_scp_handle_incoming_mts_msgs,
};

tlm_scp_record_enables_t pwr_tlm_scp_record_enables = {0};

/*------------- Functions ----------------*/

void mts_manager_scp_init(void)
{
    UINT tx_status = tx_queue_create(&s_pwr_tlm_mts_client_scp.rx_queue,
                                     "Pwr Tlm MTS client queue",
                                     1, // number of uint32_t elements
                                     pwr_tlm_client_queue_mem,
                                     sizeof(pwr_tlm_client_queue_mem));
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_mts_client_scp.rx_queue, 0, 0);

    tx_status = tx_block_pool_create(&s_pwr_tlm_mts_client_scp.rx_pool, // pool_ptr
                                     "Pwr Tlm client pool",             // name_ptr
                                     MAX_TRP_MSG_BLOCK_SIZE,            // block_size
                                     pwr_tlm_client_pool_mem,           // pool_start
                                     sizeof(pwr_tlm_client_pool_mem));  // pool_size
    FPFW_RUNTIME_ASSERT_EXT(tx_status == TX_SUCCESS, tx_status, (uintptr_t)&s_pwr_tlm_mts_client_scp.rx_pool, 0, 0);

    mts_client_register(MTS_CLIENT_ID_PWR_INST_TELEM, &s_pwr_tlm_mts_client_scp);
}

void pwr_tlm_scp_handle_incoming_mts_msgs(void)
{
    /* Note: this function is running on the callback from the MTS thread */
    p_trp_msg_t trp_msg = NULL;
    UINT queue_status = 0;

    // may be multiple messages in the queue
    do
    {
        queue_status = tx_queue_receive(&s_pwr_tlm_mts_client_scp.rx_queue, &trp_msg, TX_NO_WAIT);
        if ((queue_status == TX_SUCCESS) && (trp_msg != NULL))
        {
            // debug level trace
            FPFW_ET_LOG(ScpTlmSvcDebugIncomingTrpMsg,
                        GET_BASE_SEQ_NUM(trp_msg->hdr.source_seq_num.as_uint16),
                        trp_msg->hdr.src_node.die_id,
                        trp_msg->hdr.src_node.core_id,
                        trp_msg->hdr.dest_node.die_id,
                        trp_msg->hdr.dest_node.core_id,
                        trp_msg->hdr.mts_client_id,
                        trp_msg->hdr.trp_msg_id,
                        trp_msg->hdr.trp_msg_status,
                        GET_INT_CMD_BIT(trp_msg->hdr.source_seq_num.as_uint16));

            if (trp_msg->hdr.trp_msg_id == TRP_MSG_ID_DCP_FORWARD)
            {
                // debug level trace
                FPFW_ET_LOG(ScpTlmSvcUnexpectedIncomingDcpMsg,
                            trp_msg->payload.dcp_msg.hdr.client_id,
                            trp_msg->payload.dcp_msg.hdr.msg_id,
                            trp_msg->payload.dcp_msg.hdr.seq_num);
            }
            else
            {
                mts_manager_scp_handle_trp_msg(trp_msg);
            }

            UINT block_status = tx_block_release(trp_msg);
            if (block_status != TX_SUCCESS)
            {
                FPFW_ET_LOG(ScpMtsMgrClientFreeFail, (uintptr_t)trp_msg, block_status);
            }
        }
        else if (queue_status != TX_QUEUE_EMPTY)
        {
            FPFW_ET_LOG(ScpMtsMgrClientQueueFail, (uintptr_t)&s_pwr_tlm_mts_client_scp.rx_queue, queue_status);
        }
    } while (queue_status == TX_SUCCESS);
}

void mts_manager_scp_handle_trp_msg(p_trp_msg_t trp_msg)
{
    switch (trp_msg->hdr.trp_msg_id)
    {

    case TRP_MSG_ID_CLIENT_DEFINED: {
        p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg->payload.client_msg;

        switch (tlm_client_msg->cmd)
        {
        case TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH:
            data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(tlm_client_msg->payload.scp_records);
            break;

        case TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH:
            if (pwr_tlm_scp_record_enables.record.drop_count_en)
            {
                data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp();
            }
            if (pwr_tlm_scp_record_enables.record.vm_memory_pwr_en)
            {
                data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp();
            }
            break;

        default:
            FPFW_ET_LOG(ScpMtsMgrClientUnexpectedCmd, tlm_client_msg->cmd);
        }
        break;
    }

    default:
        FPFW_ET_LOG(ScpMtsMgrClientUnexpectedTrpMsg, trp_msg->hdr.trp_msg_id);
        break;
    }
}

void pwr_tlm_svc_scp_init(void)
{
    mts_manager_scp_init();
}

void data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(tlm_scp_record_enables_t enables)
{
    pwr_tlm_scp_record_enables = enables;
    FPFW_ET_LOG(ScpMtsMgrEnablesUpdated, pwr_tlm_scp_record_enables.as_uint16);

    // todo: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584713
    // if (pwr_tlm_scp_record_enables.record.vm_memory_pwr_en)
    // {
    //     power_tlm_mpam_scp_knobs_t mpam_knobs = config_get_pwr_tlm_mpam_scp_knobs();
    // }
    // else
    // {
    // }
}

void data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp(void)
{
    power_adclk_tel_t adclk_tel;
    // Retrieve droop count telemetry from power service
    power_get_adclk_telem(&adclk_tel);

    // Write droop counts to SCP telemetry exchange
    pwr_tlm_core_exch_scp_write_droop_counts(&adclk_tel.droop_count);
}

void data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp(void)
{
    // todo: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584713
}
