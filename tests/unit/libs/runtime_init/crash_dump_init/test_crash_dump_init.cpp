//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_init.cpp
 * Crash dump init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <fpfw_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cd_init;
extern fpfw_init_component_t _fpfw_component_cd_init_pomesh;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_crash_dump_init()
{
    function_called();
}

void __wrap_crash_dump_init_post_mesh()
{
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_crash_dump_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_crash_dump_init);

    // Check dependencies
    assert_string_equal("gpio_lib", _fpfw_component_cd_init.children[0]);

    // Call API under test
    _fpfw_component_cd_init.init_fn();
}

TEST_FUNCTION(test_crash_dump_init_post_mesh, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_crash_dump_init_post_mesh);

    // Check dependencies
    assert_string_equal("cd_init", _fpfw_component_cd_init_pomesh.children[0]);
    assert_string_equal("mesh", _fpfw_component_cd_init_pomesh.children[1]);

    // Call API under test
    _fpfw_component_cd_init_pomesh.init_fn();
}
}
