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
#include "corebits.h"   // for corebits_t
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h>     // for expect_any_always, CmockaWrapperTest
#include <assert.h>            // for assert
#include <cstddef>             // for NULL, size_t
#include <fpfw_status.h>       // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <mock_bug_check.h>    // for __wrap_crash_dump_bug_check
#include <power_i.h>           // for power_fuses_get_dts_coeff, power_fuse...
#include <power_runconfig.h>   // for power_fuse_data_t, dts_coeff_t, power...
#include <power_runconfig_i.h> // for power_fuses_read, power_fuses_get_cur...
#include <stdint.h>            // for int8_t, uint64_t, uint8_t, uintptr_t
#include <string.h>            // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

#define CONFIGURED_POWER_PER_RAIL (1)
#define R_LL                      (400)
// see setup below -- CPU voltage adjusted by loadline (.950) multiplied by the maximum electrical current limit = 475W
#define CONFIGURED_MEL                 (514)
#define CONFIGURED_THERMAL_WATTS_LIMIT (300)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static power_runconfig_t test_power_runconfig = {};
static power_service_config_t test_power_service_config = {
    .is_primary_die = true,
};

/*------------- Functions ----------------*/
//
// Mocks
//

} // extern "C"

static int setup(void** state)
{
    UNUSED(state);
    /* some tests will modify the module context */

    test_power_runconfig.knobs.r_loadline_vcpu0_uohm =
        R_LL; // just need a known value here; (uOhm) early indications were that loadline would be .40mOhm
    test_power_runconfig.knobs.r_loadline_vcpu1_uohm =
        R_LL; // just need a known value here; (uOhm) early indications were that loadline would be .40mOhm
    test_power_runconfig.knobs.vsys_r_loadline_uohm =
        R_LL; // just need a known value here; (uOhm) early indications were that loadline would be .40mOhm
    test_power_runconfig.knobs.soc_maximum_electrical_current_limit_vcpu0 = 500; // again, just need a value- this is in amps
    test_power_runconfig.knobs.soc_maximum_electrical_current_limit_vcpu1 = 500; // again, just need a value- this is in amps

    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = CONFIGURED_THERMAL_WATTS_LIMIT;
    test_power_runconfig.p_sconfig = &test_power_service_config;
    return 0;
}

static int teardown(void** state)
{
    UNUSED(state);

    return 0;
}

#define TEST_POWER_CAP 10
POWER_TEST(power_cap_get_vrcpu_cap__new_cap__false, setup, teardown)
{
    power_latest_calcs_t test_calcs = {0};

    bool newcap = false;
    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    // get current vcpu cap
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    power_cap_get_vrcpu_cap(&newcap, &test_calcs, &test_calcs);

    // no cap, so newcap should be false
    assert_false(newcap);
}

POWER_TEST(power_cap_get_vrcpu_cap__new_cap__true, setup, teardown)
{
    power_latest_calcs_t test_calcs = {0};

    bool newcap = false;
    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    // now set a cap
    assert_int_equal(power_cap_update(NULL, TEST_POWER_CAP, false), MP_POWER_CAP_PENDING);
    // get current vcpu cap
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    power_cap_get_vrcpu_cap(&newcap, &test_calcs, &test_calcs);

    // no cap, so newcap should be true
    assert_true(newcap);
}

POWER_TEST(power_cap_get_vrcpu_cap__max_electrical_limit, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};

    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = CONFIGURED_THERMAL_WATTS_LIMIT * 2;
    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    float cap = power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    assert_int_equal((uint32_t)cap, CONFIGURED_MEL);
}

POWER_TEST(power_cap_get_vrcpu_cap__soc_maximum_thermal_watts_limit, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};
    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = CONFIGURED_THERMAL_WATTS_LIMIT;

    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    // configured thermal limit in watts < MEL and power cap, so it will be limiting factor
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    float cap = power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    assert_int_equal((uint32_t)cap,
                     CONFIGURED_THERMAL_WATTS_LIMIT - (test_calcs.soc_power * 2) + (test_calcs.vcpu_power * 2));
}

POWER_TEST(power_cap_get_vrcpu_cap__soc_power_cap_watts, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};
    // increase the derived cap to ensure it is not the limiting factor
    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = CONFIGURED_THERMAL_WATTS_LIMIT * 2;

    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    // set a cap
    assert_int_equal(power_cap_update(NULL, CONFIGURED_THERMAL_WATTS_LIMIT, false), MP_POWER_CAP_PENDING);

    // configured thermal limit in watts < MEL and power cap, so it will be limiting factor
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    float cap = power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    assert_int_equal((uint32_t)cap,
                     CONFIGURED_THERMAL_WATTS_LIMIT - (test_calcs.soc_power * 2) + (test_calcs.vcpu_power * 2));
}

