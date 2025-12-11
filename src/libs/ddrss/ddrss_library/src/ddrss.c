//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss.c
 *  Main initialization for various DDR subsystem components
 */

/*------------- Includes -----------------*/
#include "ddr_atu_map.h"

#include <DbgPrint.h>
#include <FPFwInterrupts.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <bug_check.h>
#include <cmn800.h>
#include <cmsdk_wd.h> // for wdog_cmsdk_apb_disable
#include <ddr_err_inj.h>
#include <ddr_i3c.h>
#include <ddr_manager.h>
#include <ddr_manager_i3c.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <ddrss_reserved_regions_version.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h> // for fpfw_init_get_handle
#include <gtimer_prodfw.h>
#include <idhw.h> // for idhw_is_single_die_boot_en
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_scp_tfa_shared.h>
#include <memory_map/ddrss_reserved_regions.h>
#include <mscp_exp_rmss_memory_map.h>
#include <pcr_ddrss.h>
#include <sel.h>
#include <silibs_ap_top_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/
// #define SPD_UPDATE_TESTING           // Defined to prevent SPD readback from being updated for testing
// #define SPD_READ_DEBUGPRINT_ENABLE  // Define to enable debug prints for SPD readback
#define SPD_VERSION                 (3U)
#define TEXT_DDR_SPEED_GRADE_LOCKED "DDRSS - ddr_speed_grade locked to %s for %s\n"
#define SEL_RECORD_TYPE_DDR_PPR     0xCB
#define SPD_OFFSET_DDR5             (640U)
#define SPD_STRUCT_SIZE_DDR5        SIZE_16_BYTES

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define DDRSS_PROD_FW_PROD_KEY_LOADED    (BIT0)
#define DDRSS_PROD_FW_FIPS_KEY_LOADED    (BIT1)
#define DDRSS_PROD_FW_FIPS_TEST_NOTIFIED (BIT2)

static uint32_t prod_key_load_req_cnt = 0;
static uint32_t ddrss_key_loading_req = 0;
static uint32_t ddrss_interrupt_id[6] = {0, 1, 2, 3, 4, 5};
ddrss_phy_training_dq_margin_t ddrss_phy_training_dq_margin[6] = {0};

