//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_log_test.cpp
 * Power service fuse tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {
#include "corebits.h"   // for corebits_t
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h>  // for expect_any_always, CmockaWrapperTest
#include <assert.h>         // for assert
#include <cstddef>          // for NULL, size_t
#include <fpfw_status.h>    // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <mock_bug_check.h> // for __wrap_crash_dump_bug_check
#include <modules/CdDumpDescriptor.h>
#include <power_i.h>
#include <power_log.h>
#include <string.h>
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
power_log_data_t* log1;

static char log_data[POWER_LOG_LOCAL_SIZE] = {};
#define LOG_TS ((uint64_t)0)

bool use_wrap = false;

static int setup(void** state)
{
    UNUSED(state);
    log1 = get_instance();
    log1->mask = UINT32_MAX;
    log1->entries = (power_log_entry_t*)log_data;
    log1->last_entry = 0; // ensure we start at 0, so math in tests doesn't have to deal with wrap
    memset(log1->entries, 0, POWER_LOG_LOCAL_SIZE);
    power_log_use_ddr(false);
    power_log_update_timestamp(0);

    use_wrap = true;

    return 0;
}

static int teardown(void** state)
{
    UNUSED(state);

    use_wrap = false;

    /* ensure use ddr disabled */
    power_log_use_ddr(false);
    return 0;
}

void validate_entry(unsigned entry_idx, uint64_t timestamp, uint8_t type, power_log_payload_t* payload)
{
    power_log_entry_t* entry = &log1->entries[entry_idx];
    assert_int_equal(entry->timestamp, timestamp);
    assert_int_equal(entry->type, type);
    assert_memory_equal(&entry->payload, payload, sizeof(power_log_payload_t));
}

void validate_last_entry(uint64_t timestamp, uint8_t type, power_log_payload_t* payload)
{
    validate_entry(log1->last_entry, timestamp, type, payload);
}
}
/* verifies power_log_cores_ts updates timestamp and places entry in entries buffer */
POWER_TEST(log_core_ts, setup, teardown)
{

    power_log_payload_t test_payload = {.all = {'t', 'e', 's', 't'}};
    // power_log_payload_t test_payload = {};
    power_log_cores_ts(LOG_TS, &ALLCORES, TEST_ENTRY_NO_CORES, &test_payload);
    // verify last timestamp updated and log entry produced
    assert_int_equal(log1->last_timestamp, LOG_TS);
    validate_last_entry(LOG_TS, TEST_ENTRY_NO_CORES, &test_payload);
}

/* verifies power_log_core converts the given core into a corebits entry in the entry it creates in the entries buffer */
POWER_TEST(log_core, setup, teardown)
{
#define TEST_CORE_NUM 5

    corebits_t expected_cores = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM);

    power_log_payload_t test_payload = {
        .all = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'}};
    power_log_core(TEST_CORE_NUM, TEST_ENTRY_CORES, &test_payload);

    // update payload cores with expectation
    test_payload.cores = expected_cores;

    // verify log entry produced
    validate_last_entry(log1->last_timestamp, TEST_ENTRY_CORES, &test_payload);
}

/* verifies the behavior of core log with same payload/timestamp only being created in entries buffer once */
POWER_TEST(log_cores_update_first, setup, teardown)
{
#define TEST_CORE_NUM 5

    corebits_t expected_cores = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM);
    corebits_t expected_cores_2 = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM + 1);

    power_log_payload_t test_payload_a = {.all = {'t', 'e', 's', 't'}};
    power_log_payload_t test_payload_b = {
        .all = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'}};

    // 1st log
    power_log_cores(&expected_cores, TEST_ENTRY_CORES, &test_payload_b);
    // 2nd log
    power_log_cores(&expected_cores, TEST_ENTRY_NO_CORES, &test_payload_a);
    // 3rd log (should update first - same payload, different core with no timestamp change)
    power_log_cores(&expected_cores_2, TEST_ENTRY_CORES, &test_payload_b);

    // combine expecteds
    corebits_or(&expected_cores, &expected_cores_2);
    // update payload cores with expectation
    test_payload_b.cores = expected_cores;

    // verify log entries produced
    validate_last_entry(log1->last_timestamp, TEST_ENTRY_NO_CORES, &test_payload_a);
    validate_entry(log1->last_entry - 1, log1->last_timestamp, TEST_ENTRY_CORES, &test_payload_b);
}

