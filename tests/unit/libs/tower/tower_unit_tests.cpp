//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_unit_tests.cpp
 * tower tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include <idsw.h> // for idsw_set_platform_sdv, DIE_ID, _PLAT_ID
#include <silibs_status.h>
#include <tower.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Tests
//
// Single Die Boot
TEST_FUNCTION(test_tower_sequence_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (DIE_ID)0;

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_not_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_resolved_addr, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_not_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_resolved_addr, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_vab_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_rpss_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_resolved_addr, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_resolved_addr, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}
}