// Refer to N.2.5 Memory Error Section of UEFI Specification, Version 2.8 Errata C
static const guid_t STD_MEMORY_ERROR_DOMAIN_GUID = {0xB7E2A3C9, 0x4F1D, 0x4569, {0xA3, 0x9D, 0xD6, 0x5B, 0xAF, 0x10, 0x92, 0xEE}};
static const guid_t DDR_ERROR_DOMAIN_FRU_GUID = {0x3AC75B2E, 0xC8F1, 0x43E1, {0x88, 0x7C, 0x9A, 0x12, 0x34, 0x56, 0x78, 0x9A}};
static const guid_t DDR_RHTL_ERROR_DOMAIN_FRU_GUID = ACPI_ERROR_TYPE_VENDOR_RHTLM;
/*------------- Functions ----------------*/
void prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    int sts = SILIBS_SUCCESS;
    ddrss_cfg_knobs_t ddrss_cfgs;
    KNG_PLAT_ID platform_id;
    uint32_t interrupt_idx;
    const char* platform_str;
    const char* start_type;

    boot_status_req_t ddr_bsc_req_mem = {0};
    post_led_status(&ddr_bsc_req_mem, LED_STATUS_CODE_SCP_DDR_INIT_START);

    // 1 DDRSS contains 2 memory controllers
    // PHY level is under MC level in heirarchy, 1 PHY per DDRSS.
    // Register interrupt handler for DDRSS - Each DDRSS contains 2 memory controllers
    // RAS level
    // MC level
    uint32_t intr_status = 0;
    const uint32_t FIRST_DDRSS_INT = HW_INT_DDRSS0_COMBINED_SCP_INT;
    const uint32_t LAST_DDRSS_INT = HW_INT_DDRSS5_COMBINED_SCP_INT;
    for (uint32_t ddrss_int = FIRST_DDRSS_INT; ddrss_int <= LAST_DDRSS_INT; ddrss_int++)
    {
        interrupt_idx = (ddrss_int - FIRST_DDRSS_INT);
        intr_status = FPFwCoreInterruptRegisterCallback(ddrss_int,
                                                        (FPFwCoreInterruptHandler)prod_ddrss_interrupt_handler,
                                                        (void*)&ddrss_interrupt_id[interrupt_idx]);
        intr_status |= FPFwCoreInterruptEnableVector(ddrss_int);

        BUG_ASSERT_PARAM(intr_status == 0, intr_status, 0);
    }

    // Get the default DDRSS cfgs
    sts = ddrss_get_config(&ddrss_cfgs);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    if (idhw_is_single_die_boot_en())
    {
        // Set DDRSS mask for 1-die boot (12 MCs) - This will be overridden by I3C detect on RVP
        ddrss_cfgs.ext_knobs.ddrss_mask = 0x03F;
    }
    else
    {
        // Set DDRSS mask for 2-die boot (24 MCs) - This will be overridden by I3C detect on RVP
        ddrss_cfgs.ext_knobs.ddrss_mask = 0xFFF;
    }

    // Setting the PAS list generates an unrecoverable error on Dual Die FPGA. See bug:
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2383878 GENERATE_ARRAY_OF_RSVD_REGIONS
    // DDRSS_PAS_REGIONS
    // ddrss_cfgs.pas_range_list = (uintptr_t)&_mem_region_list;

    // Update DDRSS cfgs to match actual platform cfg
    ddrss_cfgs.die_id = (DIE_INSTANCE)die_num;
    ddrss_cfgs.debug_level = config_get_ddr_debug_level();
    ddrss_cfgs.parallel_phy_train_en = config_get_parallel_phy_train_en();

    ddrss_cfgs.ext_knobs.media_scrambling_en = config_get_media_scrambling_en();
    ddrss_cfgs.ext_knobs.addr_err_report = config_get_addr_err_report();
    ddrss_cfgs.ext_knobs.address_hashing_mode = config_get_address_hashing_mode();
    ddrss_cfgs.ext_knobs.address_map_mode = config_get_address_map_mode();
    ddrss_cfgs.ext_knobs.allow_non_urgent_ref = config_get_allow_non_urgent_ref();
    ddrss_cfgs.ext_knobs.async_cmd_fifo_depth = config_get_async_cmd_fifo_depth();
    ddrss_cfgs.ext_knobs.async_cmd_fifo_en = config_get_async_cmd_fifo_en();
    ddrss_cfgs.ext_knobs.auto_ecs_in_self_refresh_en = config_get_auto_ecs_in_self_refresh_en();
    ddrss_cfgs.ext_knobs.auto_phy_lp_en = config_get_auto_phy_lp_en();
    ddrss_cfgs.ext_knobs.auto_phy_lp2_en = config_get_auto_phy_lp2_en();
    ddrss_cfgs.ext_knobs.auto_self_refresh_en = config_get_auto_self_refresh_en();
    ddrss_cfgs.ext_knobs.be_cmd_exec_opt = config_get_be_cmd_exec_opt();
    ddrss_cfgs.ext_knobs.be_cmd_select_weight = config_get_be_cmd_select_weight();
    ddrss_cfgs.ext_knobs.block_pre_conf_cmd_in_be = config_get_block_pre_conf_cmd_in_be();
    ddrss_cfgs.ext_knobs.bump_priority_level = config_get_bump_priority_level();
    ddrss_cfgs.ext_knobs.busif_chi_qos_to_prio_rd = config_get_busif_chi_qos_to_prio_rd();
    ddrss_cfgs.ext_knobs.busif_chi_qos_to_prio_wr = config_get_busif_chi_qos_to_prio_wr();
    ddrss_cfgs.ext_knobs.busif_cob_skip_cnt = config_get_busif_cob_skip_cnt();
    ddrss_cfgs.ext_knobs.busif_drop_pftgt_fecq_occ = config_get_busif_drop_pftgt_fecq_occ();
    ddrss_cfgs.ext_knobs.busif_pcrdgrant_skip_cnt = config_get_busif_pcrdgrant_skip_cnt();
    ddrss_cfgs.ext_knobs.busif_rcb_skip_cnt = config_get_busif_rcb_skip_cnt();
    ddrss_cfgs.ext_knobs.ca_parity_check_en = config_get_ca_parity_check_en();
    ddrss_cfgs.ext_knobs.ca_parity_err_recovery_en = config_get_ca_parity_err_recovery_en();
    ddrss_cfgs.ext_knobs.chi_cbusy_map_mpam_0 = config_get_chi_cbusy_map_mpam_0();
    ddrss_cfgs.ext_knobs.chi_cbusy_map_mpam_1 = config_get_chi_cbusy_map_mpam_1();
    ddrss_cfgs.ext_knobs.chi_cbusy_map_mpam_2 = config_get_chi_cbusy_map_mpam_2();
    ddrss_cfgs.ext_knobs.chi_cbusy_map_mpam_3 = config_get_chi_cbusy_map_mpam_3();
    ddrss_cfgs.ext_knobs.chi_cbusy_map_mpam_4 = config_get_chi_cbusy_map_mpam_4();
    ddrss_cfgs.ext_knobs.cmd_merge_en = config_get_cmd_merge_en();
    ddrss_cfgs.ext_knobs.cob_occ_limit_prio0 = config_get_cob_occ_limit_prio0();
    ddrss_cfgs.ext_knobs.cob_occ_limit_prio1 = config_get_cob_occ_limit_prio1();
    ddrss_cfgs.ext_knobs.cob_occ_limit_prio2 = config_get_cob_occ_limit_prio2();
    ddrss_cfgs.ext_knobs.cob_occ_limit_prio3 = config_get_cob_occ_limit_prio3();
    ddrss_cfgs.ext_knobs.cob_slice_depth = config_get_cob_slice_depth();
    ddrss_cfgs.ext_knobs.criteria = config_get_criteria();
    ddrss_cfgs.ext_knobs.ctrlupd_interval = config_get_ctrlupd_interval();
    ddrss_cfgs.ext_knobs.ctrlupd_interval_unit = config_get_ctrlupd_interval_unit();
    ddrss_cfgs.ext_knobs.ctrlupd_mode = config_get_ctrlupd_mode();
    ddrss_cfgs.ext_knobs.data_comp_delay = config_get_data_comp_delay();
    ddrss_cfgs.ext_knobs.ddr_speed_grade = config_get_ddr_speed_grade();
    // ddrss_cfgs.ext_knobs.ddrss_mask handled separately
    ddrss_cfgs.ext_knobs.ddrss_ras_fw_first = config_get_ddrss_ras_fw_first();
    ddrss_cfgs.ext_knobs.dfi_ratio = config_get_dfi_ratio();
    ddrss_cfgs.ext_knobs.dimm_sub_ch_swap = config_get_dimm_sub_ch_swap();
    ddrss_cfgs.ext_knobs.dp_busrdfifo_depth = config_get_dp_busrdfifo_depth();
    ddrss_cfgs.ext_knobs.dq_margin_extract_pattern_len = config_get_dq_margin_extract_pattern_len();
    ddrss_cfgs.ext_knobs.dq_margin_extract_pattern_seed = config_get_dq_margin_extract_pattern_seed();
    ddrss_cfgs.ext_knobs.dq_margin_extract_step_size = config_get_dq_margin_extract_step_size();
    ddrss_cfgs.ext_knobs.dq_margin_th = config_get_dq_margin_th();
    ddrss_cfgs.ext_knobs.dqs_interval_timer_run_time = config_get_dqs_interval_timer_run_time();
    ddrss_cfgs.ext_knobs.dram_active_power_down_en = config_get_dram_active_power_down_en();
    ddrss_cfgs.ext_knobs.dram_init_mode = config_get_dram_init_mode();
    ddrss_cfgs.ext_knobs.dram_power_down_en = config_get_dram_power_down_en();
    ddrss_cfgs.ext_knobs.dram_power_down_entry_delay = config_get_dram_power_down_entry_delay();
    ddrss_cfgs.ext_knobs.dram_refresh_mode = config_get_dram_refresh_mode();
    ddrss_cfgs.ext_knobs.drfm_type_1x = config_get_drfm_type_1x();
    ddrss_cfgs.ext_knobs.drfm_type_2x = config_get_drfm_type_2x();
    ddrss_cfgs.ext_knobs.drop_prefetch_tgt = config_get_drop_prefetch_tgt();
    ddrss_cfgs.ext_knobs.dynamic_clk_gating_en = config_get_dynamic_clk_gating_en();
    ddrss_cfgs.ext_knobs.ecc_ce_th = config_get_ecc_ce_th();
    ddrss_cfgs.ext_knobs.ecs_readout_interval = config_get_ecs_readout_interval();
    ddrss_cfgs.ext_knobs.ecs_readout_interval_unit = config_get_ecs_readout_interval_unit();
    ddrss_cfgs.ext_knobs.en_cmd_retry = config_get_en_cmd_retry();
    ddrss_cfgs.ext_knobs.en_data_parity_check = config_get_en_data_parity_check();
    ddrss_cfgs.ext_knobs.erhm_en = config_get_erhm_en();
    ddrss_cfgs.ext_knobs.err_cnt_leak_rate = config_get_err_cnt_leak_rate();
    ddrss_cfgs.ext_knobs.fecq_bandwidth_limiter_config = config_get_fecq_bandwidth_limiter_config();
    ddrss_cfgs.ext_knobs.fecq_becq_pri_map_config = config_get_fecq_becq_pri_map_config();
    ddrss_cfgs.ext_knobs.fecq_cmd_dispatch_limit_config = config_get_fecq_cmd_dispatch_limit_config();
    ddrss_cfgs.ext_knobs.fecq_congest_lvl_0 = config_get_fecq_congest_lvl_0();
    ddrss_cfgs.ext_knobs.fecq_congest_lvl_1 = config_get_fecq_congest_lvl_1();
    ddrss_cfgs.ext_knobs.fecq_congest_lvl_2 = config_get_fecq_congest_lvl_2();
    ddrss_cfgs.ext_knobs.fecq_congest_lvl_3 = config_get_fecq_congest_lvl_3();
    ddrss_cfgs.ext_knobs.fecq_congest_lvl_alt = config_get_fecq_congest_lvl_alt();
    ddrss_cfgs.ext_knobs.fecq_fecqslicestartwr_count = config_get_fecq_fecqslicestartwr_count();
    ddrss_cfgs.ext_knobs.fecq_fecqslicestopwr_count = config_get_fecq_fecqslicestopwr_count();
    ddrss_cfgs.ext_knobs.fecq_occ_limit_prio0 = config_get_fecq_occ_limit_prio0();
    ddrss_cfgs.ext_knobs.fecq_occ_limit_prio1 = config_get_fecq_occ_limit_prio1();
    ddrss_cfgs.ext_knobs.fecq_occ_limit_prio2 = config_get_fecq_occ_limit_prio2();
    ddrss_cfgs.ext_knobs.fecq_occ_limit_prio3 = config_get_fecq_occ_limit_prio3();
    ddrss_cfgs.ext_knobs.fecq_over_subscr_slice_occ = config_get_fecq_over_subscr_slice_occ();
    ddrss_cfgs.ext_knobs.fecq_over_subscr_total_occ = config_get_fecq_over_subscr_total_occ();
    ddrss_cfgs.ext_knobs.fecq_rdpriignorepref = config_get_fecq_rdpriignorepref();
    ddrss_cfgs.ext_knobs.fecq_rdpriprefrd = config_get_fecq_rdpriprefrd();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_basicmode = config_get_fecq_rdwrpref_basicmode();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_minfecq_count = config_get_fecq_rdwrpref_minfecq_count();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_minfecqrd_count = config_get_fecq_rdwrpref_minfecqrd_count();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_minfecqwr_count = config_get_fecq_rdwrpref_minfecqwr_count();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_send_rd_max_idle_cycles = config_get_fecq_rdwrpref_send_rd_max_idle_cycles();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_startwrpref = config_get_fecq_rdwrpref_startwrpref();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_stopwrpref = config_get_fecq_rdwrpref_stopwrpref();
    ddrss_cfgs.ext_knobs.fecq_rdwrpref_wr_time_out = config_get_fecq_rdwrpref_wr_time_out();
    ddrss_cfgs.ext_knobs.fecq_slice_depth = config_get_fecq_slice_depth();
    ddrss_cfgs.ext_knobs.fecq_startwr_count = config_get_fecq_startwr_count();
    ddrss_cfgs.ext_knobs.fecq_stopwr_count = config_get_fecq_stopwr_count();
    ddrss_cfgs.ext_knobs.fecq_wrpriignorepref = config_get_fecq_wrpriignorepref();
    ddrss_cfgs.ext_knobs.fecq_wrpriprefwr = config_get_fecq_wrpriprefwr();
    ddrss_cfgs.ext_knobs.fedb_busrdfifo_depth = config_get_fedb_busrdfifo_depth();
    ddrss_cfgs.ext_knobs.fedb_ccdb_num_lcl_cmd = config_get_fedb_ccdb_num_lcl_cmd();
    ddrss_cfgs.ext_knobs.fedb_ccdb_num_total_cmd = config_get_fedb_ccdb_num_total_cmd();
    ddrss_cfgs.ext_knobs.fedb_ccdb_num_wr2dp_cmd = config_get_fedb_ccdb_num_wr2dp_cmd();
    ddrss_cfgs.ext_knobs.fips_kat_en = config_get_fips_kat_en();
    ddrss_cfgs.ext_knobs.inband_crc_mode = config_get_inband_crc_mode();
    ddrss_cfgs.ext_knobs.interleave_mc_cnt = config_get_interleave_mc_cnt();

    // Default changed in prod. fw xml from 0 to 1
    ddrss_cfgs.ext_knobs.intu_init_en = config_get_intu_init_en();
    ddrss_cfgs.ext_knobs.keystore_scrub_en = config_get_keystore_scrub_en();
    ddrss_cfgs.ext_knobs.lockout_priority_enable = config_get_lockout_priority_enable();
    ddrss_cfgs.ext_knobs.lockout_priority_threshold = config_get_lockout_priority_threshold();
    ddrss_cfgs.ext_knobs.manual_ecs_en = config_get_manual_ecs_en();
    ddrss_cfgs.ext_knobs.mc_mpam_bm_dis = config_get_mc_mpam_bm_dis();
    ddrss_cfgs.ext_knobs.mc_mpam_bp_dis = config_get_mc_mpam_bp_dis();
    ddrss_cfgs.ext_knobs.mc_mpam_mbw_cycles = config_get_mc_mpam_mbw_cycles();
    ddrss_cfgs.ext_knobs.mc_mpam_mbw_max_overflow_max = config_get_mc_mpam_mbw_max_overflow_max();
    ddrss_cfgs.ext_knobs.mc_mpam_mbw_min_overflow_max = config_get_mc_mpam_mbw_min_overflow_max();
    ddrss_cfgs.ext_knobs.mc_mpam_mbw_xfers = config_get_mc_mpam_mbw_xfers();
    ddrss_cfgs.ext_knobs.mc_mpam_winwd = config_get_mc_mpam_winwd();
    ddrss_cfgs.ext_knobs.media_ecc_mode = config_get_media_ecc_mode();
    ddrss_cfgs.ext_knobs.media_scrub_en = config_get_media_scrub_en();
    ddrss_cfgs.ext_knobs.media_scrub_interval = config_get_media_scrub_interval();
    ddrss_cfgs.ext_knobs.media_scrub_interval_unit = config_get_media_scrub_interval_unit();
    ddrss_cfgs.ext_knobs.mpam_mbw_ns_hardlim = config_get_mpam_mbw_ns_hardlim();
    ddrss_cfgs.ext_knobs.mpam_mbw_ns_max = config_get_mpam_mbw_ns_max();
    ddrss_cfgs.ext_knobs.mpam_mbw_ns_min = config_get_mpam_mbw_ns_min();
    ddrss_cfgs.ext_knobs.mpam_mbw_s_hardlim = config_get_mpam_mbw_s_hardlim();
    ddrss_cfgs.ext_knobs.mpam_mbw_s_max = config_get_mpam_mbw_s_max();
    ddrss_cfgs.ext_knobs.mpam_mbw_s_min = config_get_mpam_mbw_s_min();
    ddrss_cfgs.ext_knobs.osc_cal_interval = config_get_osc_cal_interval();
    ddrss_cfgs.ext_knobs.osc_cal_interval_unit = config_get_osc_cal_interval_unit();
    ddrss_cfgs.ext_knobs.pa_trace_stall = config_get_pa_trace_stall();
    ddrss_cfgs.ext_knobs.page_open_timer = config_get_page_open_timer();
    ddrss_cfgs.ext_knobs.page_policy = config_get_page_policy();
    ddrss_cfgs.ext_knobs.pas_encryption_en_mask = config_get_pas_encryption_en_mask();
    ddrss_cfgs.ext_knobs.patrol_scrub_urgency_qos = config_get_patrol_scrub_urgency_qos();
    ddrss_cfgs.ext_knobs.pcrdgrant_delay = config_get_pcrdgrant_delay();
    ddrss_cfgs.ext_knobs.perf_debug_profile = config_get_perf_debug_profile();
    ddrss_cfgs.ext_knobs.periodic_ecs_en = config_get_periodic_ecs_en();
    ddrss_cfgs.ext_knobs.periodic_ecs_readout_en = config_get_periodic_ecs_readout_en();
    ddrss_cfgs.ext_knobs.periodic_osc_cal_en = config_get_periodic_osc_cal_en();
    ddrss_cfgs.ext_knobs.periodic_phy_update = config_get_periodic_phy_update();
    ddrss_cfgs.ext_knobs.periodic_zq_cal_en = config_get_periodic_zq_cal_en();
    ddrss_cfgs.ext_knobs.phy_adv_training_opt = config_get_phy_adv_training_opt();
    ddrss_cfgs.ext_knobs.phy_fw_diag_en = config_get_phy_fw_diag_en();
    ddrss_cfgs.ext_knobs.phy_hdt_ctrl = config_get_phy_hdt_ctrl();
    ddrss_cfgs.ext_knobs.phy_lp_entry_delay = config_get_phy_lp_entry_delay();
    ddrss_cfgs.ext_knobs.phy_lp_wakeup = config_get_phy_lp_wakeup();
    ddrss_cfgs.ext_knobs.phy_lp2_entry_delay = config_get_phy_lp2_entry_delay();
    ddrss_cfgs.ext_knobs.phy_lp2_entry_delay_unit = config_get_phy_lp2_entry_delay_unit();
    ddrss_cfgs.ext_knobs.phy_margin_th = config_get_phy_margin_th();
    ddrss_cfgs.ext_knobs.phy_more_adv_training_opt = config_get_phy_more_adv_training_opt();
    ddrss_cfgs.ext_knobs.phy_qcs_qca_trn_en = config_get_phy_qcs_qca_trn_en();
    ddrss_cfgs.ext_knobs.phy_train_max_retry = config_get_phy_train_max_retry();
    ddrss_cfgs.ext_knobs.phy_zcal_interval = config_get_phy_zcal_interval();
    ddrss_cfgs.ext_knobs.pll_bypass_en = config_get_pll_bypass_en();
    ddrss_cfgs.ext_knobs.pll_frac_en = config_get_pll_frac_en();
    ddrss_cfgs.ext_knobs.ppr_defect_list = config_get_ppr_defect_list();
    ddrss_cfgs.ext_knobs.ppr_type = config_get_ppr_type();
    ddrss_cfgs.ext_knobs.prio_map_to_fecq = config_get_prio_map_to_fecq();

    ddrss_cfgs.ext_knobs.ras_init_en = config_get_ras_init_en(); // This may need to be set to 1 for Prod. FW
    ddrss_cfgs.ext_knobs.rcb_depth = config_get_rcb_depth();
    ddrss_cfgs.ext_knobs.rcb_en = config_get_rcb_en();
    ddrss_cfgs.ext_knobs.rcb_occ_limit_prio0 = config_get_rcb_occ_limit_prio0();
    ddrss_cfgs.ext_knobs.rcb_occ_limit_prio1 = config_get_rcb_occ_limit_prio1();
    ddrss_cfgs.ext_knobs.rcb_occ_limit_prio2 = config_get_rcb_occ_limit_prio2();
    ddrss_cfgs.ext_knobs.rcb_occ_limit_prio3 = config_get_rcb_occ_limit_prio3();
    ddrss_cfgs.ext_knobs.rcd_delay = config_get_rcd_delay();
    ddrss_cfgs.ext_knobs.ref_same_bank_order = config_get_ref_same_bank_order();
    ddrss_cfgs.ext_knobs.ref_temp_changed = config_get_ref_temp_changed();
    ddrss_cfgs.ext_knobs.ref_temp_high = config_get_ref_temp_high();
    ddrss_cfgs.ext_knobs.refresh_cmd_delay_all_bank = config_get_refresh_cmd_delay_all_bank();
    ddrss_cfgs.ext_knobs.refresh_cmd_delay_drfm_disable = config_get_refresh_cmd_delay_drfm_disable();
    ddrss_cfgs.ext_knobs.refresh_cmd_delay_ecs_disable = config_get_refresh_cmd_delay_ecs_disable();
    ddrss_cfgs.ext_knobs.refresh_cmd_delay_rfm_disable = config_get_refresh_cmd_delay_rfm_disable();
    ddrss_cfgs.ext_knobs.refresh_cmd_delay_same_bank = config_get_refresh_cmd_delay_same_bank();
    ddrss_cfgs.ext_knobs.refresh_deficit_threshold = config_get_refresh_deficit_threshold();
    ddrss_cfgs.ext_knobs.refresh_type_1x = config_get_refresh_type_1x();
    ddrss_cfgs.ext_knobs.refresh_type_2x = config_get_refresh_type_2x();
    ddrss_cfgs.ext_knobs.resp_fifo_depth = config_get_resp_fifo_depth();
    ddrss_cfgs.ext_knobs.rfm_en = config_get_rfm_en();
    ddrss_cfgs.ext_knobs.rfm_raa_dec_ref_ovr = config_get_rfm_raa_dec_ref_ovr();
    ddrss_cfgs.ext_knobs.rfm_raaimt_ovr = config_get_rfm_raaimt_ovr();
    ddrss_cfgs.ext_knobs.rfm_raammt_ovr = config_get_rfm_raammt_ovr();
    ddrss_cfgs.ext_knobs.rfm_type_1x = config_get_rfm_type_1x();
    ddrss_cfgs.ext_knobs.rfm_type_2x = config_get_rfm_type_2x();
    ddrss_cfgs.ext_knobs.rm_act_thr = config_get_rm_act_thr();
    ddrss_cfgs.ext_knobs.rm_cnt_clr = config_get_rm_cnt_clr();
    ddrss_cfgs.ext_knobs.rm_drt_clr = config_get_rm_drt_clr();
    ddrss_cfgs.ext_knobs.rm_sample_thr = config_get_rm_sample_thr();
    ddrss_cfgs.ext_knobs.rm_smc_1 = config_get_rm_smc_1();
    ddrss_cfgs.ext_knobs.rm_smc_2 = config_get_rm_smc_2();
    ddrss_cfgs.ext_knobs.rm_soc = config_get_rm_soc();
    ddrss_cfgs.ext_knobs.rm_tm_filter_bank_mask = config_get_rm_tm_filter_bank_mask();
    ddrss_cfgs.ext_knobs.rm_tm_filter_bg_mask = config_get_rm_tm_filter_bg_mask();
    ddrss_cfgs.ext_knobs.rm_tm_filter_rank_mask = config_get_rm_tm_filter_rank_mask();
    ddrss_cfgs.ext_knobs.rm_tm_filter_sub_bank_mask = config_get_rm_tm_filter_sub_bank_mask();
    ddrss_cfgs.ext_knobs.rm_tm_rec_mask = config_get_rm_tm_rec_mask();
    ddrss_cfgs.ext_knobs.rm_tm_sel_always = config_get_rm_tm_sel_always();
    ddrss_cfgs.ext_knobs.rm_tm_sel_filtered = config_get_rm_tm_sel_filtered();
    ddrss_cfgs.ext_knobs.sbr_en = config_get_sbr_en();
    ddrss_cfgs.ext_knobs.self_refresh_entry_delay = config_get_self_refresh_entry_delay();
    ddrss_cfgs.ext_knobs.self_refresh_entry_delay_unit = config_get_self_refresh_entry_delay_unit();
    ddrss_cfgs.ext_knobs.self_refresh_threshold = config_get_self_refresh_threshold();
    ddrss_cfgs.ext_knobs.self_refresh_with_ck_stop = config_get_self_refresh_with_ck_stop();
    ddrss_cfgs.ext_knobs.semi_cmd_prio = config_get_semi_cmd_prio();
    ddrss_cfgs.ext_knobs.semi_cmd_resp_limit = config_get_semi_cmd_resp_limit();
    ddrss_cfgs.ext_knobs.semi_dest_pipe_en = config_get_semi_dest_pipe_en();
    ddrss_cfgs.ext_knobs.send_rd_max_cycles = config_get_send_rd_max_cycles();
    ddrss_cfgs.ext_knobs.send_wr_max_cycle = config_get_send_wr_max_cycle();
    ddrss_cfgs.ext_knobs.send_wr_max_idle_cycles = config_get_send_wr_max_idle_cycles();
    ddrss_cfgs.ext_knobs.skip_req_exists_cob = config_get_skip_req_exists_cob();
    ddrss_cfgs.ext_knobs.skip_req_exists_pcrdgrant = config_get_skip_req_exists_pcrdgrant();
    ddrss_cfgs.ext_knobs.skip_req_exists_rcb = config_get_skip_req_exists_rcb();
    ddrss_cfgs.ext_knobs.sub_bank_hashing_mode = config_get_sub_bank_hashing_mode();
    // XML script complaining about this knob / count parameter
    // ddrss_cfgs.ext_knobs.sys_timer_delay_adder = config_get_sys_timer_delay_adder();
    ddrss_cfgs.ext_knobs.temp_comp_refresh_en = config_get_temp_comp_refresh_en();
    ddrss_cfgs.ext_knobs.temp_read_interval = config_get_temp_read_interval();
    ddrss_cfgs.ext_knobs.temp_read_interval_unit = config_get_temp_read_interval_unit();
    ddrss_cfgs.ext_knobs.ue_demand_poison_en = config_get_ue_demand_poison_en();
    ddrss_cfgs.ext_knobs.ue_max_retry = config_get_ue_max_retry();
    ddrss_cfgs.ext_knobs.ue_scrub_poison_en = config_get_ue_scrub_poison_en();
    ddrss_cfgs.ext_knobs.zq_cal_interval = config_get_zq_cal_interval();
    ddrss_cfgs.ext_knobs.zq_cal_interval_unit = config_get_zq_cal_interval_unit();
    ddrss_cfgs.ext_knobs.alert_delay = config_get_alert_delay();

    // Override with A1 stepping specific knobs if A1 stepping is detected
    bool soc_stepping_id_a1 = idhw_is_stepping_a1();
    if (soc_stepping_id_a1 == true)
    {
        ddrss_cfgs.ext_knobs.address_map_mode = config_get_a1_address_map_mode();
        ddrss_cfgs.ext_knobs.address_hashing_mode = config_get_a1_address_hashing_mode();
        ddrss_cfgs.ext_knobs.block_pre_conf_cmd_in_be = config_get_a1_block_pre_conf_cmd_in_be();
        ddrss_cfgs.ext_knobs.busif_drop_pftgt_fecq_occ = config_get_a1_busif_drop_pftgt_fecq_occ();
        ddrss_cfgs.ext_knobs.cob_occ_limit_prio0 = config_get_a1_cob_occ_limit_prio0();
        ddrss_cfgs.ext_knobs.cob_occ_limit_prio1 = config_get_a1_cob_occ_limit_prio1();
        ddrss_cfgs.ext_knobs.drop_prefetch_tgt = config_get_a1_drop_prefetch_tgt();
        ddrss_cfgs.ext_knobs.fecq_congest_lvl_0 = config_get_a1_fecq_congest_lvl_0();
        ddrss_cfgs.ext_knobs.fecq_fecqslicestartwr_count = config_get_a1_fecq_fecqslicestartwr_count();
        ddrss_cfgs.ext_knobs.fecq_fecqslicestopwr_count = config_get_a1_fecq_fecqslicestopwr_count();
        ddrss_cfgs.ext_knobs.fecq_rdwrpref_send_rd_max_idle_cycles =
            config_get_a1_fecq_rdwrpref_send_rd_max_idle_cycles();
        ddrss_cfgs.ext_knobs.fecq_rdwrpref_wr_time_out = config_get_a1_fecq_rdwrpref_wr_time_out();
        ddrss_cfgs.ext_knobs.fecq_occ_limit_prio0 = config_get_a1_fecq_occ_limit_prio0();
        ddrss_cfgs.ext_knobs.fecq_occ_limit_prio1 = config_get_a1_fecq_occ_limit_prio1();
        ddrss_cfgs.ext_knobs.pcrdgrant_delay = config_get_a1_pcrdgrant_delay();
        ddrss_cfgs.ext_knobs.rcb_en = config_get_a1_rcb_en();
        ddrss_cfgs.ext_knobs.rcb_occ_limit_prio0 = config_get_a1_rcb_occ_limit_prio0();
        ddrss_cfgs.ext_knobs.rcb_occ_limit_prio1 = config_get_a1_rcb_occ_limit_prio1();
        ddrss_cfgs.ext_knobs.send_rd_max_cycles = config_get_a1_send_rd_max_cycles();
        ddrss_cfgs.ext_knobs.send_wr_max_cycle = config_get_a1_send_wr_max_cycle();
        ddrss_cfgs.ext_knobs.send_wr_max_idle_cycles = config_get_a1_send_wr_max_idle_cycles();
    }

    platform_id = idsw_get_platform_sdv();

    bool is_warm_reset = system_info_is_warm_start() ? true : false;
    if (is_warm_reset)
    {
        ddrss_cfgs.reset_reason = DDRSS_SYS_RESET_WARM;
        start_type = "warm start";
    }
    else
    {
        ddrss_cfgs.reset_reason = DDRSS_SYS_RESET_COLD;
        start_type = "cold start";
    }

    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        platform_str = "FPGA";
        ddrss_cfgs.platform_type = DDRSS_PLATFORM_FPGA;
        // FPGA DIMM only have 1/4 of the actual DIMM capacity due to row address decoding
        // limitation. The actual DIMM size on FPGA is 64GB / 4 = 16GB.
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        // FPGA DDR runs at fixed frequency (support DDR6400 with DFI ratio 1:4)
        printf(TEXT_DDR_SPEED_GRADE_LOCKED, "DDR6400", "FPGA");
        ddrss_cfgs.ext_knobs.ddr_speed_grade = DDRSS_SPEED_6400;

        printf("Disabling UE scrub/demand poison enable: FPGA\n");
        ddrss_cfgs.ext_knobs.ue_scrub_poison_en = 0;
        ddrss_cfgs.ext_knobs.ue_demand_poison_en = 0;
    }
    else if ((platform_id == PLATFORM_EMU) || (platform_id == PLATFORM_EMU_1D) || (platform_id == PLATFORM_EMU_2D) ||
             (platform_id == PLATFORM_EMU_1D_8C) || (platform_id == PLATFORM_EMU_2D_8C))
    {
        platform_str = "EMU";
        ddrss_cfgs.platform_type = DDRSS_PLATFORM_EMU;
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        // Emulation supports DDR7200 only.
        printf(TEXT_DDR_SPEED_GRADE_LOCKED, "DDR7200", "EMU");
        ddrss_cfgs.ext_knobs.ddr_speed_grade = DDRSS_SPEED_7200;
    }
    else if (platform_id == PLATFORM_RVP_EVT_SILICON)
    {
        platform_str = "RVP";
        ddrss_cfgs.platform_type = DDRSS_PLATFORM_SILICON;
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        ddrss_cfgs.ext_knobs.ddr_speed_grade = config_get_ddr_speed_grade();

        // DDR Phy FW Load info
        ddrss_cfgs.phy_type = DDRSS_PHY_TYPE_REAL;
        ddrss_cfgs.phy_fw_type = DDRSS_PHY_FW_TRAINING;
        ddrss_cfgs.phy_fw_img_info.fimg_base = SCP_EXP_DDR_PHY_DATA_BASE;
        if (ddrss_cfgs.reset_reason == DDRSS_SYS_RESET_WARM)
        {
            ddrss_cfgs.phy_fw_img_info.fimg_check = 0;
        }
        else
        {
            ddrss_cfgs.phy_fw_img_info.fimg_check = 1;
        }
    }
    else if (platform_id == PLATFORM_SVP_SIM || platform_id == PLATFORM_SVP_MIN_CONFIG_SIM)
    {
        platform_str = "SVP";

        // For SVP only run very limited warm reset flow since the full DDR model
        // is not supported on SVP. The intention here is to provide memory map info
        // on SVP platform. The reported memory size is calculated using ddrss_mask
        // (indicates DIMM numbers) and dimm_sku (indicates DIMM size).
        // For single die, ddrss_mask = 0x3f, so total memory size is 6 * 64GB.
        // For dual die, ddrss_mask = 0xff, so total memory size is 12 * 64GB.
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;

        // Force warm reset flow and turn off INTU and RAS for SVP.
        ddrss_cfgs.reset_reason = DDRSS_SYS_RESET_WARM;
        ddrss_cfgs.ext_knobs.ras_init_en = 0;
        ddrss_cfgs.ext_knobs.intu_init_en = 0;
        start_type = "Forcing warm start for ddrss_init (SVP)";
    }
    else
    {
        printf("DDRSS init not supported on plat. Id 0x%x\n", platform_id);
        return;
    }

    printf("DDRSS die %d init: %s - %s\n", die_num, platform_str, start_type);

    // Map both DDRSS DIE0 and DIE1 cfg space through ATU
    uint32_t d0_start = ddrss_atu_map_cfg_space(SOC_D0);
    uint32_t d1_start = ddrss_atu_map_cfg_space(SOC_D1);
    ddrss_cfgs.ddrss_base_die[SOC_D0] = d0_start;
    ddrss_cfgs.ddrss_base_die[SOC_D1] = d1_start;

    BUG_ASSERT_PARAM(ddrss_set_die_base(die_num, die_num == DIE_0 ? d0_start : d1_start) == SILIBS_SUCCESS, die_num, 0);

    // Sync up with mesh config on NUMA, address hash and MC mapping cfgs
    cmn800_snf_to_mc_config_t* mc_config = cmn800_generate_ddr_mc_map_from_cached_config();
    BUG_ASSERT_PARAM(mc_config != NULL, die_num, 0);
    ddrss_cfgs.numa_cfg = mc_config->is_numa_enabled ? DDRSS_NUMA_CFG_NUMA : DDRSS_NUMA_CFG_UMA;
    ddrss_cfgs.hash_addr_bits_sel = mc_config->hash_select;
    memcpy(ddrss_cfgs.mc_mapping_order, mc_config->ddr_mc_map, sizeof(ddrss_cfgs.mc_mapping_order));
    if (platform_id == PLATFORM_RVP_EVT_SILICON)
    {
        // For RVP platform, use I3C detected DIMM population and sku.
        ddrss_cfgs.dimm_sku = mc_config->i3c_dimm_sku;
        ddrss_cfgs.ext_knobs.ddrss_mask = mc_config->i3c_ddrss_mask;
    }

    // Set DDRSS ext_knobs from config knobs
    if (config_get_phy_fw_diag_en())
    {
        ddrss_cfgs.ext_knobs.phy_fw_diag_en = 1;
        printf("DDRSS - phy_fw_diag_en enabled\n");

        // Disable Watchdog
        wdog_cmsdk_apb_disable();         // Disable watchdog
        wdog_cmsdk_apb_lock_unlock(true); // Lock counter
    }

    // Set up per-lane margin buffer
    ddrss_cfgs.dq_lane_margin_base = (uint64_t)(uintptr_t)&ddrss_phy_training_dq_margin;

    // Map both DDRSS fips test space through ATU
    if (!is_warm_reset && ddrss_cfgs.ext_knobs.fips_kat_en)
    {
        if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP) ||
            ((platform_id == PLATFORM_RVP_EVT_SILICON) && (ddrss_cfgs.reset_reason == DDRSS_SYS_RESET_COLD)))
        {
            ddrss_cfgs.fips_kat_cfg.ns_mem_base[SOC_D0] = ddrss_atu_map_fips_ns_space(SOC_D0);
            ddrss_cfgs.fips_kat_cfg.rt_mem_base[SOC_D0] = ddrss_atu_map_fips_rt_space(SOC_D0);
            ddrss_cfgs.fips_kat_cfg.ns_mem_base[SOC_D1] = ddrss_atu_map_fips_ns_space(SOC_D1);
            ddrss_cfgs.fips_kat_cfg.rt_mem_base[SOC_D1] = ddrss_atu_map_fips_rt_space(SOC_D1);
        }
    }

    // Run DDRSS init
    sts = ddrss_init(&ddrss_cfgs);
    if (sts != SILIBS_SUCCESS)
    {
        ddrss_phy_training_error_info_t phy_err_info;
        led_status_codes_t bsc_status = LED_STATUS_CODE_SCP_E_DDR0_TRAINING;
        boot_status_req_t bsc_req_mem = {0};

        /**
         * If ddrss_init returns SILIBS_E_STATE erros it indicates that
         * the DDRSS init failed due to PHY training failure.
         */
        if (sts == SILIBS_E_STATE && (ddrss_get_phy_training_failure(&phy_err_info) == SILIBS_SUCCESS))
        {
            bsc_status = LED_STATUS_CODE_SCP_E_DDR0_TRAINING + DDRSS_GET_LOCAL_DDRSS(phy_err_info.mc);
        }

        /**
         * If we cannot get the PHY training error info, we still need to send
         * boot status code to HSP to indicate that DDRSS init failed.
         * Send DDR0 SS (Dies' perspective) as default.
         * TODO: Add a new boot status code for DDRSS init failure which
         * indicates a failure other than PHY training.
         */

        /**
         * This will be a sync call as we are running in runtime init phase
         * and BSC module makes sure that BSC is sent to HSP synchronously
         * during runtime init phase.
         */
        post_led_status(&bsc_req_mem, bsc_status);

        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    }

    if (!is_warm_reset && ddrss_cfgs.ext_knobs.fips_kat_en)
    {
        // Unmap FIPS test space
        if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP) ||
            ((platform_id == PLATFORM_RVP_EVT_SILICON) && (ddrss_cfgs.reset_reason == DDRSS_SYS_RESET_COLD)))
        {
            ddrss_atu_unmap_fips_space(SOC_D0);
            ddrss_atu_unmap_fips_space(SOC_D1);

            if ((ddrss_key_loading_req & DDRSS_PROD_FW_FIPS_KEY_LOADED) &&
                (ddrss_key_loading_req & DDRSS_PROD_FW_FIPS_TEST_NOTIFIED) &&
                (ddrss_key_loading_req & DDRSS_PROD_FW_PROD_KEY_LOADED))
            {
                printf("DDRSS - HSP notifcation: FIPS KAT test PASSED!\n");
            }
            else
            {
                printf("DDRSS - HSP notifcation: FIPS KAT test FAILED!\n");
            }
        }
    }

    // Unmap previous ATU mapping for other DIE
    if (die_num == DIE_0)
    {
        ddrss_atu_unmap_cfg_space(SOC_D1);
    }
    else
    {
        ddrss_atu_unmap_cfg_space(SOC_D0);
    }

    /* Register the vendor and standard error domain with the health monitor */
    hm_register_error_domain(ACPI_ERROR_DOMAIN_STD_MEMORY, &STD_MEMORY_ERROR_DOMAIN_GUID, "Std Memory Error", ddr_error_injection_cb, NULL);
    hm_register_error_domain(ACPI_ERROR_DOMAIN_DDR, &DDR_ERROR_DOMAIN_FRU_GUID, "DDR Error Domain", ddr_error_injection_cb, NULL);
    hm_register_error_domain(ACPI_ERROR_DOMAIN_RHTLM, &DDR_RHTL_ERROR_DOMAIN_FRU_GUID, "RowHammer Domain", ddr_error_injection_cb, NULL);

    printf("DDRSS init exit\n");
}

