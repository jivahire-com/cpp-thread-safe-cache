//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mts_init.cpp
 * ATU init tests
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <fpfw_init.h>
#include <pwr_tlm_core_exchange.h> // for ACCEL_ID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pwr_tlm_core_ex;

/*------------- Functions ----------------*/

//
// Tests
//
TEST_FUNCTION(test_pwr_tlm_core_exch_init, nullptr, nullptr)
{
    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_tlm_core_ex.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
