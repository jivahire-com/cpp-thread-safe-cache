//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_css_init.cpp
 * CSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <fpfw_init.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_css_prme;
extern fpfw_init_component_t _fpfw_component_css_pome;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_css_pre_mesh_init(uint8_t die_num)
{
    check_expected(die_num);
}

void __wrap_css_post_mesh_init()
{
    function_called();
}

void __wrap_css_configure_system_tower(uint8_t die_num)
{
    check_expected(die_num);
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

//
// Tests
//
TEST_FUNCTION(test_css_pre_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idhw_get_die_id, test_die);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_css_pre_mesh_init, die_num, test_die);
    expect_value(__wrap_css_configure_system_tower, die_num, test_die);

    // Call API under test
    _fpfw_component_css_prme.init_fn();
}

TEST_FUNCTION(test_css_post_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_css_post_mesh_init);

    // Call API under test
    _fpfw_component_css_pome.init_fn();
}
}