void prod_ddrss_pcr_init(KNG_DIE_ID die_num)
{
    // Set DDRSS mask for 1D boot (12 MCs) or 2D boot (24 MCs)
    uint32_t ddrss_mask = idhw_is_single_die_boot_en() ? 0x03F : 0xFFF;

    uintptr_t start_addr = ddrss_atu_map_cfg_space(die_num);
    BUG_ASSERT_PARAM(ddrss_set_die_base(die_num, start_addr) == SILIBS_SUCCESS, die_num, 0);
    pcr_ddrss_configure_clock_and_pcr_reset(ddrss_mask, die_num);
    ddrss_atu_unmap_cfg_space(die_num);
}

ddrss_phy_training_dq_margin_t* ddrss_get_training_margin_base(void)
{
    return &ddrss_phy_training_dq_margin[0];
}

int ddrss_load_crypto_key(uint32_t mc, uint32_t msg, uint32_t timeout_us)
{
    // 'mc' is currently unused but reserved for future use.
    (void)mc;
    // 'timeout_us' is reserved for future use to allow configurable timeouts.
    // Consider integrating it into the send/receive call when appropriate.
    (void)timeout_us;

    if (system_info_is_warm_start())
    {
        // for warm reboot, skipping key loading.
        printf("DDRSS key loading skipped for warm reset\n");
        return SILIBS_SUCCESS;
    }

    bool fips_kat_enabled = config_get_fips_kat_en();
    bool encryption_enabled = config_get_pas_encryption_en_mask() ? true : false;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    uint32_t req_cmd = 0;
    uint32_t rsp_cmd = 0;
    int sts = SILIBS_E_SUPPORT;
    kng_hsp_mailbox_msg mailbox_msg = {0};
    size_t recv_msg_size_bytes = 0;
    fpfw_status_t icc_status = {0};

    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_hspmbx");
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP) ||
        (platform_id == PLATFORM_RVP_EVT_SILICON))
    {
        if ((msg == DDRSS_HSP_MSG_FIPS_KEYS_LOADED) && fips_kat_enabled)
        {
            req_cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_FIPS_KEYS_REQ;
            rsp_cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_FIPS_KEYS_RSP;
        }
        else if (msg == DDRSS_HSP_MSG_PROD_KEYS_LOADED)
        {
            prod_key_load_req_cnt++;
            req_cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_PROD_KEYS_REQ;
            rsp_cmd = HSP_MAILBOX_CMD_DDRSS_DEPLOY_PROD_KEYS_RSP;

            // When FIPS and encryption both are on, there will be 2 prod key loading request from silibs.
            // Only the 2nd request will be sent to HSP for performance optimization.
            if (fips_kat_enabled && encryption_enabled)
            {
                if (prod_key_load_req_cnt < 2)
                {
                    req_cmd = 0;
                    sts = SILIBS_SUCCESS;
                }
            }
        }

        if (req_cmd > 0)
        {
            mailbox_msg.header.cmd = req_cmd;
            icc_status = fpfw_icc_base_send_recv_sync(icc_ctx, &mailbox_msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

            //! Verify sync return status & response message
            BUG_ASSERT_PARAM(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_status, 0);
            BUG_ASSERT_PARAM(recv_msg_size_bytes > 0, recv_msg_size_bytes, 0);
            BUG_ASSERT_PARAM(mailbox_msg.header.cmd == rsp_cmd, mailbox_msg.header.cmd, rsp_cmd);
            BUG_ASSERT_PARAM(mailbox_msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS, mailbox_msg.rsp.status, 0);

            if (req_cmd == HSP_MAILBOX_CMD_DDRSS_DEPLOY_PROD_KEYS_REQ)
            {
                ddrss_key_loading_req |= DDRSS_PROD_FW_PROD_KEY_LOADED;
                // check the ddr reserved regionversion from the response and compare with the expected version
                uint32_t ddrss_fw_version = mailbox_msg.rsp.status_ex;
                printf("DDRSS PROD KEY: DDRSS reserved region version received from HSP: 0x%lx, "
                       "expected version: "
                       "0x%x\n",
                       (unsigned long)ddrss_fw_version,
                       DDRSS_RESERVED_REGIONS_REVISION);
            }
            else if (req_cmd == HSP_MAILBOX_CMD_DDRSS_DEPLOY_FIPS_KEYS_REQ)
            {
                ddrss_key_loading_req |= DDRSS_PROD_FW_FIPS_KEY_LOADED;
            }

            sts = SILIBS_SUCCESS;
        }

        if ((msg == DDRSS_HSP_MSG_FIPS_KEYS_LOADED) && fips_kat_enabled)
        {
            memset(&mailbox_msg, 0x0, sizeof(kng_hsp_mailbox_msg));

            mailbox_msg.fips_key_test_status_notify.header.cmd = HSP_MAILBOX_CMD_DDRSS_FIPS_KEY_TEST_STATUS_NOTIFY;

            icc_status = fpfw_icc_base_send_sync(icc_ctx, &mailbox_msg, sizeof(kng_hsp_mailbox_msg));

            //! Verify sync return status
            BUG_ASSERT_PARAM(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS, icc_status, 0);

            ddrss_key_loading_req |= DDRSS_PROD_FW_FIPS_TEST_NOTIFIED;
        }
    }

    return sts;
}

