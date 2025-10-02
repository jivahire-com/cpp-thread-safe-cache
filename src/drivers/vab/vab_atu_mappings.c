//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file vab_atu_mappings
 * This section is used to manage atu mappings for all VAB and underlying IPs
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <silibs_status.h>
#include <stdint.h>
#include <vab_rpss_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*
 * Both die 0 and die 1 VAB addresses are statically defined here to allow
 * dynamically mapping a VAB on either die at runtime.
 */
static atu_map_entry_t atu_vabss_map[MAX_VAB_INSTANCES] = {
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

/*------------- Functions ----------------*/
silibs_status_t map_vab_instance(uint16_t vab_id)
{
    BUG_ASSERT_PARAM(vab_id < MAX_VAB_INSTANCES, vab_id, 0);
    return atu_map(ATU_ID_MSCP, &atu_vabss_map[vab_id]);
}

uintptr_t get_vab_resolved_base(uint16_t vab_id)
{

    BUG_ASSERT_PARAM(vab_id < MAX_VAB_INSTANCES, vab_id, 0);
    BUG_ASSERT_PARAM(atu_vabss_map[vab_id].mscp_start_address != 0, atu_vabss_map[vab_id].mscp_start_address, 0);

    return (uintptr_t)(atu_vabss_map[vab_id].mscp_start_address);
}

uintptr_t get_rpss_resolved_base(RPSS_INSTANCE rpss_id)
{
    uint32_t rpss_resolved_base = 0;

    BUG_ASSERT_PARAM(rpss_id < NUM_RPSS, rpss_id, 0);

    switch (rpss_id)
    {
    case RPSS0:
        rpss_resolved_base = atu_vabss_map[D0_VAB0_RPSS0].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS1:
        rpss_resolved_base = atu_vabss_map[D0_VAB1_RPSS1].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS2:
        rpss_resolved_base = atu_vabss_map[D0_VAB2_RPSS2].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS3:
        rpss_resolved_base = atu_vabss_map[D0_VAB3_RPSS3].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS4:
        rpss_resolved_base = atu_vabss_map[D1_VAB0_RPSS0].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS5:
        rpss_resolved_base = atu_vabss_map[D1_VAB1_RPSS1].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS6:
        rpss_resolved_base = atu_vabss_map[D1_VAB2_RPSS2].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    case RPSS7:
        rpss_resolved_base = atu_vabss_map[D1_VAB3_RPSS3].mscp_start_address + VAB_RPSS_TOP_RPSS_ADDRESS;
        break;
    default:
        break;
    }

    BUG_ASSERT_PARAM(rpss_resolved_base != 0, rpss_resolved_base, 0);
    return (uintptr_t)(rpss_resolved_base);
}
