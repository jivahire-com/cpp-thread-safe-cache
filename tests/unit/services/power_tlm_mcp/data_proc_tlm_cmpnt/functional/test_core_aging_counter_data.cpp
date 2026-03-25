//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_aging_counter_data.cpp
 * Test functionality and flow for aging counter data collection
 *
 * Objective:
 *   Validate aging counter data collection, processing, and packaging.
 *   Test register-level mock interactions for aging counter hardware APIs.
 *
 * Generic Test Flow:
 * ----------
 * - Set up mock register read/write operations for aging counter APIs
 * - Initialize aging counter state (armed measurement)
 * - Mock DVFS register status and counter data
 * - Trigger: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()
 * - Call: package_create_pwr_core_aging_record()
 * - Verify/Assert results for aging counter data:
 *
 * Test Functions:
 * ---------------
 * 1. test_single_core_aging_counter_complete: Tests single core aging counter measurement completion
 * 2. test_aging_counter_last_pair_boundary: Tests behavior when reading the last counter pair (index 7)
 *    - Verifies measurement_armed becomes false after last counter read
 *    - Checks measurement_index increments to 8 (past the last valid index 0-7) after last pair read
 * 3. test_all_cores_aging_counter_complete: Tests all cores processing aging counters simultaneously
 *    - Validates that all 4 cores can read aging counters with unique values in parallel
 *    - Verifies each core's data is correctly processed and packaged independently
 *
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779228

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <aging_counters_i.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <telemetry_package_defs.h>
extern uint32_t time_t0;
extern aging_counter_t core_aging[NUMBER_OF_CORES_PER_DIE];
extern bool core_is_active[NUMBER_OF_CORES_PER_DIE];
extern int g_enable_mock_die_id; // Enable/disable die ID mocking for aging counter tests
}

// Global variable to control debug printing
static bool g_print_logs = true;

#define NUMBER_OF_CORES_TO_TEST (4)

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();

    // Enable die ID mocking for aging counter tests
    g_enable_mock_die_id = 1;

    // Initialize core active status and aging counter state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        core_is_active[core_id] = true; // Mark cores as active for aging counter processing

        // Initialize aging counter state
        core_aging[core_id].measurement_armed = true;
        core_aging[core_id].measurement_index = 0; // Start with first counter pair

        // Initialize core runtime voltage and temperature (needed for aging counter metrics)
        core_rt[core_id].latest_vmin_mV = 1000;     // 1.0V
        core_rt[core_id].latest_max_value_dC = 850; // 85.0°C
    }

    // Reset mock timestamp to initial value
    time_t0 = 100;

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);

    // Disable die ID mocking after test completes
    g_enable_mock_die_id = 0;

    reset_pwr_tlm_data();
    return 0;
}

// Helper function to set up mock aging counter register operations
static void setup_mock_aging_counter_ready(uint8_t core_id, uint32_t aged_counter, uint32_t unaged_counter)
{
    FPFW_UNUSED(core_id);

    // Mock aging sensor status as measurement complete (returns int, not enum)
    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 1); // 1 = MEASUREMENT_COMPLETE

    // Mock successful counter data read (returns int, not silibs_status_t)
    // Note: aging_counter_read() calls with parameters: counter_a = unaged, counter_b = aged
    // So we return: bank_a (counter_a) = unaged, bank_b (counter_b) = aged
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, unaged_counter); // bank_a_counter (counter_a = unaged)
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, aged_counter);   // bank_b_counter (counter_b = aged)
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 0);              // 0 = success

    // Expect enable measurement call for next counter
    expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);
}

// Helper function to set up mock aging counter not ready
static void setup_mock_aging_counter_not_ready(void)
{
    // Mock aging sensor status as measurement not complete (returns int, 0 = not complete)
    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 0); // 0 = MEASUREMENT_NOT_COMPLETE
}

