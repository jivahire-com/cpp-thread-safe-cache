//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_css_init.cpp
 * CSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <fpfw_init.h>
#include <idhw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_et_init;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_scp_etc_initialize()
{
    function_called();
}
//
// Tests
//
TEST_FUNCTION(test_et_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_scp_etc_initialize);

    // Call API under test
    _fpfw_component_et_init.init_fn();
}
}