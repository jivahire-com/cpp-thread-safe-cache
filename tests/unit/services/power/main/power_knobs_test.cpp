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

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <idsw_kng.h>
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

int16_t __wrap_config_get_power_vcpu0_offset()
{
    return mock_type(int16_t);
}

int16_t __wrap_config_get_power_vcpu1_offset()
{
    return mock_type(int16_t);
}

} // extern "C"
//
// Tests
//
POWER_TEST(read_knobs, NULL, NULL)
{
    power_knobs_t test_knobs = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    will_return_always(__wrap_config_get_power_vcpu0_offset, 0);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    power_knobs_read(&test_knobs);
}

POWER_TEST(read_knobs_survivability_mode, NULL, NULL)
{
    power_knobs_t test_knobs = {};
    test_enable_survivability_mode = true;

    expect_any_always(__wrap_FpFwAssert, expression);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_config_get_power_vcpu0_offset, 0);
    power_knobs_read(&test_knobs);
    assert_int_equal(test_knobs.enable_survivability_mode, true);
    assert_int_equal(test_knobs.loops_disable, (test_knobs.loops_disable | power_loops_disable_t_CTRL_LOOP));
}

POWER_TEST(read_knobs_vcpu_offset_die0, NULL, NULL)
{
    power_knobs_t test_knobs = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_config_get_power_vcpu0_offset, 100);
    power_knobs_read(&test_knobs);
    assert_int_equal(test_knobs.vcpu_offset_mv, 100);
}

POWER_TEST(read_knobs_vcpu_offset_die1, NULL, NULL)
{
    power_knobs_t test_knobs = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_config_get_power_vcpu1_offset, 200);
    power_knobs_read(&test_knobs);
    assert_int_equal(test_knobs.vcpu_offset_mv, 200);
}
