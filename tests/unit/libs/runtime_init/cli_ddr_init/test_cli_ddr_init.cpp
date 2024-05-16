//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cli_ddr_init.cpp
 * Tests the init of cli ddr component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, assert...

extern "C" {
#include <fpfw_init.h> // for fpfw_init_component_t, fpfw_init_get_handle

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cli_ddr;

/*------------- Functions ----------------*/

//
// Mocks
//
void __wrap_cli_ddr_init()
{
    function_called();
}

//
// Tests
//
TEST_FUNCTION(test_cli_ddr, nullptr, nullptr)
{
    expect_function_call(__wrap_cli_ddr_init);
    //! Call API under test
    fpfw_init_result_t result = _fpfw_component_cli_ddr.init_fn();

    //! Verify expected result
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
