//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_mocks.c
 * Mock functions for tower sequence
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // IWYU pragma: keep
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <idsw.h>
#include <silibs_status.h>
#include <stddef.h>
#include <tower_sequence.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }
    // Keep mscp base non-zero to allow checking base address in UTs
    atu_map_entry->mscp_start_address = 0xffffffff;

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    if (atu_id >= ATU_ID_MAX || atu_map_entry == NULL)
    {
        return SILIBS_E_PARAM;
    }

    return mock_type(int);
}

PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(PLAT_ID);
}

int __wrap_tower_sequence_configure_towers(tower_sequence_soc_init_params_t* tower_sequence_param)
{
    check_expected(tower_sequence_param->tower_configure_fabric_apu);
    check_expected(tower_sequence_param->tower_fabric_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_periph_apu);
    check_expected(tower_sequence_param->tower_periph_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_vab_sam);
    check_expected(tower_sequence_param->tower_configure_vab_apu);
    check_expected(tower_sequence_param->tower_configure_vab_fmu);
    check_expected(tower_sequence_param->tower_vab_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_rpss_sam);
    check_expected(tower_sequence_param->tower_configure_rpss_apu);
    check_expected(tower_sequence_param->tower_configure_rpss_fmu);
    check_expected(tower_sequence_param->tower_rpss_instances_enabled);

    check_expected(tower_sequence_param->tower_configure_sdmss_sam);
    check_expected(tower_sequence_param->tower_configure_sdmss_apu);
    check_expected(tower_sequence_param->tower_sdmss_tower_resolved_addr);

    check_expected(tower_sequence_param->tower_configure_ioss_sam);
    check_expected(tower_sequence_param->tower_configure_ioss_apu);
    check_expected(tower_sequence_param->tower_ioss_tower_resolved_addr);

    check_expected(tower_sequence_param->die_id);
    function_called();
    return 0;
}
