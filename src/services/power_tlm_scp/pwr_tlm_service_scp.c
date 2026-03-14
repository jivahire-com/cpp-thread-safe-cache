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
#include <core_info.h>
#include <ddrss_runtime_api.h>
#include <fpfw_cfg_mgr.h>
#include <kng_soc_constants.h>
#include <message_transfer_service.h>
#include <power_runconfig.h>
#include <pwr_tlm_core_exchange.h>
#include <stdint.h>

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
// This will accumulate all allocated and unallocated counts across all memory controllers on this die
uint64_t pmu_cnt_all[DDRSS_MAX_MC_NUM_PER_DIE][DDRSS_MAX_PWR_TEL_EVT];

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
            // Initialize and write core Vmin values to core exchange if aging counter is enabled
            if (pwr_tlm_scp_record_enables.record.core_aging_en)
            {
                data_proc_scp_tlm_cmpnt_init_core_vmin();
            }
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
    int silibs_status = SILIBS_SUCCESS;

    if (pwr_tlm_scp_record_enables.record.vm_memory_pwr_en)
    {
        power_tlm_mpam_scp_knobs_t mpam_knobs = config_get_pwr_tlm_mpam_scp_knobs();

        ddrss_pwr_tel_evt_fltr_t pwr_tel_evt_catch_all_filter = {0};
        // for values, see Telemetry spec, Appendix section, Programming Configurations for VM Memory Collections
        // https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/doc2.aspx?sourcedoc=%7BFD425643-C61B-46E0-9AD0-93E10FAF5D0B%7D&file=1PFW_Kingsgate_Power_Telemetry_Requirements_And_Specifications%20V0.4%20WIP.docx&action=default&mobileredirect=true&DefaultItemOpen=1

        pwr_tel_evt_catch_all_filter.mask_mpam_sp = 1;
        pwr_tel_evt_catch_all_filter.mask_partid = 511;
        pwr_tel_evt_catch_all_filter.mask_perf_mon_group = 0;
        pwr_tel_evt_catch_all_filter.match_mpam_sp = 1;
        pwr_tel_evt_catch_all_filter.match_partid = 0;
        pwr_tel_evt_catch_all_filter.match_perf_mon_group = 0;
        pwr_tel_evt_catch_all_filter.invert = 0;
        pwr_tel_evt_catch_all_filter.no_event_en = 1;
        pwr_tel_evt_catch_all_filter.mask_match_en = 0;

        ddrss_pwr_tel_evt_fltr_t pwr_tel_evt_mpam_filter = {0};

        // see power telemetry spec for values
        pwr_tel_evt_mpam_filter.mask_mpam_sp = 1;
        pwr_tel_evt_mpam_filter.mask_partid = 511;
        pwr_tel_evt_mpam_filter.mask_perf_mon_group = 0;
        pwr_tel_evt_mpam_filter.match_mpam_sp = 1;
        pwr_tel_evt_mpam_filter.match_partid = 1; // initial mpam,  0 is catch all
        pwr_tel_evt_mpam_filter.match_perf_mon_group = 0;
        pwr_tel_evt_mpam_filter.invert = 0;
        pwr_tel_evt_mpam_filter.no_event_en = 0;
        pwr_tel_evt_mpam_filter.mask_match_en = 1;

        ddrss_pwr_tel_cfg_t pwr_tel_cfg = {0};
        pwr_tel_cfg.evt_sel_en = 1;
        pwr_tel_cfg.th0 = mpam_knobs.T0_thresh;
        pwr_tel_cfg.th1 = mpam_knobs.T1_thresh;
        pwr_tel_cfg.th2 = mpam_knobs.T2_thresh;

        pwr_tel_cfg.pwr_tel_rp[0].u_rd = mpam_knobs.relative_pwr_0.unenc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[0].e_rd = mpam_knobs.relative_pwr_0.enc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[0].u_wr = mpam_knobs.relative_pwr_0.unenc_wr_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[0].e_wr = mpam_knobs.relative_pwr_0.enc_wr_rel_pwr;

        pwr_tel_cfg.pwr_tel_rp[1].u_rd = mpam_knobs.relative_pwr_1.unenc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[1].e_rd = mpam_knobs.relative_pwr_1.enc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[1].u_wr = mpam_knobs.relative_pwr_1.unenc_wr_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[1].e_wr = mpam_knobs.relative_pwr_1.enc_wr_rel_pwr;

        pwr_tel_cfg.pwr_tel_rp[2].u_rd = mpam_knobs.relative_pwr_2.unenc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[2].e_rd = mpam_knobs.relative_pwr_2.enc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[2].u_wr = mpam_knobs.relative_pwr_2.unenc_wr_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[2].e_wr = mpam_knobs.relative_pwr_2.enc_wr_rel_pwr;

        pwr_tel_cfg.pwr_tel_rp[3].u_rd = mpam_knobs.relative_pwr_3.unenc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[3].e_rd = mpam_knobs.relative_pwr_3.enc_rd_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[3].u_wr = mpam_knobs.relative_pwr_3.unenc_wr_rel_pwr;
        pwr_tel_cfg.pwr_tel_rp[3].e_wr = mpam_knobs.relative_pwr_3.enc_wr_rel_pwr;

        ddrss_pmu_evt_type_cfg_t evt_cfg = {0};

        uint8_t die_id = mts_get_this_die_id();
        uint16_t mc_start = die_id * DDRSS_MAX_MC_NUM_PER_DIE;
        uint16_t mc_end = mc_start + DDRSS_MAX_MC_NUM_PER_DIE;
        for (uint16_t mc = mc_start; mc < mc_end; mc++)
        {
            silibs_status = ddrss_set_power_telemetry_config(mc, &pwr_tel_cfg);
            FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, 0, 0);

            for (uint16_t event_idx = 0; event_idx < DDRSS_MAX_PWR_TEL_EVT; event_idx++)
            {
                if (event_idx == DDRSS_MAX_PWR_TEL_EVT - 1)
                {
                    // for each memory controller, there are 8 pmu's. 7 are assigned to individual mpams, this
                    // one is assigned to catch the rest.
                    silibs_status = ddrss_set_power_telemetry_filter(mc, event_idx, &pwr_tel_evt_catch_all_filter);
                }
                else
                {
                    silibs_status = ddrss_set_power_telemetry_filter(mc, event_idx, &pwr_tel_evt_mpam_filter);
                    pwr_tel_evt_mpam_filter.match_partid += 1; // next mpam partid
                }

                FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, event_idx, 0);

                // the first 8 pmu's are assigned to power telemetry events, so can use event_idx as pmu_en
                // which is actually the pmu index.
                silibs_status = ddrss_pmu_init(mc, event_idx);
                FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, event_idx, 0);

                evt_cfg.evt_id = event_idx;
                silibs_status = ddrss_pmu_set_event(mc, event_idx, &evt_cfg);
                FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, event_idx, 0);

                silibs_status = ddrss_pmu_enable(mc, event_idx, 0x1);
                FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, event_idx, 0);
            }
        }
    }
    else
    {
        uint8_t die_id = mts_get_this_die_id();
        uint16_t mc_start = die_id * DDRSS_MAX_MC_NUM_PER_DIE;
        uint16_t mc_end = mc_start + DDRSS_MAX_MC_NUM_PER_DIE;
        for (uint16_t mc = mc_start; mc < mc_end; mc++)
        {
            for (uint16_t pmu_idx = 0; pmu_idx < DDRSS_MAX_PWR_TEL_EVT; pmu_idx++)
            {
                silibs_status = ddrss_pmu_enable(mc, pmu_idx, 0x0);
                FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, pmu_idx, 0);
            }
        }
    }
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

    int silibs_status = SILIBS_SUCCESS;
    uint64_t pmn_cnt = 0;
    uint8_t die_id = mts_get_this_die_id();
    uint16_t mc_start = die_id * DDRSS_MAX_MC_NUM_PER_DIE;
    uint16_t mc_end = mc_start + DDRSS_MAX_MC_NUM_PER_DIE;
    // clear the pmu_cnt_all buffer before accumulation, in case there are stale values from previous collections
    memset(pmu_cnt_all, 0, sizeof(pmu_cnt_all));

    if (IS_PLATFORM_SVP())
    {
        // not all ddrss features are supported on SVP
        return;
    }

    for (uint16_t mc = mc_start; mc < mc_end; mc++)
    {
        // Calculate mc and local buffer index.
        uint16_t mc_local_index = mc - mc_start;
        for (uint16_t pmu_idx = 0; pmu_idx < DDRSS_MAX_PWR_TEL_EVT; pmu_idx++)
        {
            silibs_status = ddrss_pmu_read_counter_snapshot(mc, pmu_idx, &pmn_cnt);
            FPFW_RUNTIME_ASSERT_EXT(silibs_status == SILIBS_SUCCESS, silibs_status, mc, pmu_idx, 0);
            pmu_cnt_all[mc_local_index][pmu_idx] = pmn_cnt;
        }
    }
    // Write VM memory power telemetry to SCP telemetry exchange
    pwr_tlm_core_exch_scp_write_mpam_pmu_counts((uint64_t*)pmu_cnt_all);
}

void data_proc_scp_tlm_cmpnt_init_core_vmin(void)
{
    uint16_t vmin_array[NUM_AP_CORES_PER_DIE] = {0};
    corebits_t* enabled_cores = core_info_get_enable_cores_result();

    // Retrieve Vmin values from power service for each enabled core
    for (unsigned int core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        if (corebits_is_bit_set(enabled_cores, core_id))
        {
            int32_t status = power_runconfig_get_core_vmin_mv(core_id, &vmin_array[core_id]);
            FPFW_RUNTIME_ASSERT_EXT(status == 0, status, core_id, 0, 0);
        }
        else
        {
            // Core is disabled, set Vmin to 0
            vmin_array[core_id] = 0;
        }
    }

    // Write Vmin values to SCP telemetry exchange
    pwr_tlm_core_exch_scp_write_vmin(vmin_array);

    FPFW_ET_LOG(ScpCoreVminInitialized);
}
