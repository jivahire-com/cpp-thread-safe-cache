//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_aging_counters.cpp
 * Test  for aging counters
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <aging_counters_i.h>
#include <kng_soc_constants.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern aging_counter_t core_aging[NUMBER_OF_CORES_PER_DIE];
}

#define TEST_CORE_ID (0)
/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

TEST_FUNCTION(test_aging_counter_init, test_setup, test_teardown)
{
    // enabled cores
    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);
    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);
    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);
    uint8_t core_id = 0;
    for (core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);
    }

    aging_counter_init();
}

TEST_FUNCTION(test_aging_counter_get_sensor_status, test_setup, test_teardown)
{
    aging_sensor_status_t status;
    uint8_t core_id = 0;

    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 1);
    status = aging_counter_get_sensor_status(core_id);
    assert_int_equal(status, MEASUREMENT_COMPLETE);

    // case 2 fail case

    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 0);
    status = aging_counter_get_sensor_status(core_id);
    assert_int_equal(status, MEASUREMENT_NOT_COMPLETE);
}

TEST_FUNCTION(test_aging_counter_enable_sensor_measurement, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t counter_id = 0;
    expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);
    aging_counter_enable_sensor_measurement(core_id, counter_id);
}

TEST_FUNCTION(test_aging_counter_enable_sensor_measurement_fail, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t counter_id = NUMBER_OF_AGING_COUNTER_PAIRS;
    aging_counter_enable_sensor_measurement(core_id, counter_id);
}

TEST_FUNCTION(test_aging_counter_read, test_setup, test_teardown)
{
    uint32_t counter_a = 0;
    uint32_t counter_b = 0;
    uint8_t core_id = 0;
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 30);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 28);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 0);

    aging_counter_read(core_id, &counter_a, &counter_b);
    assert_int_equal(counter_a, 30);
    assert_int_equal(counter_b, 28);
}

TEST_FUNCTION(test_aging_counter_read_fail, test_setup, test_teardown)
{
    uint32_t counter_a = 0;
    uint32_t counter_b = 0;
    uint8_t core_id = NUM_AP_CORES_PER_DIE;
    // case  1: core id out of range
    bool status = aging_counter_read(core_id, &counter_a, &counter_b);

    assert_int_equal(status, false);
    // case 2 : counter not armed
    core_aging[core_id].measurement_armed = false;
    status = aging_counter_read(core_id, &counter_a, &counter_b);
    assert_int_equal(status, false);

    // case 3 : counter id out of range
    core_aging[core_id].measurement_index = NUMBER_OF_AGING_COUNTER_PAIRS + 1;
    status = aging_counter_read(core_id, &counter_a, &counter_b);
    assert_int_equal(status, false);
}

TEST_FUNCTION(test_aging_counter_read_fail_from_silibs, test_setup, test_teardown)
{
    uint32_t counter_a = 0;
    uint32_t counter_b = 0;
    uint8_t core_id = 0;

    core_aging[core_id].measurement_armed = true;
    core_aging[core_id].measurement_index = 0;

    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 30);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 28);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 1);

    bool status = aging_counter_read(core_id, &counter_a, &counter_b);
    assert_int_equal(status, false);
}

TEST_FUNCTION(test_aging_counter_reset, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    for (core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_aging[core_id].measurement_armed = false;
        expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);
    }
    aging_counter_reset();
    for (core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core_aging[core_id].measurement_armed, true);
    }
}