int32_t ddrss_update_ppr_completion(ddrs_spd_addr_info_t* addr_info, ddrss_res_info_t* res_info)
{
    /**
     * @brief
     * This API updaets the SEL record and SPD status bytes after PPR completion.
     * 1. The SEL record needs to be sent to MCP which will then update it in BMC.
     * 2. The SEL service provides API to log the record which can be used here.
     * 3. A lot of the SPD fields are read-write-modify, hence the bytes need to be read first.
     * 4. To reduce I3C access to SPD region, plan is to read 16 bytes starting from byte 640 (DDR5).
     * 5. Modify the required bytes in the read buffer and write back the entire buffer.
     *
     * SPD fields: (DDR4/DDR5)
     *  Byte 384/640 = SPD Version

        Byte 385/641 = PPR Count

        Byte 386/642 = Status
        0 no repair
        1 ppr passed
        2 ppr failed,
        3 ppr failed overflow
        4 ppr failed temperature

        Byte 387/643 = number of failed PPR occurrences

        Byte 388/644 = number of PPR executions

        Byte 391:398/647:654 = address

        address is a combination of the below fields and has a total size of 8-bytes

        uint64_t socket : 4;    //up to 16
        uint64_t ch:4;          //up to 16
        uint64_t dimm:1;        //primary/secondary 2dpc
        uint64_t rank:2;        //front/back
        uint64_t subrank:3;     //could be 8h 3ds
        uint64_t subCh:1;       //left/right

        uint64_t bankGroup:3;   //8 bank groups
        uint64_t bank:2;        //4 banks
        uint64_t row:17;        //0-16 (no 64Gb support)
        uint64_t dq:7;          //allow 128 bits but only need 40 /subCh
        uint64_t temp:8;        //-40 to 100 
        uint64_t resrved:12;
     *
     */

    sel_event_record_t sel_event;
    i3c_cmd_t s_i3c_cmd = {0};
    int32_t status = E_PPR_STATUS_UPDATE_OK;
    uint8_t spd_data[SIZE_16_BYTES];
    uint16_t data_offset = SPD_OFFSET_DDR5;
    uint16_t data_len = 0;
    uint16_t vendor_id = get_dimm_vendor_id(addr_info->dimm);

    // Vendor ID readback was not valid
    if (vendor_id == DIMM_UNKNOWN)
    {
        // TODO ADO: 3144540 Add an event trace here?
        status = E_PPR_STATUS_UPDATE_INVALID_VENDOR_ID;
        goto exit;
    }

    status = ddr_i3c_interface_read_spd_nvm_data(&s_i3c_cmd, addr_info->dimm, spd_data, &data_len, &data_offset, SIZE_16_BYTES);
    if (status != SILIBS_SUCCESS)
    {
        // TODO ADO: 3144540 Add an event trace here?
        FPFW_DBGPRINT_ERROR("SPD_READ_FAIL: %d\r\n", status);
        status = E_PPR_STATUS_UPDATE_SPD_READ_FAIL;
        goto exit;
    }

#ifdef SPD_READ_DEBUGPRINT_ENABLE
    FPFW_DBGPRINT_INFO("+++++++++++++++++++++++++++SPD_READ_BACK+++++++++++++++++++++++++++\r\n");
    for (uint8_t i = 0; i < SIZE_16_BYTES; i++)
    {
        FPFW_DBGPRINT_INFO("SPD_DATA[%d]:0x%x\r\n", i, spd_data[i]);
    }
#endif

// The ifdef is to allow for testing I3C write without modifying data hence avoiding any corruption as a precaution
#ifndef SPD_UPDATE_TESTING
    spd_data[0] = SPD_VERSION;
    spd_data[1] += 1;                       // Increment PPR count
    spd_data[2] = res_info->spd_ppr_status; // Update status
    if (res_info->spd_ppr_status >= E_PPR_STATUS_FAILED)
    {
        spd_data[3] += 1; // Increment number of failed PPR occurrences
    }
    spd_data[4] += 1; // Increment number of PPR executions
    memcpy(&spd_data[7], addr_info, sizeof(ddrs_spd_addr_info_t));
#endif

    // Write data has a max size of 8-bytes, hence executing a hardcoded 2-loop write of 8-bytes each
    for (uint8_t i = 0; i < (SPD_STRUCT_SIZE_DDR5 / SIZE_8_BYTES); i++)
    {
        data_offset = SPD_OFFSET_DDR5 + (i * SIZE_8_BYTES);
        status = ddr_i3c_interface_write_spd_nvm_data(&s_i3c_cmd,
                                                      addr_info->dimm,
                                                      &spd_data[(i * SIZE_8_BYTES)],
                                                      &data_len,
                                                      &data_offset,
                                                      SIZE_8_BYTES);
        if (status != SILIBS_SUCCESS)
        {
            // TODO ADO: 3144540 Add an event trace here?
            FPFW_DBGPRINT_ERROR("SPD_WRITE_FAIL: %d\r\n", status);
            status = E_PPR_STATUS_UPDATE_SPD_WRITE_FAIL;
            goto exit;
        }
    }

    sel_event.ddr_info.record_id = 0; // TODO ADO: 3144540
    sel_event.ddr_info.record_type = SEL_RECORD_TYPE_DDR_PPR;
    sel_event.default_info.timestamp = gtimer_prodfw_get_counter();
    sel_event.ddr_info.manufacturer_id = 0x0; // TODO ADO: 3144540
    sel_event.ddr_info.vendor_id = vendor_id;
    sel_event.ddr_info.device_id = 0; // TODO ADO: 3144540
    sel_event.ddr_info.oem_data_1 = res_info->sel_ppr_status;
    sel_event.ddr_info.oem_data_2 = res_info->num_repair_rows;

    // The SEL framework must queue the SEL log and transmit it to MCP once MCP boot is done
    status = log_sel_event(&sel_event);
    if (status != KNG_SUCCESS)
    {
        // TODO ADO: 3144540 Add an event trace here?
        status = E_PPR_STATUS_UPDATE_SEL_LOG_FAIL;
        FPFW_DBGPRINT_ERROR("SEL_LOG_FAIL: %d\r\n", status);
    }

exit:
    return status;
}