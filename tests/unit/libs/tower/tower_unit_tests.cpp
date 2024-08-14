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
#include <hsp_firmware_headers.h>
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
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);
    will_return_always(__wrap_system_info_is_hsp_present, false);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_cdedss_tower_init_die0_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    kng_hsp_mailbox_msg send_msg = {.header = {
        .cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ,
        .flags = HSP_MAILBOX_FLAGS_ACCL_ISOLATION_DISABLED
    }};
    FPFW_MBX_PAYLOAD send_payload = {.payloadBuffer = &send_msg, .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value_count(__wrap_FpFwMailboxSend, pMessage->payloadSize, send_payload.payloadSize, 1);

    expect_value_count(__wrap_FpFwMailboxSend, cmd, (send_msg.header.cmd & 0xFFFF), 1);
    expect_value_count(__wrap_FpFwMailboxSend, flags, send_msg.header.flags, 1);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    will_return(__wrap_FpFwMailboxReceive, FPFW_MBX_SUCCESS);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}

TEST_FUNCTION(test_tower_sequence_cdedss_tower_init_die1_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    FPFW_MBX_PRIMITIVE_CTX mbx_ctx;
    kng_hsp_mailbox_msg send_msg = {.header = {
        .cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ,
        .flags = HSP_MAILBOX_FLAGS_ACCL_ISOLATION_DISABLED
    }};
    FPFW_MBX_PAYLOAD send_payload = {.payloadBuffer = &send_msg, .payloadSize = (HSP_MBX_FIFO_DEPTH * sizeof(uint32_t))};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_any_always(__wrap_FpFwMailboxSend, pMbxCtx);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage);
    expect_any_always(__wrap_FpFwMailboxSend, pMessage->payloadBuffer);
    expect_value_count(__wrap_FpFwMailboxSend, pMessage->payloadSize, send_payload.payloadSize, 2);

    expect_value_count(__wrap_FpFwMailboxSend, cmd, (send_msg.header.cmd & 0xFFFF), 2);
    expect_value_count(__wrap_FpFwMailboxSend, flags, send_msg.header.flags, 2);

    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_UNINITIALIZED);
    will_return(__wrap_FpFwMailboxSend, FPFW_MBX_SUCCESS);
    will_return(__wrap_FpFwMailboxReceive, FPFW_MBX_UNINITIALIZED);
    will_return(__wrap_FpFwMailboxReceive, FPFW_MBX_SUCCESS);

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
    tower_init(test_die, (FPFW_MBX_PRIMITIVE_CTX *) &mbx_ctx);
}
}
