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
#include <idsw_kng.h>
#include <kng_soc_constants.h>
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
TEST_FUNCTION(test_tower_sequence_init_die0_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) |
                  (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die1_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) |
                  (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die0_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die1_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_rpss_instances_enabled, (1 << D1_VAB1_RPSS1));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die0_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0x0);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0x0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) |
                  (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die1_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0x0);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0x0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_rpss_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die0_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_vab_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_rpss_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}

TEST_FUNCTION(test_tower_sequence_init_die1_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_vab_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_rpss_instances_enabled, 0);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die);
}
}