// Test single core aging counter measurement completion and data processing
// Validates basic aging counter read, compute metrics update, and package creation
//
// Test Flow Summary:
// ==================
// 1. Initialize: measurement_index=0, measurement_armed=true
// 2. Mock: aging counter measurement ready with aged=12345, unaged=10000
// 3. Trigger: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()
// 4. Verify: measurement_index incremented to 1
// 5. Create package: package_create_pwr_core_aging_record()
// 6. Assert: Package data matches expected values (counter_id=0, aged=12345, unaged=10000,
//            timestamp=5000000us, vmin_mV=1000, temperature_dC=850)
TEST_FUNCTION(test_single_core_aging_counter_complete, test_setup, test_teardown)
{
    // ===== INPUT VALUES =====
    const uint8_t test_core_id_1 = 1;
    const uint32_t input_aged_counter_1 = 12345;
    const uint32_t input_unaged_counter_1 = 10000;
    const uint8_t test_core_id_2 = 2;
    const uint32_t input_aged_counter_2 = 55555;
    const uint32_t input_unaged_counter_2 = 44444;
    const uint64_t input_timestamp_uS = 5000000;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_single_core_aging_counter_complete ---\n");
        printf("=== MULTI-CORE AGING COUNTER TEST ===\n");
        printf("Testing core %d with aged=%u, unaged=%u\n", test_core_id_1, input_aged_counter_1, input_unaged_counter_1);
        printf("Testing core %d with aged=%u, unaged=%u\n", test_core_id_2, input_aged_counter_2, input_unaged_counter_2);
    }

    // Setup mocks for data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg() infrastructure calls (executed first)
    // Mock multi-die communication and SCP telemetry service notifications
    // Note: Order matches actual implementation - SCP notified first, then secondary MCPs
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    will_return(__wrap_die_2_die_exch_get_this_die_id, 0); // PRIMARY_DIE_ID
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);

    // Setup mock aging counter register operations (executed after infrastructure calls)
    // Cores 1 and 2 have measurements ready, cores 0 and 3 not ready
    setup_mock_aging_counter_not_ready(); // core 0: not ready
    setup_mock_aging_counter_ready(test_core_id_1, input_aged_counter_1, input_unaged_counter_1); // core 1: ready
    setup_mock_aging_counter_ready(test_core_id_2, input_aged_counter_2, input_unaged_counter_2); // core 2: ready
    setup_mock_aging_counter_not_ready(); // core 3: not ready

    // Set up timestamp for aging counter processing
    // The mock timestamp function increments time_t0 by MOCK_TIMESTAMP_INCREMENT (1000us) each call
    // The aging counter processing calls exec_tlm_cmpnt_get_timestamp_microseconds() to get current timestamp
    // We need to account for the increment that happens during the call
    time_t0 = input_timestamp_uS - 1000; // Will return input_timestamp_uS when called

    if (g_print_logs)
        printf("Pre-processing time_t0: %u us, expected return: %" PRIu64 " us\n", time_t0, input_timestamp_uS);

    // Trigger complete power package data preparation
    if (g_print_logs)
        printf("Calling data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()...\n");

    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    // Create aging counter package and verify results
    pwr_core_record_aging_t aging_record = {{0}};
    uint32_t record_size = package_create_pwr_core_aging_record(&aging_record);

    if (g_print_logs)
        printf("Package created successfully, size: %u bytes\n", record_size);

    // Creating aging_record as a local variable for testing purpose only. So that we can assert
    // the actual values copied locally to expected values. In production it gets copied to memory.
    // In CMocka reading the actual values from memory might not be straight forward, and hence
    // copied locally after package creation and then validated.
    //
    // aging_record structure hierarchy:
    //   aging_record → aging_collection[core_id] → aging_element[0..7] (8 counter pairs per core)
    //
    // aging_elements pointer points to the first counter pair (aging_element[0]) for each core
    pwr_core_element_aging_t* aging_elements_1 = &aging_record.aging_collection[test_core_id_1].aging_element[0];
    pwr_core_element_aging_t* aging_elements_2 = &aging_record.aging_collection[test_core_id_2].aging_element[0];

    // ===== EXPECTED VALUES (after package creation) =====
    // Core 1 expected values
    const uint8_t expected_counter_id_1 = 0;        // First counter pair (measurement_index starts at 0)
    const uint8_t expected_measurement_index_1 = 1; // Should increment after successful read
    const uint32_t expected_aged_counter_1 = input_aged_counter_1;
    const uint32_t expected_unaged_counter_1 = input_unaged_counter_1;
    const uint64_t expected_timestamp_uS =
        input_timestamp_uS; // Mock timestamp from exec_tlm_cmpnt_get_timestamp_microseconds()
    const uint32_t expected_voltage_mV = 1000;    // Set in test_setup() as core_rt[].latest_voltage_mV
    const uint32_t expected_temperature_dC = 850; // Set in test_setup() as core_rt[].latest_max_value_dC

    // Core 2 expected values
    const uint8_t expected_counter_id_2 = 0;        // First counter pair (measurement_index starts at 0)
    const uint8_t expected_measurement_index_2 = 1; // Should increment after successful read
    const uint32_t expected_aged_counter_2 = input_aged_counter_2;
    const uint32_t expected_unaged_counter_2 = input_unaged_counter_2;

    // ===== PRINT ACTUAL VS EXPECTED =====
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON ===\n");

        // Core 1 results
        printf("Core %d Aging Counter State:\n", test_core_id_1);
        printf("  measurement_index - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_1].measurement_index,
               expected_measurement_index_1);
        printf("Core %d Package Data:\n", test_core_id_1);
        printf("  Counter ID       - Actual: %d, Expected: %d\n", aging_elements_1[0].counter_id, expected_counter_id_1);
        printf("  Aged Counter     - Actual: %u, Expected: %u\n", aging_elements_1[0].aged_counter, expected_aged_counter_1);
        printf("  Unaged Counter   - Actual: %u, Expected: %u\n", aging_elements_1[0].unaged_counter, expected_unaged_counter_1);
        printf("  Timestamp (us)   - Actual: %" PRIu64 ", Expected: %" PRIu64 "\n",
               aging_elements_1[0].timestamp_uS,
               expected_timestamp_uS);
        printf("  Voltage (mV)     - Actual: %u, Expected: %u\n", aging_elements_1[0].voltage_mV, expected_voltage_mV);
        printf("  Temperature (dC) - Actual: %u, Expected: %u\n", aging_elements_1[0].temperature_dC, expected_temperature_dC);

        // Core 2 results
        printf("\nCore %d Aging Counter State:\n", test_core_id_2);
        printf("  measurement_index - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_2].measurement_index,
               expected_measurement_index_2);
        printf("Core %d Package Data:\n", test_core_id_2);
        printf("  Counter ID       - Actual: %d, Expected: %d\n", aging_elements_2[0].counter_id, expected_counter_id_2);
        printf("  Aged Counter     - Actual: %u, Expected: %u\n", aging_elements_2[0].aged_counter, expected_aged_counter_2);
        printf("  Unaged Counter   - Actual: %u, Expected: %u\n", aging_elements_2[0].unaged_counter, expected_unaged_counter_2);
        printf("  Timestamp (us)   - Actual: %" PRIu64 ", Expected: %" PRIu64 "\n",
               aging_elements_2[0].timestamp_uS,
               expected_timestamp_uS);
        printf("  Voltage (mV)     - Actual: %u, Expected: %u\n", aging_elements_2[0].voltage_mV, expected_voltage_mV);
        printf("  Temperature (dC) - Actual: %u, Expected: %u\n", aging_elements_2[0].temperature_dC, expected_temperature_dC);
    }

    // ===== ASSERT ACTUAL VS EXPECTED =====
    // Verify core 1 aging counter state and package data
    assert_int_equal(core_aging[test_core_id_1].measurement_index, expected_measurement_index_1);
    assert_int_equal(aging_elements_1[0].counter_id, expected_counter_id_1);
    assert_int_equal(aging_elements_1[0].aged_counter, expected_aged_counter_1);
    assert_int_equal(aging_elements_1[0].unaged_counter, expected_unaged_counter_1);
    assert_int_equal(aging_elements_1[0].timestamp_uS, expected_timestamp_uS);
    assert_int_equal(aging_elements_1[0].voltage_mV, expected_voltage_mV);
    assert_int_equal(aging_elements_1[0].temperature_dC, expected_temperature_dC);

    // Verify core 2 aging counter state and package data
    assert_int_equal(core_aging[test_core_id_2].measurement_index, expected_measurement_index_2);
    assert_int_equal(aging_elements_2[0].counter_id, expected_counter_id_2);
    assert_int_equal(aging_elements_2[0].aged_counter, expected_aged_counter_2);
    assert_int_equal(aging_elements_2[0].unaged_counter, expected_unaged_counter_2);
    assert_int_equal(aging_elements_2[0].timestamp_uS, expected_timestamp_uS);
    assert_int_equal(aging_elements_2[0].voltage_mV, expected_voltage_mV);
    assert_int_equal(aging_elements_2[0].temperature_dC, expected_temperature_dC);

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_single_core_aging_counter_complete ---\n");
        printf("***\n");
    }
}

