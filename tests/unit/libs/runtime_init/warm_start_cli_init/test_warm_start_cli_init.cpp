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
#include <startup_shutdown_init.h>
extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_ws_cli_init;
/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_warm_start_cli_init(psos_device_t p_device)
{
    FPFW_UNUSED(p_device);
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_warm_start_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_warm_start_cli_init);

    // Call API under test
    _fpfw_component_ws_cli_init.init_fn();
}
}
