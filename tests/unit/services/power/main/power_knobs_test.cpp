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
static bool test_enable_survivability_mode = false;
/*------------- Functions ----------------*/
//
// Mocks
//

bool __wrap_config_get_power_enable_survivability_mode()
{
    return test_enable_survivability_mode;
}

} // extern "C"

//
// Tests
//
POWER_TEST(read_knobs, NULL, NULL)
{
    power_knobs_t test_knobs = {};

    expect_any_always(__wrap_FpFwAssert, expression);

    power_knobs_read(&test_knobs);
}

POWER_TEST(read_knobs_survivability_mode, NULL, NULL)
{
    power_knobs_t test_knobs = {};
    test_enable_survivability_mode = true;

    expect_any_always(__wrap_FpFwAssert, expression);
    power_knobs_read(&test_knobs);
    assert_int_equal(test_knobs.enable_survivability_mode, true);
    assert_int_equal(test_knobs.loops_disable, (test_knobs.loops_disable | power_loops_disable_t_CTRL_LOOP));
}
