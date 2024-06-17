//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This file carries releases VABs from reset, initializes VAB tower
 * interconnects and any other top-level VAB initialization that may be
 * needed. Some routines may be blocking by deign to ensure VABs are
 * correctly brought up.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <silibs_status.h>
#include <smmu_knobs.h>
#include <stdint.h>
#include <vab_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*------------- Functions ----------------*/

int vab_common_init(uint16_t vab_instances_to_init)
{
    int status = SILIBS_SUCCESS;

    atu_map_entry_t atu_vabss_map[MAX_VAB_INSTANCES] = {
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

    // Keep the default memory attributes
    smmu_gbpa_cfg_t smmu_gbpa_cfg = {0};
    smmu_gbpa_cfg.sh_cfg = 1;

    vab_init_t vab_init_cfg = {.vab_smmu_gbpa_cfg = &smmu_gbpa_cfg};

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init >> vab_id) & 0x1)
        {
            FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vabss_map[vab_id]));

            // TODO: What should be the default security state? This is used for configuring SMMU registers
            vab_init_cfg.security_state = SECURITY_STATE_NON_SECURE;
            // TODO: Use silibs knobs to get the system counter delay value
            vab_init_cfg.system_counter_delay = 0;
            vab_init_cfg.vab_resolved_base_addr = atu_vabss_map[vab_id].mscp_start_address;

            FPFW_RUNTIME_ASSERT(!vab_init(&vab_init_cfg));

            FPFW_RUNTIME_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_vabss_map[vab_id]));
        }
    }
    return status;
}