/* verifies the behavior of core log with same payload/timestamp only being created in entries buffer once - changed ordering */
POWER_TEST(log_cores_update_second, setup, teardown)
{
#define TEST_CORE_NUM 5

    corebits_t expected_cores = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM);
    corebits_t expected_cores_2 = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM + 1);

    power_log_payload_t test_payload_a = {.all = {'t', 'e', 's', 't'}};
    power_log_payload_t test_payload_b = {
        .all = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'}};

    // 1st log
    power_log_cores(&expected_cores, TEST_ENTRY_NO_CORES, &test_payload_a);
    // 2nd log
    power_log_cores(&expected_cores, TEST_ENTRY_CORES, &test_payload_b);
    // 3rd log (should update previous - same payload, different core with no timestamp change)
    power_log_cores(&expected_cores_2, TEST_ENTRY_CORES, &test_payload_b);

    // combine expecteds
    corebits_or(&expected_cores, &expected_cores_2);
    // update payload cores with expectation
    test_payload_b.cores = expected_cores;

    // verify log entries produced
    validate_entry(log1->last_entry - 1, log1->last_timestamp, TEST_ENTRY_NO_CORES, &test_payload_a);
    validate_last_entry(log1->last_timestamp, TEST_ENTRY_CORES, &test_payload_b);
}

/* verifies the behavior of core log with same payload different timestamp creating multiple entries */
POWER_TEST(log_cores_update_ts_between, setup, teardown)
{
#define TEST_CORE_NUM 5
    // UNUSED(pContext);

    uint64_t original_ts = log1->last_timestamp;
    corebits_t expected_cores = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM);
    corebits_t expected_cores_2 = {0};
    corebits_set_bit(&expected_cores, TEST_CORE_NUM + 1);

    power_log_payload_t test_payload_a = {.all = {'t', 'e', 's', 't'}};
    power_log_payload_t test_payload_b = {
        .all = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'}};

    // 1st log
    power_log_cores(&expected_cores, TEST_ENTRY_NO_CORES, &test_payload_a);
    // 2nd log
    power_log_cores(&expected_cores, TEST_ENTRY_CORES, &test_payload_b);
    // 3rd log (should be a third entry - same payload as previous, different core, but with timestamp change)
    power_log_update_timestamp(LOG_TS);
    power_log_cores(&expected_cores_2, TEST_ENTRY_CORES, &test_payload_b);

    // verify log entries produced
    validate_entry(log1->last_entry - 2, original_ts, TEST_ENTRY_NO_CORES, &test_payload_a);
    // update payload cores with expectation
    test_payload_b.cores = expected_cores;
    validate_entry(log1->last_entry - 1, original_ts, TEST_ENTRY_CORES, &test_payload_b);
    // update payload cores with expectation
    test_payload_b.cores = expected_cores_2;
    validate_last_entry(log1->last_timestamp, TEST_ENTRY_CORES, &test_payload_b);
}

/* verifies the power_log_has cores for test entries */
POWER_TEST(log_has_cores, setup, teardown)
{

    // confirm the two test entries match expectations
    assert_true(power_log_has_cores(TEST_ENTRY_CORES));
    assert_false(power_log_has_cores(TEST_ENTRY_NO_CORES));
}

POWER_TEST(log_string, setup, teardown)
{
#define TEST_STRING "test string"

    power_log_payload_t test_payload = {.all = TEST_STRING};

    char expected1[] = "TE1 - " TEST_STRING;
    char expected2[] = "TE2";

    assert_memory_equal(power_log_string(TEST_ENTRY_NO_CORES, &test_payload), &expected1, sizeof(expected1));
    assert_memory_equal(power_log_string(TEST_ENTRY_CORES, &test_payload), &expected2, sizeof(expected2));
}

POWER_TEST(power_log_init, setup, teardown)
{

    expect_function_call(__wrap_crash_dump_register_address32);
    power_log_init();
}