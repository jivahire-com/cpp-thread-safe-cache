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
#include <FpFwUtils.h>
#include <accelerator_ip.h>
#include <cper.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <ras_agent.h>
#include <silibs_status.h>
#include <tower.h>
#include <tower_isr.h>

/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

tower_knobs_t __real_config_get_tower_knobs(void);

/*-- Declarations (Statics and globals) --*/
static jmp_buf mock_jump_buf;
static bool should_return;

/*------------- Functions ----------------*/
void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}

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

silibs_status_t mock_ras_generic_handler(ras_error_record_t* record)
{
    assert_non_null(record);
    return SILIBS_SUCCESS;
}

TEST_FUNCTION(test_tower_fmu_corrected_handler, nullptr, nullptr)
{
    ras_agent_entity_t mock_ras_entity;
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, true);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, false);
    expect_function_call(__wrap_ras_print_record);
    will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_CORRECTED);
    will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
    tower_fmu_handler(TOWER_VAB_RPSS0, SOC_D0, 0);
}

TEST_FUNCTION(test_tower_fmu_fatal_handler, nullptr, nullptr)
{
    ras_agent_entity_t mock_ras_entity;

    will_return(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, true);
    will_return(__wrap_ras_agent_probe, mock_ras_generic_handler);
    will_return(__wrap_ras_agent_probe, false);
    expect_function_call(__wrap_ras_print_record);
    will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
    expect_function_call(__wrap_crash_dump_bug_check);
    if (!bugcheck_mock_return())
    {
        tower_fmu_handler(TOWER_VAB_RPSS0, SOC_D0, 0);
    }
}

TEST_FUNCTION(test_tower_error_injections, nullptr, nullptr)
{
    ras_agent_entity_t mock_ras_entity;
    ras_einj_info_t mock_einj_payload;
    acpi_einj_cmd_status_t ret;
    tower_error_inj_op_type_t* op_type =
        (tower_error_inj_op_type_t*)(&(mock_einj_payload.param_type.error_parameters[0]));

    mock_einj_payload.component_group = ACPI_ERROR_DOMAIN_NITOWER;
    mock_einj_payload.component_instance = 0;
    mock_einj_payload.component_type = TOWER_D2DSS0;
    op_type->op = TOWER_EINJ_TARGET_BY_NODE;
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return_always(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_inject_single_error, SILIBS_SUCCESS);
    will_return_always(__wrap_ras_arm_fmu_agent_trigger_by_type, SILIBS_SUCCESS);
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);

    op_type->op = TOWER_EINJ_TARGET_BY_INDEX;
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);

    op_type->op = TOWER_EINJ_TARGET_RAW;
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);
}

TEST_FUNCTION(test_tower_error_injections_failures, nullptr, nullptr)
{
    ras_agent_entity_t mock_ras_entity;
    ras_einj_info_t mock_einj_payload;
    acpi_einj_cmd_status_t ret;
    tower_error_inj_op_type_t* op_type =
        (tower_error_inj_op_type_t*)(&(mock_einj_payload.param_type.error_parameters[0]));

    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    /* Bad einj buffer */
    ret = tower_error_injection_cb(nullptr, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Bad component group */
    mock_einj_payload.component_group = ACPI_ERROR_DOMAIN_INVALID;
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Bad component instance */
    mock_einj_payload.component_group = ACPI_ERROR_DOMAIN_NITOWER;
    mock_einj_payload.component_instance = 0;
    mock_einj_payload.component_type = TOWER_D2DSS0;
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 1);
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    /* Bad error type */
    mock_einj_payload.component_group = ACPI_ERROR_DOMAIN_NITOWER;
    mock_einj_payload.component_instance = 0;
    mock_einj_payload.component_type = TOWER_D2DSS0;
    op_type->op = TOWER_EINJ_TARGET_RAW + 1;
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    will_return(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_SUCCESS);

    /* Silibs internal errors */
    mock_einj_payload.component_group = ACPI_ERROR_DOMAIN_NITOWER;
    mock_einj_payload.component_instance = 0;
    mock_einj_payload.component_type = TOWER_D2DSS0;
    op_type->op = TOWER_EINJ_TARGET_BY_NODE;
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    will_return(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_tower_fmu_inject_single_error, SILIBS_E_PARAM);
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return(__wrap_idsw_get_die_id, 0);
    will_return(__wrap_tower_get_ras_agent_entity, &mock_ras_entity);
    will_return(__wrap_ras_arm_fmu_agent_set_base, SILIBS_SUCCESS);
    will_return(__wrap_ras_arm_fmu_agent_trigger_by_type, SILIBS_E_PARAM);
    op_type->op = TOWER_EINJ_TARGET_RAW;
    ret = tower_error_injection_cb(&mock_einj_payload, nullptr);
    assert_int_equal(ret, ACPI_EINJ_INVALID_ACCESS);
}
}
