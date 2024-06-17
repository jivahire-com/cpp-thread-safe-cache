//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tower.c
 *  This modules configures various SOC towers as done by the SCP FW
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <atu_lib.h>
#include <idsw.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tower.h>
#include <tower_sequence.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// Skip RPSS on SVP to avoid simulation slowdown
#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP ((1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SVP ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_FPGA \
    ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_FPGA \
    ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SILICON                                              \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) | \
     (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SILICON                                              \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) | \
     (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

static uint16_t tower_vab_instances_to_be_enabled(uint8_t die_num)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);
    uint16_t vab_instances_to_init = 0;
    PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        // Skip RPSS on SVP to avoid simulation slowdown
        vab_instances_to_init =
            (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SVP;
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        vab_instances_to_init = (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_FPGA
                                               : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_FPGA;
        break;

    case PLATFORM_RVP_EVT_SILICON:
        vab_instances_to_init = (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SILICON
                                               : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SILICON;
        break;

    default:
        printf("Skip VAB init on unknown platform id: %d\n", plat);
        break;
    }

    return vab_instances_to_init;
}

// Skip RPSS on SVP to avoid simulation slowdown
#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SVP (0)
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SVP (0)

#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_FPGA ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2))
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_FPGA ((1 << D1_VAB1_RPSS1))

#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SILICON \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3))
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SILICON \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3))

static uint16_t tower_rpss_instances_to_be_enabled(uint8_t die_num)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);
    uint16_t rpss_instances_to_init = 0;
    PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        printf("Skip RPSS init on SVP\n");
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SVP
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SVP;
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_FPGA
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_FPGA;
        break;

    case PLATFORM_RVP_EVT_SILICON:
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SILICON
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SILICON;
        break;

    default:
        printf("Skip RPSS init on unknown platform id: %d\n", plat);
        break;
    }

    return rpss_instances_to_init;
}

// Map the VAB-SS instead of just the tower space because this sequence also configures SMMU straps and
// SMMU reset de-assertion to allow configuring the VAB FMU
static atu_map_entry_t atu_vab_maps[MAX_VAB_INSTANCES] = {
    ATU_MAPPING_D0_VAB0_RPSS0(),
    ATU_MAPPING_D0_VAB1_RPSS1(),
    ATU_MAPPING_D0_VAB2_RPSS2(),
    ATU_MAPPING_D0_VAB3_RPSS3(),
    ATU_MAPPING_D1_VAB0_RPSS0(),
    ATU_MAPPING_D1_VAB1_RPSS1(),
    ATU_MAPPING_D1_VAB2_RPSS2(),
    ATU_MAPPING_D1_VAB3_RPSS3(),
    ATU_MAPPING_D0_VAB4_SDMSS(),
    ATU_MAPPING_D1_VAB4_SDMSS(),
    ATU_MAPPING_D0_VAB5_CDEDSS_IOSS(),
    ATU_MAPPING_D1_VAB5_CDEDSS_IOSS(),
};

static atu_map_entry_t atu_rpss_tower_maps[NUM_RPSS] = {
    ATU_MAPPING_D0_RPSS0_TOWER(),
    ATU_MAPPING_D0_RPSS1_TOWER(),
    ATU_MAPPING_D0_RPSS2_TOWER(),
    ATU_MAPPING_D0_RPSS3_TOWER(),
    ATU_MAPPING_D1_RPSS0_TOWER(),
    ATU_MAPPING_D1_RPSS1_TOWER(),
    ATU_MAPPING_D1_RPSS2_TOWER(),
    ATU_MAPPING_D1_RPSS3_TOWER(),
};

