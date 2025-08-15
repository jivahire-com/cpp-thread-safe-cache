//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tower_unit_tests.cpp
 * tower tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...
#include <fpfw_cfg_mgr.h>

extern "C" {
#include <accelerator_ip.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <silibs_status.h>
#include <tower.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

tower_knobs_t __real_config_get_tower_knobs(void);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Tests
//
TEST_FUNCTION(test_tower_sequence_init_die0_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_silicon, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_emu_1d_8c, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D_8C);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_emu_1d, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_emu_2d_8c, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D_8C);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_emu_2d, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_emu_2d, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_emu_2d_8c, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D_8C);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_fpga, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_svp_sdm_isolation_enable, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, true);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) |
                  (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die0_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_init_die1_invalid_platform, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);
    will_return_always(__wrap_system_info_is_hsp_present, false);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, true);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, true);
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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_cdedss_tower_init_die0_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ;
    msg.header.flags = HSP_MAILBOX_FLAGS_ACCL_ISOLATION_DISABLED;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_ddrss_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_ddrss_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_pmus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_pmus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_rpss_os_first_ras_err_handling = 0;
    msg.tower_config_req.tower_unlock_flags.tower_vab_os_first_ras_err_handling = 0;

    size_t output_recv_bytes = 0;

    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);
    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_CDED);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, icc_ctx, icc_ctx);
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &msg, sizeof(msg));
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes, &output_recv_bytes, sizeof(output_recv_bytes));
    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, sizeof(msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_cdedss_tower_init_die0_svp_isolation_enable, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    size_t output_recv_bytes = 0;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ;
    msg.header.flags = 0;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_ddrss_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_ddrss_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_pmus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_pmus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_rpss_os_first_ras_err_handling = 0;
    msg.tower_config_req.tower_unlock_flags.tower_vab_os_first_ras_err_handling = 0;

    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, true);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);
    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_CDED);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, icc_ctx, icc_ctx);
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &msg, sizeof(msg));
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes, &output_recv_bytes, sizeof(output_recv_bytes));
    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, sizeof(msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

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
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}

TEST_FUNCTION(test_tower_sequence_cdedss_tower_init_die1_svp, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    uint32_t dummy_icc_ctx = 0;
    fpfw_icc_base_ctx_t* icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;
    size_t output_recv_bytes = 0;

    tower_knobs_t tower_config_knobs = __real_config_get_tower_knobs();
    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ;
    msg.header.flags = HSP_MAILBOX_FLAGS_ACCL_ISOLATION_DISABLED;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_ddrss_apus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_ddrss_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_pmus_as_nonsecure =
        tower_config_knobs.tower_mode_cfg.tower_configure_all_pmus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_rpss_os_first_ras_err_handling = 0;
    msg.tower_config_req.tower_unlock_flags.tower_vab_os_first_ras_err_handling = 0;

    will_return(__wrap_config_get_tower_knobs, &tower_config_knobs);
    expect_function_call(__wrap_config_get_tower_knobs);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_system_info_is_hsp_present, true);

    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_SDM);
    expect_value(__wrap_accel_is_isolation_enabled, acc_id, ACCEL_ID_CDED);

    expect_value(__wrap_fpfw_icc_base_send_recv_sync, icc_ctx, icc_ctx);
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, payload_buffer, &msg, sizeof(msg));
    expect_memory(__wrap_fpfw_icc_base_send_recv_sync, output_recv_bytes, &output_recv_bytes, sizeof(output_recv_bytes));
    expect_value(__wrap_fpfw_icc_base_send_recv_sync, buffer_size, sizeof(msg));
    will_return(__wrap_fpfw_icc_base_send_recv_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_fabric_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_fabric_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_periph_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_periph_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_d2dss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg0_tower_resolved_addr, 0xffffffff);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_d2dss_cfg1_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_vab_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_vab_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) |
                  (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_rpss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers,
                 tower_sequence_param->tower_rpss_instances_enabled,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3)));

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_sdmss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_isolation_enabled, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_sdmss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_sam, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_apu, true);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_configure_ioss_fmu, false);
    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->tower_ioss_tower_resolved_addr, 0xffffffff);

    expect_value(__wrap_tower_sequence_configure_towers, tower_sequence_param->die_id, test_die);

    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_function_call(__wrap_tower_sequence_configure_towers);

    // Call API under test
    tower_init(test_die, icc_ctx);
}
}
