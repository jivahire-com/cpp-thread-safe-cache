//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main_test.cpp
 * Power service main tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> //

extern "C" {

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <pid_resource.h>  // for pid_config_t, pid_context_t, pid_reset

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

int pid_adjust_tests(float measurement, uint32_t initial_resources, uint32_t eventual_resources)
{
#define MAX_ITER 10000 // a maximum number of iterations to wait for a change
    int count = 0;
    pid_set_resources(initial_resources);
    uint32_t updated_resources = initial_resources;
    while ((updated_resources != eventual_resources) && (count++ < MAX_ITER))
    {
        updated_resources = pid_calculate_resources(0.001, measurement);
    }
    // FWK_LOG_INFO("u %d i %d e %d c %d", updated_resources, initial_resources,
    // eventual_resources, count);
    assert_true(count < MAX_ITER);
    assert_int_equal(updated_resources, eventual_resources);
    return count;
}

/* test pid_resource management functionality */
POWER_TEST(pid_resource, NULL, NULL)
{
    const pid_config_t config = {
        .kp = 15,
        .ki = 2000,
        .kd = 0,
        .setpoint = 200,
        .setpoint_offset = 0,
        .max = 5000,
    };

    /* test that init succeeds, provide an init that works with tests */
    expect_value(__wrap_FpFwAssert, expression, true);
    pid_init(&config);

    uint32_t initial_resources = config.max / 2;

    /* start by testing that resources don't adjust if measured value ==
     * setpoint */
    float setpoint = 200;
    float actual_setpoint = setpoint - config.setpoint_offset;

    pid_reset();
    pid_set_resources(initial_resources);
    pid_update_setpoint(setpoint);
    uint32_t updated_resources = pid_calculate_resources(0.001, actual_setpoint);
    assert_int_equal(initial_resources, updated_resources);

    /* verify initial resource count limited to max */
    pid_set_resources(config.max * 2);
    updated_resources = pid_calculate_resources(0.001, actual_setpoint);
    assert_int_equal(config.max, updated_resources);

    pid_reset();
    /* confirm that resources will adjust to max if measurement <
     * set_point (measurement 90% of setpoint) */
    int first_count = pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    /* confirm that resources will adjust to max if measurement <
     * set_point (measurement 80% of setpoint) */
    int second_count = pid_adjust_tests(actual_setpoint * .80f, initial_resources, config.max);
    /* confirm that adjustment took less iterations when error was greater */
    assert_true(second_count < first_count);

    pid_reset();
    /* confirm that resources will adjust to 0 if measurement > set_point
     * (measurement 110% of setpoint) */
    first_count = pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    /* confirm that resources will adjust to 0 if measurement > set_point
     * (measurement 120% of setpoint) */
    second_count = pid_adjust_tests(actual_setpoint * 1.20f, initial_resources, 0);
    /* confirm that adjustment took less iterations when error was greater */
    assert_true(second_count < first_count);

    /* integral tests */
    pid_reset();
    /* wind up integral in one direction */
    pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    /* adjust in opposite direction */
    first_count = pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    /* same adjustment again */
    second_count = pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    assert_true(second_count < first_count);
    /* same adjustment with reset */
    pid_reset();
    second_count = pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    assert_true(second_count < first_count);

    /* accumulate error test */
    pid_reset();
    /* wind up integral in one direction */
    pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    for (int i = 0; i < 100; ++i)
    {
        /* continue to push the same error another 100 iterations once we're
         * saturated */
        updated_resources = pid_calculate_resources(0.001, actual_setpoint * .90f);
    }
    /* adjust in opposite direction */
    int this_count = pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    /* ensure no additional error accumulated */
    assert_int_equal(this_count, first_count);

    /* opposite of above */
    pid_reset();
    /* wind up integral in one direction */
    pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    /* adjust in opposite direction */
    first_count = pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    /* same adjustment again */
    second_count = pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    assert_true(second_count < first_count);
    /* same adjustment with reset */
    pid_reset();
    second_count = pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    assert_true(second_count < first_count);

    /* accumulate error test */
    pid_reset();
    /* wind up integral in one direction */
    pid_adjust_tests(actual_setpoint * 1.10f, initial_resources, 0);
    for (int i = 0; i < 100; ++i)
    {
        /* continue to push the same error another 100 iterations once we're
         * saturated */
        updated_resources = pid_calculate_resources(0.001, actual_setpoint * 1.10f);
    }
    /* adjust in opposite direction */
    this_count = pid_adjust_tests(actual_setpoint * .90f, initial_resources, config.max);
    /* ensure no additional error accumulated */
    assert_int_equal(this_count, first_count);
}

POWER_TEST(pid_resource__get_config, NULL, NULL)
{
    const pid_config_t config = {
        .kp = 15,
        .ki = 2000,
        .kd = 0,
        .setpoint = 200,
        .setpoint_offset = 0,
        .max = 5000,
    };

    /* test that init succeeds, provide an init that works with tests */
    expect_value(__wrap_FpFwAssert, expression, true);
    pid_init(&config);

    pid_config_t test_config;
    expect_value(__wrap_FpFwAssert, expression, true);
    pid_get_config(&test_config);

    assert_memory_equal(&config, &test_config, sizeof(pid_config_t));
}

POWER_TEST(pid_resource__get_context, NULL, NULL)
{
    const pid_context_t context = {
        .prev_error = 0.123F,
        .integral = 456.0F,
        .available_resources = 789,
    };

    /* set values to internal context */
    expect_value(__wrap_FpFwAssert, expression, true);
    pid_set_context(&context);

    /* get values and compare */
    pid_context_t test_context;
    expect_value(__wrap_FpFwAssert, expression, true);
    pid_get_context(&test_context);

    assert_memory_equal(&context, &test_context, sizeof(pid_context_t));
}
