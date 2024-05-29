//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_knobs_test.cpp
 * Power service knob tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include <CMockaWrapper.h>     // for expect_value, check_expected_ptr, Cmo...
#include <power_runconfig.h>   // for power_service_config_t
#include <power_runconfig_i.h> // for power_runconfig_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//

} // extern "C"

//
// Tests
//
POWER_TEST(read_knobs, NULL, NULL)
{
    power_knobs_t test_knobs = {};

    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491012/
    // placeholder - implement test with actual knob implementation
    expect_any_always(__wrap_FpFwAssert, expression);

    power_knobs_read(&test_knobs);
}
