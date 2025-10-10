//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file css.c
 *  This modules initializes various CSS components
 */

/*------------- Includes -----------------*/

#include <atu_lib.h>
#include <bug_check.h>
#include <clocks_sequence.h>
#include <clocks_sequence_knobs.h>
#include <css.h>
#include <fpfw_cfg_mgr.h>
#include <ift_fw.h>
#include <kng_soc_constants.h>
#include <ppu_v1.h>
#include <silibs_ap_top_regs.h>
#include <silibs_common.h>
#include <stdbool.h>
#include <stdint.h>
#include <tower_system_control.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define MAX_SYSTEM_TOWER_INSTANCES (2)
atu_map_entry_t atu_system_tower_map[MAX_SYSTEM_TOWER_INSTANCES] = {
    {
        // D0-System Tower
        .ap_base_address = AP_TOP_D0_SYSTEM_INTER_NIC_GPV_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_SYSTEM_INTER_NIC_GPV_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        // D1-System Tower
        .ap_base_address = AP_TOP_D1_SYSTEM_INTER_NIC_GPV_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D1_SYSTEM_INTER_NIC_GPV_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
};

/*------------- Functions ----------------*/

// TODO: WI 1726113 Create a component initialization library for these APIs
void css_pre_mesh_init(uint8_t die_num)
{
    BUG_ASSERT_PARAM(die_num < NUM_DIE, die_num, 0);

    intclk_cfg_t intclk_knobs = config_get_clock_seq_knobs().clocks_intclk_cfg;

    clocks_sequence_css_pre_mesh_init_t scp_clocks_pre_mesh_param = {};
    scp_clocks_pre_mesh_param.system_ppu_opmode = PPU_V1_OPMODE_01;
    scp_clocks_pre_mesh_param.skip_css_pcr_init = false;
    scp_clocks_pre_mesh_param.skip_msxp_pcr_init = false;
    // GPT enable / disable would be a knob controlled by TF-A. This param here doesn't enable GPT
    // but configures the GPT size (needs to be programmed before smmu is released out of reset) which can be
    // done regardless of whether GPT/GPC is enabled or not.
    scp_clocks_pre_mesh_param.system_smmu_gpt_enabled = true;
    scp_clocks_pre_mesh_param.system_smmu_l0gptsz = 0;
    scp_clocks_pre_mesh_param.intclk_cfg = &intclk_knobs;
    int sts = clocks_sequence_css_pre_mesh_init(&scp_clocks_pre_mesh_param);
    BUG_ASSERT_PARAM(sts == 0, sts, 0);
}

void css_configure_system_tower(uint8_t die_num)
{
    // System tower should be configured by the HSP in two stages - presystop and post-systop bringup
    // Since this function is running on SCP after systop de-assertion, we can configure system tower in one-shot
    BUG_ASSERT_PARAM(die_num < NUM_DIE, die_num, 0);

    int sts = atu_map(ATU_ID_MSCP, &atu_system_tower_map[die_num]);
    BUG_ASSERT_PARAM(sts == 0, sts, 0);
    tower_system_control_configure_apu_aon(atu_system_tower_map[die_num].mscp_start_address, die_num, false);
    tower_system_control_configure_aon_sam(atu_system_tower_map[die_num].mscp_start_address, die_num);

    tower_system_control_configure_apu_systop(atu_system_tower_map[die_num].mscp_start_address, die_num, false);
    tower_system_control_configure_systop_sam(atu_system_tower_map[die_num].mscp_start_address, die_num);
    sts = atu_unmap(ATU_ID_MSCP, &atu_system_tower_map[die_num]);
    BUG_ASSERT_PARAM(sts == 0, sts, 0);
}

void css_post_mesh_init()
{
    clocks_sequence_css_post_mesh_init_t scp_clocks_post_mesh_param = {};

    scp_clocks_post_mesh_param.skip_periph_pcr_init = false;
    scp_clocks_post_mesh_param.ift_mode_en = ift_is_enabled();
    sys_counter_delay_cfg_t sys_counter_delay_cfg = config_get_clock_seq_knobs().sys_counter_delay_cfg;
    scp_clocks_post_mesh_param.sys_counter_delay = &sys_counter_delay_cfg;
    int sts = clocks_sequence_css_post_mesh_init(&scp_clocks_post_mesh_param);
    BUG_ASSERT_PARAM(sts == 0, sts, 0);
}
