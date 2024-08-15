//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_icc_cli_init.cpp
 * Test stub for ICC CLI initialization
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_init.h>
#include <icc_cli.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_icc_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_icc_cli_init(icc_cli_init_params_t *params)
{
    FPFW_UNUSED(params);
    function_called();
}
//
// Tests
//
TEST_FUNCTION(test_icc_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_icc_cli_init);

    // Call API under test
    _fpfw_component_icc_cli.init_fn();
}
}