// Test behavior when reading the last counter pair (index 7)
// This test validates proper handling of the boundary condition at the last counter pair
//
// Test Flow Summary:
// ==================
// 1. Initialize: measurement_index=6, measurement_armed=true
// 2. Mock: Read counter pair 6 (aged=88888, unaged=77777)
// 3. Trigger: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()
// 4. Mock: Read counter pair 7 (last valid counter, aged=99999, unaged=88888)
// 5. Trigger: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg() again
// 6. Create package: package_create_pwr_core_aging_record()
// 7. Expected: measurement_armed=false, measurement_index=8 (incremented past last valid index 0-7)
// 8. Assert: measurement_index=8, measurement_armed=false
// 9. Verify: Package contains correct data for both counter pairs (counter_id, aged, unaged
//            counters for pairs 6 and 7)
TEST_FUNCTION(test_aging_counter_last_pair_boundary, test_setup, test_teardown)
{
    // ===== INPUT VALUES =====
    const uint8_t test_core_id_1 = 1;
    const uint32_t input_aged_counter_1_6 = 88888;
    const uint32_t input_unaged_counter_1_6 = 77777;
    const uint32_t input_aged_counter_1_7 = 99999;
    const uint32_t input_unaged_counter_1_7 = 88888;

    const uint8_t test_core_id_2 = 2;
    const uint32_t input_aged_counter_2_6 = 11111;
    const uint32_t input_unaged_counter_2_6 = 22222;
    const uint32_t input_aged_counter_2_7 = 33333;
    const uint32_t input_unaged_counter_2_7 = 44444;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_aging_counter_last_pair_boundary ---\n");
        printf("=== LAST COUNTER PAIR BOUNDARY TEST (MULTI-CORE) ===\n");
        printf("Testing cores %d and %d transitioning from counter 6 to 7 (last pair)\n", test_core_id_1, test_core_id_2);
    }

    // Setup: Set measurement_index to 6 (second-to-last counter pair) for both cores
    core_aging[test_core_id_1].measurement_index = 6;
    core_aging[test_core_id_1].measurement_armed = true;
    core_aging[test_core_id_2].measurement_index = 6;
    core_aging[test_core_id_2].measurement_armed = true;

    if (g_print_logs)
    {
        printf("Initial state:\n");
        printf("  Core %d: measurement_index=%d, measurement_armed=%d\n",
               test_core_id_1,
               core_aging[test_core_id_1].measurement_index,
               core_aging[test_core_id_1].measurement_armed);
        printf("  Core %d: measurement_index=%d, measurement_armed=%d\n",
               test_core_id_2,
               core_aging[test_core_id_2].measurement_index,
               core_aging[test_core_id_2].measurement_armed);
    }

    // ===== STEP 1: Read counter pair 6 for both cores =====
    if (g_print_logs)
        printf("\n--- Step 1: Reading counter pair 6 ---\n");

    // Setup mocks for infrastructure calls
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    will_return(__wrap_die_2_die_exch_get_this_die_id, 0); // PRIMARY_DIE_ID
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);

    // Setup mock for counter pair 6 read (cores 1 and 2 ready, cores 0 and 3 not ready)
    // At measurement_index=6, after read it becomes 7 (still valid), so expect enable call
    setup_mock_aging_counter_not_ready(); // core 0: not ready
    setup_mock_aging_counter_ready(test_core_id_1, input_aged_counter_1_6, input_unaged_counter_1_6); // core 1: ready
    setup_mock_aging_counter_ready(test_core_id_2, input_aged_counter_2_6, input_unaged_counter_2_6); // core 2: ready
    setup_mock_aging_counter_not_ready(); // core 3: not ready

    // Trigger data processing
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    // ===== STEP 2: Read counter pair 7 (last counter) for both cores =====
    if (g_print_logs)
        printf("\n--- Step 2: Reading counter pair 7 (LAST) ---\n");

    // Setup mocks for infrastructure calls
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    will_return(__wrap_die_2_die_exch_get_this_die_id, 0); // PRIMARY_DIE_ID
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);

    // Setup mock for last counter pair read (cores 1 and 2 ready, cores 0 and 3 not ready)
    setup_mock_aging_counter_not_ready(); // core 0: not ready

    // Note: Cannot use helper function setup_mock_aging_counter_ready() here because it expects
    // a call to __wrap_dvfs_c2_pcm_enable_aging_sensor_measurement, but at measurement_index=8
    // after reading counter 7, the boundary check prevents this call from happening.
    // So setting up the mocks inline without the expect_function_call to __wrap_dvfs_c2_pcm_enable_aging_sensor_measurement.
    // setup_mock_aging_counter_ready(test_core_id_1, input_aged_counter_1_6, input_unaged_counter_1_6); // core 1: ready
    // setup_mock_aging_counter_ready(test_core_id_2, input_aged_counter_2_6, input_unaged_counter_2_6); // core 2: ready

    // Core 1: Mock last counter read (index 7) - inline setup without expect_function_call to __wrap_dvfs_c2_pcm_enable_aging_sensor_measurement
    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 1); // MEASUREMENT_COMPLETE
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, input_unaged_counter_1_7);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, input_aged_counter_1_7);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 0); // success

    // Core 2: Mock last counter read (index 7) - inline setup without expect_function_call to __wrap_dvfs_c2_pcm_enable_aging_sensor_measurement
    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 1); // MEASUREMENT_COMPLETE
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, input_unaged_counter_2_7);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, input_aged_counter_2_7);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 0); // success

    setup_mock_aging_counter_not_ready(); // core 3: not ready

    // Trigger data processing for last counter
    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    // Create package
    pwr_core_record_aging_t aging_record = {{0}};
    package_create_pwr_core_aging_record(&aging_record);
    pwr_core_element_aging_t* aging_elements_1 = &aging_record.aging_collection[test_core_id_1].aging_element[0];
    pwr_core_element_aging_t* aging_elements_2 = &aging_record.aging_collection[test_core_id_2].aging_element[0];

    // ===== EXPECTED VALUES (after package creation) =====
    // Expected behavior after reading last counter pair (index 7):
    // - measurement_index increments to 8 (one past the last valid index)
    // - measurement_armed becomes false (disarmed after last pair read)

    // Core 1 expected values
    const uint8_t expected_measurement_index_1 = 8; // Incremented past last valid index (0-7)
    const bool expected_measurement_armed_1 = false;
    const uint8_t expected_counter_id_1_6 = 6;
    const uint32_t expected_aged_counter_1_6 = input_aged_counter_1_6;
    const uint32_t expected_unaged_counter_1_6 = input_unaged_counter_1_6;
    const uint8_t expected_counter_id_1_7 = 7;
    const uint32_t expected_aged_counter_1_7 = input_aged_counter_1_7;
    const uint32_t expected_unaged_counter_1_7 = input_unaged_counter_1_7;

    // Core 2 expected values
    const uint8_t expected_measurement_index_2 = 8; // Incremented past last valid index (0-7)
    const bool expected_measurement_armed_2 = false;
    const uint8_t expected_counter_id_2_6 = 6;
    const uint32_t expected_aged_counter_2_6 = input_aged_counter_2_6;
    const uint32_t expected_unaged_counter_2_6 = input_unaged_counter_2_6;
    const uint8_t expected_counter_id_2_7 = 7;
    const uint32_t expected_aged_counter_2_7 = input_aged_counter_2_7;
    const uint32_t expected_unaged_counter_2_7 = input_unaged_counter_2_7;

    // ===== PRINT ACTUAL VS EXPECTED =====
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON ===\n");

        // Core 1 results
        printf("Core %d Counter Pair 6 Package Data:\n", test_core_id_1);
        printf("  Counter ID     - Actual: %d, Expected: %d\n", aging_elements_1[6].counter_id, expected_counter_id_1_6);
        printf("  Aged Counter   - Actual: %u, Expected: %u\n", aging_elements_1[6].aged_counter, expected_aged_counter_1_6);
        printf("  Unaged Counter - Actual: %u, Expected: %u\n", aging_elements_1[6].unaged_counter, expected_unaged_counter_1_6);

        printf("\nCore %d Counter Pair 7 (Last) Package Data:\n", test_core_id_1);
        printf("  Counter ID     - Actual: %d, Expected: %d\n", aging_elements_1[7].counter_id, expected_counter_id_1_7);
        printf("  Aged Counter   - Actual: %u, Expected: %u\n", aging_elements_1[7].aged_counter, expected_aged_counter_1_7);
        printf("  Unaged Counter - Actual: %u, Expected: %u\n", aging_elements_1[7].unaged_counter, expected_unaged_counter_1_7);

        printf("\nCore %d Final State:\n", test_core_id_1);
        printf("  measurement_index - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_1].measurement_index,
               expected_measurement_index_1);
        printf("  measurement_armed - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_1].measurement_armed,
               expected_measurement_armed_1);

        // Core 2 results
        printf("\nCore %d Counter Pair 6 Package Data:\n", test_core_id_2);
        printf("  Counter ID     - Actual: %d, Expected: %d\n", aging_elements_2[6].counter_id, expected_counter_id_2_6);
        printf("  Aged Counter   - Actual: %u, Expected: %u\n", aging_elements_2[6].aged_counter, expected_aged_counter_2_6);
        printf("  Unaged Counter - Actual: %u, Expected: %u\n", aging_elements_2[6].unaged_counter, expected_unaged_counter_2_6);

        printf("\nCore %d Counter Pair 7 (Last) Package Data:\n", test_core_id_2);
        printf("  Counter ID     - Actual: %d, Expected: %d\n", aging_elements_2[7].counter_id, expected_counter_id_2_7);
        printf("  Aged Counter   - Actual: %u, Expected: %u\n", aging_elements_2[7].aged_counter, expected_aged_counter_2_7);
        printf("  Unaged Counter - Actual: %u, Expected: %u\n", aging_elements_2[7].unaged_counter, expected_unaged_counter_2_7);

        printf("\nCore %d Final State:\n", test_core_id_2);
        printf("  measurement_index - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_2].measurement_index,
               expected_measurement_index_2);
        printf("  measurement_armed - Actual: %d, Expected: %d\n",
               core_aging[test_core_id_2].measurement_armed,
               expected_measurement_armed_2);
    }

    // ===== ASSERT ACTUAL VS EXPECTED =====
    // Verify core 1 measurement_index incremented past last valid index and measurement_armed state
    assert_int_equal(core_aging[test_core_id_1].measurement_index, expected_measurement_index_1);
    assert_int_equal(core_aging[test_core_id_1].measurement_armed, expected_measurement_armed_1);

    // Verify core 1 counter pair 6 and 7 data
    assert_int_equal(aging_elements_1[6].counter_id, expected_counter_id_1_6);
    assert_int_equal(aging_elements_1[6].aged_counter, expected_aged_counter_1_6);
    assert_int_equal(aging_elements_1[6].unaged_counter, expected_unaged_counter_1_6);
    assert_int_equal(aging_elements_1[7].counter_id, expected_counter_id_1_7);
    assert_int_equal(aging_elements_1[7].aged_counter, expected_aged_counter_1_7);
    assert_int_equal(aging_elements_1[7].unaged_counter, expected_unaged_counter_1_7);

    // Verify core 2 measurement_index incremented past last valid index and measurement_armed state
    assert_int_equal(core_aging[test_core_id_2].measurement_index, expected_measurement_index_2);
    assert_int_equal(core_aging[test_core_id_2].measurement_armed, expected_measurement_armed_2);

    // Verify core 2 counter pair 6 and 7 data
    assert_int_equal(aging_elements_2[6].counter_id, expected_counter_id_2_6);
    assert_int_equal(aging_elements_2[6].aged_counter, expected_aged_counter_2_6);
    assert_int_equal(aging_elements_2[6].unaged_counter, expected_unaged_counter_2_6);
    assert_int_equal(aging_elements_2[7].counter_id, expected_counter_id_2_7);
    assert_int_equal(aging_elements_2[7].aged_counter, expected_aged_counter_2_7);
    assert_int_equal(aging_elements_2[7].unaged_counter, expected_unaged_counter_2_7);

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_aging_counter_last_pair_boundary ---\n");
        printf("***\n");
    }
}

