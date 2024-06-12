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
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <silibs_platform.h>
#include <stdbool.h>
#include <stdint.h>
#include <tower.h>
#include <tower_sequence.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

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
    tower_sequence_params.tower_configure_vab_sam = false;
    tower_sequence_params.tower_configure_vab_apu = false;
    tower_sequence_params.tower_vab_instances_enabled = 0;

    // RPSS towers
    tower_sequence_params.tower_configure_rpss_sam = false;
    tower_sequence_params.tower_configure_rpss_apu = false;
    tower_sequence_params.tower_rpss_instances_enabled = 0;

    // SDMSS tower
    tower_sequence_params.tower_configure_sdmss_sam = false;
    tower_sequence_params.tower_configure_sdmss_apu = false;
    tower_sequence_params.tower_sdmss_tower_resolved_addr = 0;

    // IOSS tower
    tower_sequence_params.tower_configure_ioss_sam = false;
    tower_sequence_params.tower_configure_ioss_apu = false;
    tower_sequence_params.tower_ioss_tower_resolved_addr = 0;

    tower_sequence_params.die_id = die_num;

    FPFW_RUNTIME_ASSERT(!tower_sequence_configure_towers(&tower_sequence_params));

    // Un-map towers
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_peripheral_tower_map));
    FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_fabric_tower_map));
}
