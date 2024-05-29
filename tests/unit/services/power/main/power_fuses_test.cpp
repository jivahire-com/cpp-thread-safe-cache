//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuses_test.cpp
 * Power service fuse tests
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
POWER_TEST(read_fuses, NULL, NULL)
{
    power_fuse_data_t test_fuses = {};

    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491015/
    // placeholder - implement test with actual fuse implementation
    expect_any_always(__wrap_FpFwAssert, expression);

    power_fuses_read(&test_fuses);
}
