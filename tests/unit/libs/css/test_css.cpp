//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_css.cpp
 * CSS tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {

#include <atu_lib.h>         // for ATU_ID_MSCP, atu_id_t, atu_map_entry_t
#include <clocks_sequence.h> // for clocks_sequence_css_pre_mesh_init_t
#include <css.h>             // for css_pre_mesh_init
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h> // for KNG_DIE_ID
#include <ppu_v1.h>   // for PPU_V1_OPMODE_01

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//

intclk_cfg_t __wrap_config_get_intclk_knobs()
{
    intclk_cfg_t intclk = {

        .freq_mhz = 2000,
        .ref_div = 1,
        .post_div01 = 3,
        .post_div02 = 0,
    };
    return intclk;
}

int __wrap_clocks_sequence_css_pre_mesh_init(const clocks_sequence_css_pre_mesh_init_t* clocks_sequence_params)
{
    check_expected(clocks_sequence_params->system_ppu_opmode);
    check_expected(clocks_sequence_params->skip_css_pcr_init);
    check_expected(clocks_sequence_params->skip_msxp_pcr_init);
    check_expected(clocks_sequence_params->system_smmu_gpt_enabled);
    check_expected(clocks_sequence_params->system_smmu_l0gptsz);
    check_expected(clocks_sequence_params->intclk_cfg->freq_mhz);
    check_expected(clocks_sequence_params->intclk_cfg->ref_div);
    check_expected(clocks_sequence_params->intclk_cfg->post_div01);
    check_expected(clocks_sequence_params->intclk_cfg->post_div02);

    function_called();
    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_css_pre_mesh_init_die0_intclk_2000mhz, nullptr, nullptr)
{

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_ppu_opmode, PPU_V1_OPMODE_01);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->skip_css_pcr_init, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->skip_msxp_pcr_init, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_smmu_gpt_enabled, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_smmu_l0gptsz, 0);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->freq_mhz, 2000);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->ref_div, 1);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->post_div01, 3);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->post_div02, 0);

    expect_function_call(__wrap_clocks_sequence_css_pre_mesh_init);
    // Call API under test
    css_pre_mesh_init(test_die);
}

TEST_FUNCTION(test_css_pre_mesh_init_die1_intclk_2000mhz, nullptr, nullptr)
{

    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;

    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_ppu_opmode, PPU_V1_OPMODE_01);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->skip_css_pcr_init, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->skip_msxp_pcr_init, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_smmu_gpt_enabled, false);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->system_smmu_l0gptsz, 0);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->freq_mhz, 2000);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->ref_div, 1);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->post_div01, 3);
    expect_value(__wrap_clocks_sequence_css_pre_mesh_init, clocks_sequence_params->intclk_cfg->post_div02, 0);

    expect_function_call(__wrap_clocks_sequence_css_pre_mesh_init);
    // Call API under test
    css_pre_mesh_init(test_die);
}
}