//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss.c
 *  Main initialization for various DDR subsystem components
 */

/*------------- Includes -----------------*/

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
    atu_map_entry_t atu_map_struct[NUM_DIE];
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

    platform_id = idsw_get_platform_sdv();
    if (platform_id == PLATFORM_SVP_SIM)
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
        // TBD: For RVP platform, after I3C DIMM SPD detection is added,
        //      the dimm_sku needs to be updated using detection results.
        ddrss_cfgs.dimm_sku = DDR5_RDIMM_2Rx4_16Gb_64GB;
        ddrss_cfgs.ext_knobs.ddr_speed_grade =
            config_get_ddr_speed_grade(); // TODO: Update default in Config XML? DDRSS_SPEED_7200;
    }
    else
    {
        printf("DDRSS init is not supported yet on platform Id 0x%x\n", platform_id);
        return;
    }

    printf("DDRSS init for %s\n", platform_str);

    atu_entry_attr_t atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
    // DDRSS init flow needs communication between 2 dies.
    // Both dies need to have access to local and remote cfg spaces.
    atu_map_struct[SOC_D0].ap_base_address = AP_TOP_D0_DDRSS_0_ADDRESS;
    atu_map_struct[SOC_D0].mscp_start_address = 0;
    atu_map_struct[SOC_D0].mscp_end_address =
        (AP_TOP_D0_DDRSS_1_ADDRESS - AP_TOP_D0_DDRSS_0_ADDRESS) * (DDRSS_MAX_SS_NUM / 2) - 1;
    atu_map_struct[SOC_D0].attribute.as_uint32 = atu_root_attr.as_uint32;

    // Map both DDRSS DIE0 and DIE1 cfg space through ATU
    memcpy(&atu_map_struct[SOC_D1], &atu_map_struct[SOC_D0], sizeof(atu_map_entry_t));

    sts = atu_map(ATU_ID_MSCP, &atu_map_struct[SOC_D0]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    atu_map_struct[SOC_D1].ap_base_address = AP_TOP_D1_DDRSS_0_ADDRESS;
    sts = atu_map(ATU_ID_MSCP, &atu_map_struct[SOC_D1]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    // Update DDRSS cfgs to match actual platform cfg
    ddrss_cfgs.die_id = (DIE_INSTANCE)die_num;
    ddrss_cfgs.ddrss_base_die[SOC_D0] = atu_map_struct[SOC_D0].mscp_start_address;
    ddrss_cfgs.ddrss_base_die[SOC_D1] = atu_map_struct[SOC_D1].mscp_start_address;
    ddrss_cfgs.debug_level = DDRSS_DEBUG_LEVEL_INFO;
    ddrss_cfgs.ext_knobs.interleave_mc_cnt = 0;
    if (idhw_is_single_die_boot_en())
    {
        // Set DDRSS mask for 1D boot (12 MCs)
        ddrss_cfgs.ext_knobs.ddrss_mask = 0x03F;
    }
    else
    {
        // Set DDRSS mask for 2D boot (24 MCs)
        ddrss_cfgs.ext_knobs.ddrss_mask = 0xFFF;
    }

    // Sync up with mesh config on NUMA, address hash and MC mapping cfgs
    cmn800_snf_to_mc_config_t* mc_config = cmn800_generate_ddr_mc_map_from_cached_config();
    FPFW_RUNTIME_ASSERT(mc_config != NULL);
    ddrss_cfgs.numa_cfg = mc_config->is_numa_enabled ? DDRSS_NUMA_CFG_NUMA : DDRSS_NUMA_CFG_UMA;
    ddrss_cfgs.hash_addr_bits_sel = mc_config->hash_select;
    memcpy(ddrss_cfgs.mc_mapping_order, mc_config->ddr_mc_map, sizeof(ddrss_cfgs.mc_mapping_order));

    // Run DDRSS init
    sts = ddrss_init(&ddrss_cfgs);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    // Unmap previous ATU mapping for other DIE
    if (die_num == DIE_0)
    {
        sts = atu_unmap(ATU_ID_MSCP, &atu_map_struct[SOC_D1]);
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    }
    else
    {
        sts = atu_unmap(ATU_ID_MSCP, &atu_map_struct[SOC_D0]);
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    }

    printf("DDRSS init exit\n");
}
