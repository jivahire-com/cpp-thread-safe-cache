//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file css.c
 *  This modules initializes various CSS components
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <atu_lib.h>
#include <clocks_sequence.h>
#include <css.h>
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
#define ARSM_BASE_ADDRESS_DIEn(n) \
    ((AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS) + ((AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS) * (n)))
#define ARSM_SIZE (AP_TOP_D0_ARSM_SHARED_SRAM_SIZE)

atu_map_entry_t atu_global_map_die0[] = {
    {
        .ap_base_address = ARSM_BASE_ADDRESS_DIEn(0),
        .mscp_start_address = ATU_AP_ARSM_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_ARSM_ADDRESS + ARSM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_NS},
    },
};

atu_map_entry_t atu_global_map_die1[] = {
    {
        .ap_base_address = ARSM_BASE_ADDRESS_DIEn(1),
        .mscp_start_address = ATU_AP_ARSM_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_ARSM_ADDRESS + ARSM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_NS},
    },
};

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
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    clocks_sequence_css_pre_mesh_init_t scp_clocks_pre_mesh_param = {};
    scp_clocks_pre_mesh_param.system_ppu_opmode = PPU_V1_OPMODE_01;
    scp_clocks_pre_mesh_param.skip_css_pcr_init = false;
    scp_clocks_pre_mesh_param.skip_msxp_pcr_init = false;
    scp_clocks_pre_mesh_param.system_smmu_gpt_enabled = false;
    scp_clocks_pre_mesh_param.system_smmu_l0gptsz = 0;
    int sts = clocks_sequence_css_pre_mesh_init(&scp_clocks_pre_mesh_param);
    FPFW_RUNTIME_ASSERT(sts == 0);

    // Initialization of ATU
    atu_map_entry_t* atu_global_map = (die_num == 0) ? atu_global_map_die0 : atu_global_map_die1;
    sts = atu_init(ATU_ID_MSCP, atu_global_map, ARRAY_SIZE(&atu_global_map));
    FPFW_RUNTIME_ASSERT(sts == 0);
}

void css_configure_system_tower(uint8_t die_num)
{
    // System tower should be configured by the HSP in two stages - presystop and post-systop bringup
    // Since this function is running on SCP after systop de-assertion, we can configure system tower in one-shot
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    int sts = atu_map(ATU_ID_MSCP, &atu_system_tower_map[die_num]);
    FPFW_RUNTIME_ASSERT(sts == 0);
    tower_system_control_configure_aon_apu(atu_system_tower_map[die_num].mscp_start_address, die_num);
    tower_system_control_configure_aon_sam(atu_system_tower_map[die_num].mscp_start_address, die_num);

    tower_system_control_configure_systop_apu(atu_system_tower_map[die_num].mscp_start_address, die_num);
    tower_system_control_configure_systop_sam(atu_system_tower_map[die_num].mscp_start_address, die_num);
    sts = atu_unmap(ATU_ID_MSCP, &atu_system_tower_map[die_num]);
    FPFW_RUNTIME_ASSERT(sts == 0);
}

void css_post_mesh_init()
{
    clocks_sequence_css_post_mesh_init_t scp_clocks_post_mesh_param = {};
    scp_clocks_post_mesh_param.skip_periph_pcr_init = false;
    int sts = clocks_sequence_css_post_mesh_init(&scp_clocks_post_mesh_param);
    FPFW_RUNTIME_ASSERT(sts == 0);
}
