//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh.cpp
 * mesh tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <cmn800_sequence.h> // for cmn800_sequence_params_t
#include <cmn_config.h>      // for CMN800_CONFIG_CONFIG
#include <idsw.h>            // for idsw_set_platform_sdv, DIE_ID, _PLAT_ID
#include <mesh.h>            // for mesh_init

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Setup and Teardown Functions
//
static int setup_svp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    return 0;
}

static int setup_fpga_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    return 0;
}

static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    return 0;
}

//
// Mocks
//
int __wrap_cmn800_sequence(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);
    check_expected(cmn800_sequence_param.cmn_config_enum);
    check_expected(cmn800_sequence_param.CMN_INIT);
    check_expected(cmn800_sequence_param.BOOT_2D_ENABLE);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE0);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE1);
    check_expected(cmn800_sequence_param.D2D_INIT);
    function_called();
    return 0;
}

//
// Tests
//
// Single Die Boot
TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (DIE_ID)0;

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (DIE_ID)1;
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (DIE_ID)0;
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (DIE_ID)1;
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die);
}
}