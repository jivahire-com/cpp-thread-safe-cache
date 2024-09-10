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
#include <ddrss.h>
#include <ddrss_lib.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <silibs_ap_top_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DDRSS_FPGA_ATU_MAPPING_BASE (0xA0000000)

/*-- Declarations (Statics and globals) --*/
static uint32_t ddrss_interrupt_id[6] = {0, 1, 2, 3, 4, 5};

/*------------- Functions ----------------*/
void prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    int sts = 0x0;
    ddrss_cfg_knobs_t ddrss_cfgs;
    bool ddrss_init_skip = false;
    bool use_fixed_atu_mapping = false;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();
    DDRSS_PLATFORM_TYPE ddrss_platform_override = DDRSS_PLATFORM_UNKNOWN;
    atu_map_entry_t atu_map_struct[NUM_DIE];
    uint32_t interrupt_idx;

    printf("DDRSS init start - die %d\n", die_num); //(1 ddrss contains 2 memory controllers).  PHY level is under MC level in heirarchy

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

    if (platform_id == PLATFORM_SVP_SIM)
    {
        // SVP does not support full DDR model, skip it.
        ddrss_init_skip = true;
    }
    else if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        printf("DDRSS init for FPGA platform\n");

        use_fixed_atu_mapping = false;

        ddrss_platform_override = DDRSS_PLATFORM_FPGA;
    }
    else
    {
        printf("DDRSS init is not supported yet on the platform\n");

        ddrss_init_skip = true;
    }

    if (ddrss_init_skip)
    {
        printf("DDRSS init is skipped\n");
        return;
    }

    if (use_fixed_atu_mapping)
    {
        // FPGA script will setup the fixed ATU mapping. Reuse it.
        atu_map_struct[SOC_D0].mscp_start_address = DDRSS_FPGA_ATU_MAPPING_BASE;

        // Currently no local ATU space mapped for remote die with FPGA script
        atu_map_struct[SOC_D1].mscp_start_address = 0;
    }
    else
    {
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
    }

    // Get the default DDRSS cfgs
    sts = ddrss_get_config(&ddrss_cfgs);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    // Update DDRSS cfgs to match actual platform cfg
    ddrss_cfgs.die_id = (DIE_INSTANCE)die_num;
    ddrss_cfgs.ddrss_base_die[SOC_D0] = atu_map_struct[SOC_D0].mscp_start_address;
    ddrss_cfgs.ddrss_base_die[SOC_D1] = atu_map_struct[SOC_D1].mscp_start_address;
    ddrss_cfgs.debug_level = DDRSS_DEBUG_LEVEL_INFO;
    ddrss_cfgs.numa_cfg = DDRSS_NUMA_CFG_UMA;
    ddrss_cfgs.ext_knobs.ddrss_mask = 0x3F;
    ddrss_cfgs.ext_knobs.interleave_mc_cnt = 0;
    if (ddrss_platform_override != DDRSS_PLATFORM_UNKNOWN)
    {
        ddrss_cfgs.platform_type = ddrss_platform_override;
    }

    // Run DDRSS init
    sts = ddrss_init(&ddrss_cfgs);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    if (!use_fixed_atu_mapping)
    {
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
    }

    printf("DDRSS init exit\n");
}
