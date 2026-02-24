//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_utils.cpp
 * Unit tests for utils delay functions (delay_init and sleep_ms)
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <stdint.h>
#include <utils.h>

/*-- Declarations (Statics and globals) --*/
static uint64_t s_mock_counter_value = 0;
static uint32_t s_mock_counter_call_count = 0;
static uint64_t s_mock_counter_increment = 0;

/*------------- Mock Counter ----------------*/

static uint64_t mock_get_counter(void)
{
    uint64_t val = s_mock_counter_value;
    s_mock_counter_value += s_mock_counter_increment;
    s_mock_counter_call_count++;
    return val;
}

static void reset_mock_counter(void)
{
    s_mock_counter_value = 1000;
    s_mock_counter_call_count = 0;
    s_mock_counter_increment = 0;
}

} /* extern "C" */

/*------------- Test Setup ----------------*/

static int test_setup(void** state)
{
    (void)state;
    reset_mock_counter();
    return 0;
}

/*------------- Tests ----------------*/

TEST_FUNCTION(test_delay_init_valid, test_setup, nullptr)
{
    delay_init(1000000U, mock_get_counter);
    s_mock_counter_increment = 1000;
    sleep_ms(1);
    assert_true(s_mock_counter_call_count >= 2);
}

TEST_FUNCTION(test_delay_init_zero_freq, test_setup, nullptr)
{
    delay_init(0, mock_get_counter);
    sleep_ms(0);
}

TEST_FUNCTION(test_delay_init_null_counter, test_setup, nullptr)
{
    delay_init(1000000U, NULL);
    sleep_ms(0);
}

TEST_FUNCTION(test_sleep_ms_zero, test_setup, nullptr)
{
    delay_init(1000000U, mock_get_counter);
    uint32_t call_count_before = s_mock_counter_call_count;
    sleep_ms(0);
    assert_true((s_mock_counter_call_count - call_count_before) <= 2);
}

TEST_FUNCTION(test_sleep_ms_spins_to_target, test_setup, nullptr)
{
    delay_init(1000000U, mock_get_counter);
    s_mock_counter_increment = 100;
    s_mock_counter_call_count = 0;
    sleep_ms(1);
    assert_true(s_mock_counter_call_count >= 10);
}

TEST_FUNCTION(test_sleep_ms_multiple_ms, test_setup, nullptr)
{
    delay_init(1000000U, mock_get_counter);
    s_mock_counter_increment = 500;
    s_mock_counter_call_count = 0;
    sleep_ms(5);
    assert_true(s_mock_counter_call_count >= 10);
}

TEST_FUNCTION(test_delay_init_idempotent, test_setup, nullptr)
{
    delay_init(1000000U, mock_get_counter);
    delay_init(2000000U, mock_get_counter);
    s_mock_counter_increment = 2000;
    s_mock_counter_call_count = 0;
    sleep_ms(1);
    assert_true(s_mock_counter_call_count >= 2);
    assert_true(s_mock_counter_call_count <= 4);
}