POWER_TEST(power_cap_get_vrcpu_cap__zero, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};
    // increase the derived cap to ensure it is not the limiting factor
    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = CONFIGURED_THERMAL_WATTS_LIMIT * 2;

    // have to call this once to preload internal soc_power_cap_watts variable
    power_cap_init();

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    // set a cap so low that adjusted cap will be 0
    assert_int_equal(power_cap_update(NULL, TEST_POWER_CAP, false), MP_POWER_CAP_PENDING);

    // configured thermal limit in watts < MEL and power cap, so it will be limiting factor
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    float cap = power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    assert_int_equal((uint32_t)cap, 0);
}

void test_power_cap_callback(int result, uint16_t current, uint16_t previous)
{
    check_expected(result);
    check_expected(current);
    check_expected(previous);
}

void test_power_cap_callback2(int result, uint16_t current, uint16_t previous)
{
    check_expected(result);
    check_expected(current);
    check_expected(previous);
}

#define TEST_NEW_POWER_CAP 200
POWER_TEST(power_cap_update__previous_not_complete, setup, teardown)
{
    // have to call init to ensure internal state
    power_cap_init();

    // using different caps to confirm changes
    int response = power_cap_update(test_power_cap_callback, CONFIGURED_THERMAL_WATTS_LIMIT, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);
    // expected values for failure callback
    expect_value_count(test_power_cap_callback, result, MP_POWER_CAP_FAIL_PREVIOUS_UPDATED, 1);
    expect_value_count(test_power_cap_callback, current, TEST_NEW_POWER_CAP, 1);
    expect_value_count(test_power_cap_callback, previous, CONFIGURED_THERMAL_WATTS_LIMIT, 1);
    // provide a new cap with a new callback (old call back should be the one that gets the notice)
    response = power_cap_update(test_power_cap_callback2, TEST_NEW_POWER_CAP, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);
}

POWER_TEST(power_cap_update__previous_not_complete__cli, setup, teardown)
{
    // have to call init to ensure internal state
    power_cap_init();

    // using different caps to confirm changes
    int response = power_cap_update(test_power_cap_callback, CONFIGURED_THERMAL_WATTS_LIMIT, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);
    // expected no failure callback
    // provide a new cap with a new callback (old call back should be the one that gets the notice)
    response = power_cap_update(test_power_cap_callback2, TEST_NEW_POWER_CAP, true);
    assert_int_equal(response, MP_POWER_CAP_FAIL_CLI_NOT_ALLOWED);
}

POWER_TEST(power_cap_update__finalize_callback, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};

    // have to call init to ensure internal state
    power_cap_init();

    // using different caps to confirm changes
    int response = power_cap_update(test_power_cap_callback, CONFIGURED_THERMAL_WATTS_LIMIT, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);
    // expected values for failure callback
    expect_value_count(test_power_cap_callback, result, MP_POWER_CAP_SUCCESS, 1);
    expect_value_count(test_power_cap_callback, current, CONFIGURED_THERMAL_WATTS_LIMIT, 1);
    expect_value_count(test_power_cap_callback, previous, NO_POWER_CAP, 1);

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);
    // query cpu cap to update internal state
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    // indicate cap is finalized - should also save to warm start since cap changed
    expect_function_call(__wrap_power_ws_save_pwr_cap);
    power_cap_finalize();
}

POWER_TEST(power_cap_cancel, setup, teardown)
{
    // have to call init to ensure internal state
    power_cap_init();

    int response = power_cap_update(test_power_cap_callback, TEST_NEW_POWER_CAP, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);

    // expected values for failure callback
    expect_value_count(test_power_cap_callback, result, MP_POWER_CAP_FAIL_PREVIOUS_UPDATED, 1);
    expect_value_count(test_power_cap_callback, current, NO_POWER_CAP, 1);
    expect_value_count(test_power_cap_callback, previous, TEST_NEW_POWER_CAP, 1);

    response = power_cap_cancel(NULL, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);
}

POWER_TEST(power_cap_is_capped, setup, teardown)
{
    power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};

    // have to call init to ensure internal state
    power_cap_init();

    int response = power_cap_update(test_power_cap_callback, TEST_NEW_POWER_CAP, false);
    assert_int_equal(response, MP_POWER_CAP_PENDING);

    // setup our test runconfig as the response to the power_runconfig_get
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);
    // query cpu cap to update internal state
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);

    assert_true(power_cap_is_capped());
}