// Test all cores aging counter measurement completion
// Validates that all cores can process aging counters simultaneously
//
// Test Flow Summary:
// ==================
// 1. Initialize: All cores with measurement_index=0, measurement_armed=true
// 2. Mock: All cores have aging counter measurements ready (different values per core)
// 3. Trigger: data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()
// 4. Verify: All cores' measurement_index incremented to 1
// 5. Create package: package_create_pwr_core_aging_record()
// 6. Assert: All cores' package data matches expected values
TEST_FUNCTION(test_all_cores_aging_counter_complete, test_setup, test_teardown)
{
    // ===== INPUT VALUES =====
    // Define unique aged/unaged counter values for each core
    const uint32_t input_aged_counters[NUMBER_OF_CORES_TO_TEST] = {11111, 22222, 33333, 44444};
    const uint32_t input_unaged_counters[NUMBER_OF_CORES_TO_TEST] = {10000, 20000, 30000, 40000};
    const uint64_t input_timestamp_uS = 5000000;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_all_cores_aging_counter_complete ---\n");
        printf("=== ALL CORES AGING COUNTER TEST ===\n");
        printf("Testing all %d cores with unique aging counter values\n", NUMBER_OF_CORES_TO_TEST);
    }

    // Setup mocks for data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg() infrastructure calls
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg);
    will_return(__wrap_die_2_die_exch_get_this_die_id, 0); // PRIMARY_DIE_ID
    expect_function_call(__wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg);

    // Setup mock aging counter register operations for all cores
    // All cores have measurements ready with unique values
    // All cores start at measurement_index=0 (from test_setup), so use normal ready helper
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        setup_mock_aging_counter_ready(core_id, input_aged_counters[core_id], input_unaged_counters[core_id]);
    }

    // Set up timestamp for aging counter processing
    time_t0 = input_timestamp_uS - 1000; // Will return input_timestamp_uS when called

    if (g_print_logs)
        printf("Pre-processing time_t0: %u us, expected return: %" PRIu64 " us\n", time_t0, input_timestamp_uS);

    // Trigger complete power package data preparation
    if (g_print_logs)
        printf("Calling data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg()...\n");

    data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg();

    // Create aging counter package and verify results
    pwr_core_record_aging_t aging_record = {{0}};
    uint32_t record_size = package_create_pwr_core_aging_record(&aging_record);

    if (g_print_logs)
        printf("Package created successfully, size: %u bytes\n", record_size);

    // ===== EXPECTED VALUES (after package creation) =====
    const uint8_t expected_counter_id = 0;        // First counter pair (measurement_index starts at 0)
    const uint8_t expected_measurement_index = 1; // Should increment after successful read
    const uint64_t expected_timestamp_uS = input_timestamp_uS;
    const uint32_t expected_voltage_mV = 1000;
    const uint32_t expected_temperature_dC = 850;

    // ===== PRINT ACTUAL VS EXPECTED =====
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON ===\n");
        for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
        {
            pwr_core_element_aging_t* aging_elements = &aging_record.aging_collection[core_id].aging_element[0];

            printf("\nCore %d Aging Counter State:\n", core_id);
            printf("  measurement_index - Actual: %d, Expected: %d\n", core_aging[core_id].measurement_index, expected_measurement_index);
            printf("Core %d Package Data:\n", core_id);
            printf("  Counter ID       - Actual: %d, Expected: %d\n", aging_elements[0].counter_id, expected_counter_id);
            printf("  Aged Counter     - Actual: %u, Expected: %u\n",
                   aging_elements[0].aged_counter,
                   input_aged_counters[core_id]);
            printf("  Unaged Counter   - Actual: %u, Expected: %u\n",
                   aging_elements[0].unaged_counter,
                   input_unaged_counters[core_id]);
            printf("  Timestamp (us)   - Actual: %" PRIu64 ", Expected: %" PRIu64 "\n",
                   aging_elements[0].timestamp_uS,
                   expected_timestamp_uS);
            printf("  Voltage (mV)     - Actual: %u, Expected: %u\n", aging_elements[0].voltage_mV, expected_voltage_mV);
            printf("  Temperature (dC) - Actual: %u, Expected: %u\n", aging_elements[0].temperature_dC, expected_temperature_dC);
        }
    }

    // ===== ASSERT ACTUAL VS EXPECTED =====
    // Verify all cores' aging counter state and package data
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        pwr_core_element_aging_t* aging_elements = &aging_record.aging_collection[core_id].aging_element[0];

        // Verify aging counter state updated
        assert_int_equal(core_aging[core_id].measurement_index, expected_measurement_index);

        // Verify package data matches expected values
        assert_int_equal(aging_elements[0].counter_id, expected_counter_id);
        assert_int_equal(aging_elements[0].aged_counter, input_aged_counters[core_id]);
        assert_int_equal(aging_elements[0].unaged_counter, input_unaged_counters[core_id]);
        assert_int_equal(aging_elements[0].timestamp_uS, expected_timestamp_uS);
        assert_int_equal(aging_elements[0].voltage_mV, expected_voltage_mV);
        assert_int_equal(aging_elements[0].temperature_dC, expected_temperature_dC);
    }

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_all_cores_aging_counter_complete ---\n");
        printf("***\n");
    }
}
