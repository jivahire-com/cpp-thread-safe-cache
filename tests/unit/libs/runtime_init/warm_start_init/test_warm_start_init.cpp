//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_warm_start_init.cpp
 * warm start init tests
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

extern fpfw_init_component_t _fpfw_component_ws_init;
/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_warm_start_init()
{
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_warm_start_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_warm_start_init);

    // Call API under test
    _fpfw_component_ws_init.init_fn();
}
}