void tower_init(uint8_t die_num)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    tower_sequence_soc_init_params_t tower_sequence_params = {0};

    // Fabric Tower
    tower_sequence_params.tower_configure_fabric_apu = true;
    atu_map_entry_t atu_fabric_tower_map = ATU_MAPPING_FABRIC_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_fabric_tower_map));
    tower_sequence_params.tower_fabric_tower_resolved_addr = atu_fabric_tower_map.mscp_start_address;

    // Peripheral Tower
    tower_sequence_params.tower_configure_periph_apu = true;
    atu_map_entry_t atu_peripheral_tower_map = ATU_MAPPING_PERIPHERAL_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_peripheral_tower_map));
    tower_sequence_params.tower_periph_tower_resolved_addr = atu_peripheral_tower_map.mscp_start_address;

    // TODO: D2DSS towers

    // VAB Towers
    uint16_t vab_instances_to_init = tower_vab_instances_to_be_enabled(die_num);
    tower_sequence_params.tower_vab_instances_enabled = vab_instances_to_init;
    printf("Mask of VAB instances to be enabled: 0x%x\n", vab_instances_to_init);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init & (0x1 << vab_id)) != 0)
        {
            printf("Configure VAB ATU map for VAB: 0x%x\n", vab_id);
            FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vab_maps[vab_id]));
            tower_sequence_params.tower_vab_resolved_addr[vab_id] = atu_vab_maps[vab_id].mscp_start_address;
            tower_sequence_params.vab_smmu_l0gpt_size[vab_id] = 0;
            // TODO: When should GPT be enabled for VAB SMMU?
            tower_sequence_params.vab_smmu_gpt_enabled[vab_id] = false;
        }
    }
    if (vab_instances_to_init != 0)
    {
        tower_sequence_params.tower_configure_vab_sam = true;
        tower_sequence_params.tower_configure_vab_apu = true;
        tower_sequence_params.tower_configure_vab_fmu = true;
        // TODO: WI 1869184 FMU is not supported in SVP currently
        if (idsw_get_platform_sdv() == PLATFORM_SVP_SIM)
        {
            tower_sequence_params.tower_configure_vab_fmu = false;
        }
    }

    // RPSS towers
    uint8_t rpss_instances_to_init = tower_rpss_instances_to_be_enabled(die_num);
    tower_sequence_params.tower_rpss_instances_enabled = rpss_instances_to_init;
    printf("Mask of RPSS instances to be enabled: 0x%x\n", rpss_instances_to_init);

    for (uint8_t rpss_id = 0; rpss_id < NUM_RPSS; rpss_id++)
    {
        if ((rpss_instances_to_init & (0x1 << rpss_id)) != 0)
        {
            printf("Configure RPSS Tower ATU map for RPSS: 0x%x\n", rpss_id);
            FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_rpss_tower_maps[rpss_id]));
            tower_sequence_params.tower_rpss_tower_resolved_addr[rpss_id] = atu_rpss_tower_maps[rpss_id].mscp_start_address;
        }
    }
    if (rpss_instances_to_init != 0)
    {
        tower_sequence_params.tower_configure_rpss_sam = true;
        tower_sequence_params.tower_configure_rpss_apu = true;
        tower_sequence_params.tower_configure_rpss_fmu = true;
        // TODO: WI 1869184 FMU is not supported in SVP currently
        if (idsw_get_platform_sdv() == PLATFORM_SVP_SIM)
        {
            tower_sequence_params.tower_configure_rpss_fmu = false;
        }
    }

    // SDMSS tower
    printf("Configure SDMSS Tower ATU map\n");
    atu_map_entry_t sdmss_tower_map = ATU_MAPPING_SDMSS_TOWER((die_num == 0 ? D0_SDMSS : D1_SDMSS));
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &sdmss_tower_map));
    tower_sequence_params.tower_sdmss_tower_resolved_addr = sdmss_tower_map.mscp_start_address;
    tower_sequence_params.tower_configure_sdmss_sam = true;
    tower_sequence_params.tower_configure_sdmss_apu = true;

    // IOSS tower
    printf("Configure IOSS tower ATU map\n");
    atu_map_entry_t ioss_tower_map = ATU_MAPPING_IOSS_TOWER((die_num == 0 ? D0_IOSS : D1_IOSS));
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &ioss_tower_map));
    tower_sequence_params.tower_ioss_tower_resolved_addr = ioss_tower_map.mscp_start_address;
    tower_sequence_params.tower_configure_ioss_sam = true;
    tower_sequence_params.tower_configure_ioss_apu = true;

    tower_sequence_params.die_id = die_num;

    printf("Configure all towers\n");
    FPFW_RUNTIME_ASSERT(!tower_sequence_configure_towers(&tower_sequence_params));
    printf("Towers configured\n");

    // Un-map towers
    printf("Unmap towers\n");
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_peripheral_tower_map));
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_fabric_tower_map));

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init & (0x1 << vab_id)) != 0)
        {
            FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_vab_maps[vab_id]));
        }
    }
    for (uint8_t rpss_id = 0; rpss_id < NUM_RPSS; rpss_id++)
    {
        if ((rpss_instances_to_init & (0x1 << rpss_id)) != 0)
        {
            FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_rpss_tower_maps[rpss_id]));
        }
    }
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &sdmss_tower_map));
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &ioss_tower_map));
}
