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
#include <idsw_kng.h>        // for KNG_DIE_ID
#include <ppu_v1.h>          // for PPU_V1_OPMODE_01

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
int __wrap_clocks_sequence_css_pre_mesh_init(const clocks_sequence_css_pre_mesh_init_t* clocks_sequence_params)
{
    check_expected_ptr(clocks_sequence_params);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_css_pre_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    clocks_sequence_css_pre_mesh_init_t scp_clocks_pre_mesh_param = {};
    scp_clocks_pre_mesh_param.system_ppu_opmode = PPU_V1_OPMODE_01;
    scp_clocks_pre_mesh_param.skip_css_pcr_init = false;
    scp_clocks_pre_mesh_param.skip_msxp_pcr_init = false;
    expect_memory(__wrap_clocks_sequence_css_pre_mesh_init,
                  clocks_sequence_params,
                  &scp_clocks_pre_mesh_param,
                  sizeof(scp_clocks_pre_mesh_param));

    // Call API under test
    css_pre_mesh_init(test_die);
}
}