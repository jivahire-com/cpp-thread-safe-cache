//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss.c
 *  Main initialization for various DDR subsystem components
 */

/*------------- Includes -----------------*/
#include "ddr_atu_map.h"

#include <FPFwInterrupts.h>
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <cmn800.h>
#include <ddr_i3c.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
#include <idhw.h> // for idhw_is_single_die_boot_en
#include <idsw_kng.h>
#include <interrupts.h>
#include <memory_map/ddrss_reserved_regions.h>
#include <memory_map/mscp_exp_rmss_memory_map.h>
#include <pcr_ddrss.h>
#include <silibs_ap_top_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*-- Declarations (Statics and globals) --*/
static uint32_t ddrss_interrupt_id[6] = {0, 1, 2, 3, 4, 5};

/*------------- Functions ----------------*/
void prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    int sts = SILIBS_SUCCESS;
    ddrss_cfg_knobs_t ddrss_cfgs;
    KNG_PLAT_ID platform_id;
    uint32_t interrupt_idx;

    // 1 DDRSS contains 2 memory controllers
    // PHY level is under MC level in heirarchy, 1 PHY per DDRSS.
    printf("DDRSS init start - die %d\n", die_num);

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

        FPFW_RUNTIME_ASSERT(intr_status == 0);
    }

    // Get the default DDRSS cfgs
    sts = ddrss_get_config(&ddrss_cfgs);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

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

    // Update DDRSS cfgs to match actual platform cfg
    ddrss_cfgs.die_id = (DIE_INSTANCE)die_num;
    ddrss_cfgs.debug_level = DDRSS_DEBUG_LEVEL_INFO;
    ddrss_cfgs.ext_knobs.interleave_mc_cnt = 0;
    ddrss_cfgs.ext_knobs.intu_init_en = 1;
    ddrss_cfgs.ext_knobs.ras_init_en = 1;

    // Load other config knobs - Up to date for Pre-TO requirements
    ddrss_cfgs.ext_knobs.address_hashing_mode = config_get_address_hashing_mode();
    ddrss_cfgs.ext_knobs.address_map_mode = config_get_address_map_mode();
    ddrss_cfgs.ext_knobs.cmd_merge_en = config_get_cmd_merge_en();
    ddrss_cfgs.ext_knobs.dram_power_down_entry_delay = config_get_dram_power_down_entry_delay();
    ddrss_cfgs.ext_knobs.dram_refresh_mode = config_get_dram_refresh_mode();
    ddrss_cfgs.ext_knobs.ecc_ce_th = config_get_ecc_ce_th();
    ddrss_cfgs.ext_knobs.dram_power_down_entry_delay = config_get_dram_power_down_entry_delay();
    ddrss_cfgs.ext_knobs.erhm_en = config_get_erhm_en();
    ddrss_cfgs.ext_knobs.media_ecc_mode = config_get_media_ecc_mode();
    ddrss_cfgs.ext_knobs.media_scrub_en = config_get_media_scrub_en();
    ddrss_cfgs.ext_knobs.page_open_timer = config_get_page_open_timer();
    ddrss_cfgs.ext_knobs.phy_hdt_ctrl = config_get_phy_hdt_ctrl();
    ddrss_cfgs.ext_knobs.phy_margin_th = config_get_phy_margin_th();
    ddrss_cfgs.ext_knobs.ref_temp_changed = config_get_ref_temp_changed();
    ddrss_cfgs.ext_knobs.ref_temp_high = config_get_ref_temp_high();
    ddrss_cfgs.ext_knobs.sbr_en = config_get_sbr_en();
    ddrss_cfgs.ext_knobs.sub_bank_hashing_mode = config_get_sub_bank_hashing_mode();

    GENERATE_ARRAY_OF_RSVD_REGIONS
    DDRSS_PAS_REGIONS
    ddrss_cfgs.pas_range_list = (uintptr_t)&_mem_region_list;
    ddrss_cfgs.ext_knobs.pas_encryption_en_mask = config_get_pas_encryption_en_mask();

    platform_id = idsw_get_platform_sdv();
    if (platform_id == PLATFORM_SVP_SIM || platform_id == PLATFORM_SVP_MIN_CONFIG_SIM)
    {
        // SVP does not support full DDR model, skip it.
        printf("DDRSS init is skipped for SVP\n");
        return;
    }

    const char* platform_str;
    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        platform_str = "FPGA";
        ddrss_cfgs.platform_type = DDRSS_PLATFORM_FPGA;
        // FPGA DIMM only have 1/4 of the actual DIMM capacity due to row address decoding
        // limitation. The actual DIMM size on FPGA is 64GB / 4 = 16GB.
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        // FPGA DDR runs at fixed frequency (support DDR6400 with DFI ratio 1:4)
        printf("DDRSS - ddr_speed_grade locked to DDR6400 for FPGA\n");
        ddrss_cfgs.ext_knobs.ddr_speed_grade = DDRSS_SPEED_6400;

        printf("Disabling UE scrub/demand poison enable for FPGA\n");
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
        printf("DDRSS - ddr_speed_grade locked to DDR7200 for EMU\n");
        ddrss_cfgs.ext_knobs.ddr_speed_grade = DDRSS_SPEED_7200;
    }
    else if (platform_id == PLATFORM_RVP_EVT_SILICON)
    {
        platform_str = "RVP";
        ddrss_cfgs.platform_type = DDRSS_PLATFORM_SILICON;
        // TODO: For RVP platform, after I3C DIMM SPD detection is added,
        //      the dimm_sku and ddrss_mask needs to be updated using detection results.
        // ddrss_cfgs.ext_knobs.ddrss_mask = get_i3c_dimm_detected(); // Will enable on separate PR..

        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        ddrss_cfgs.ext_knobs.ddr_speed_grade = config_get_ddr_speed_grade();

        // DDR Phy FW Load info
        ddrss_cfgs.phy_type = DDRSS_PHY_TYPE_REAL;
        ddrss_cfgs.phy_fw_type = DDRSS_PHY_FW_TRAINING;
        ddrss_cfgs.phy_fw_img_info.fimg_base = SCP_EXP_DDR_PHY_DATA_BASE;
        ddrss_cfgs.phy_fw_img_info.fimg_check = 1;
    }
    else
    {
        printf("DDRSS init is not supported yet on platform Id 0x%x\n", platform_id);
        return;
    }

    printf("DDRSS init for %s\n", platform_str);

    // Map both DDRSS DIE0 and DIE1 cfg space through ATU
    uint32_t d0_start = ddrss_atu_map_cfg_space(SOC_D0);
    uint32_t d1_start = ddrss_atu_map_cfg_space(SOC_D1);
    ddrss_cfgs.ddrss_base_die[SOC_D0] = d0_start;
    ddrss_cfgs.ddrss_base_die[SOC_D1] = d1_start;

    FPFW_RUNTIME_ASSERT(ddrss_set_die_base(die_num, die_num == DIE_0 ? d0_start : d1_start) == SILIBS_SUCCESS);

    // Sync up with mesh config on NUMA, address hash and MC mapping cfgs
    cmn800_snf_to_mc_config_t* mc_config = cmn800_generate_ddr_mc_map_from_cached_config();
    FPFW_RUNTIME_ASSERT(mc_config != NULL);
    ddrss_cfgs.numa_cfg = mc_config->is_numa_enabled ? DDRSS_NUMA_CFG_NUMA : DDRSS_NUMA_CFG_UMA;
    ddrss_cfgs.hash_addr_bits_sel = mc_config->hash_select;
    memcpy(ddrss_cfgs.mc_mapping_order, mc_config->ddr_mc_map, sizeof(ddrss_cfgs.mc_mapping_order));

    // Set DDRSS ext_knobs from config knobs
    if (config_get_phy_fw_diag_en())
    {
        ddrss_cfgs.ext_knobs.phy_fw_diag_en = 1;
        printf("DDRSS - phy_fw_diag_en enabled\n");
    }

    // Run DDRSS init
    sts = ddrss_init(&ddrss_cfgs);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    // Unmap previous ATU mapping for other DIE
    if (die_num == DIE_0)
    {
        ddrss_atu_unmap_cfg_space(SOC_D1);
    }
    else
    {
        ddrss_atu_unmap_cfg_space(SOC_D0);
    }

    printf("DDRSS init exit\n");
}

void prod_ddrss_pcr_init(KNG_DIE_ID die_num)
{
    // Set DDRSS mask for 1D boot (12 MCs) or 2D boot (24 MCs)
    uint32_t ddrss_mask = idhw_is_single_die_boot_en() ? 0x03F : 0xFFF;

    uintptr_t start_addr = ddrss_atu_map_cfg_space(die_num);
    FPFW_RUNTIME_ASSERT(ddrss_set_die_base(die_num, start_addr) == SILIBS_SUCCESS);
    pcr_ddrss_configure_clock_and_pcr_reset(ddrss_mask, die_num);
    ddrss_atu_unmap_cfg_space(die_num);
}