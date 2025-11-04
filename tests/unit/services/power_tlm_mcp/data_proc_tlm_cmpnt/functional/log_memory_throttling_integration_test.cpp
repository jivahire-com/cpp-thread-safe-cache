//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file log_memory_throttling_integration_test.cpp
 * Integration test for memory throttling (DIMM info for DDR manager)
 *
 * Test Objectives:
 * ================
 * - Validate memory throttling data collection from DIMM sensor FIFO
 * - Verify throttle entry counts, duration, and source tracking per DIMM
 * - Confirm package creation for memory throttle records
 * - Test all 6 DIMMs with independent throttling data
 * - Validate DIMM power accumulation across all DIMMs
 *
 * Test Flow:
 * ==========
 * 1. Create dataset of DIMM sensor inputs with throttling information
 * 2. Call data_smpl_process_dimm_sensor_fifo() to process sensor data
 * 3. Call package_create_pwr_soc_memory_throttle_record() to create package
 * 4. Verify throttle records match expected values (entry_counts, duration_mS, throttle_source)
 *
 * Test Functions:
 * ===============
 * 1. test_single_dimm_throttling: Test single DIMM with throttling data
 * 2. test_all_dimms_throttling: Test all 6 DIMMs with independent throttling data
 * 3. test_dimm_multiple_throttle_events: Test accumulation of multiple throttle events for same DIMM
 * 4. test_dimm_no_throttling: Test DIMMs with no throttling activity
 * 5. test_dimm_total_power_accumulation: Test DIMM power accumulation across all DIMMs
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779235

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// Global variable to control debug printing
static bool g_print_logs = true;

// Helper Functions

// Setup mock DIMM sensor data structure with throttling parameters and and setup mocks
static void setup_mock_dimm_throttle_data(sensor_ram_dimm_info_t* dimm_data, // Pointer to sensor_ram_dimm_info_t structure to fill
                                          uint8_t dimm_id,       // DIMM ID (0-5)
                                          uint64_t timestamp_uS, // Timestamp in microseconds
                                          uint16_t temp_s0_dC,   // Temperature sensor 0 in deciCelsius
                                          uint16_t temp_s1_dC,   // Temperature sensor 1 in deciCelsius
                                          uint16_t power_mW,     // Power in milliwatts
                                          uint16_t entry_count,  // Number of throttle events
                                          uint8_t duration_ms,   // Throttle duration in milliseconds
                                          uint8_t throttle_source, // Throttle source (0=none, 1=thermal, 2=power, etc.)
                                          uint8_t frequency_id, // Memory frequency ID
                                          bool more_entries) // True if more FIFO entries follow, false if this is the last entry
{
    // Fill in the DIMM sensor data structure
    dimm_data->timestamp = timestamp_uS;
    dimm_data->dimm_temp_s0_dC = temp_s0_dC;
    dimm_data->dimm_temp_s1_dC = temp_s1_dC;
    dimm_data->dimm_power_mW = power_mW;
    dimm_data->dimm_throttle_count = entry_count;
    dimm_data->dimm_id = dimm_id;
    dimm_data->dimm_throttling = throttle_source;
    dimm_data->dimm_memory_frequency_id = frequency_id;
    dimm_data->dimm_throttle_duration_ms = duration_ms;

    // Setup mocks for sensor FIFO polling
    // Order: curr_data_is_valid, more_entries, data pointer
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true);         // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, more_entries); // more_entries
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, dimm_data);    // data pointer

    if (g_print_logs)
    {
        printf("  DIMM %d: entries=%d, duration=%d ms, source=%d, temp_s0=%d dC, power=%d mW\n",
               dimm_id,
               entry_count,
               duration_ms,
               throttle_source,
               temp_s0_dC,
               power_mW);
    }
}

// Setup function - called before each test
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    // Enable in-band publishing for telemetry data processing so that comp_metrics_for_single_soc_dimm can store data
    in_band_publishing_active = true;
    return 0;
}

// Teardown function - called after each test
static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// Test function to verify single DIMM throttling data collection and package creation
// Validates basic throttle entry count, duration, and source tracking
//
// Test Flow Summary:
// ==================
// 1. Initialize: Create DIMM sensor data with throttling information
// 2. Mock: DIMM sensor FIFO with single DIMM entry
// 3. Trigger: data_smpl_process_dimm_sensor_fifo()
// 4. Create package: package_create_pwr_soc_memory_throttle_record()
// 5. Assert: Package data matches expected throttle values
TEST_FUNCTION(test_single_dimm_throttling, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_single_dimm_throttling ---\n");
        printf("=== SINGLE DIMM MEMORY THROTTLING TEST ===\n");
    }

    // ===== INPUT VALUES =====
    const uint8_t test_dimm_id = 0;
    const uint16_t input_entry_count = 5;     // Number of throttle events
    const uint8_t input_duration_ms = 100;    // Total throttle duration in milliseconds
    const uint8_t input_throttle_source = 1;  // Throttle source (e.g., thermal)
    const uint64_t input_timestamp_uS = 1000; // Timestamp in microseconds
    const uint16_t input_temp_s0_dC = 85;     // Temperature sensor 0 in deciCelsius
    const uint16_t input_temp_s1_dC = 82;     // Temperature sensor 1 in deciCelsius
    const uint16_t input_power_mW = 1500;     // Power in milliwatts
    const uint8_t input_frequency_id = 3;     // Memory frequency ID

    // Create and setup mock DIMM sensor data
    sensor_ram_dimm_info_t dimm_data;
    setup_mock_dimm_throttle_data(&dimm_data,
                                  test_dimm_id,
                                  input_timestamp_uS,
                                  input_temp_s0_dC,
                                  input_temp_s1_dC,
                                  input_power_mW,
                                  input_entry_count,
                                  input_duration_ms,
                                  input_throttle_source,
                                  input_frequency_id,
                                  false); // more_entries = false (last entry)

    // Process DIMM sensor FIFO
    if (g_print_logs)
        printf("\nProcessing DIMM sensor FIFO...\n");

    bool result = data_smpl_process_dimm_sensor_fifo();
    assert_true(result);

    // Create memory throttle package
    pwr_soc_record_memory_throttle_t throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_soc_memory_throttle_record(&throttle_record);

    if (g_print_logs)
    {
        printf("\nPackage created successfully, size: %u bytes\n", record_size);
        printf("\n=== RESULTS COMPARISON ===\n");
    }

    // Get actual values from package
    uint32_t actual_entry_counts =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.entry_counts;
    uint32_t actual_duration_ms =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.total_duration_mS;
    uint8_t actual_throttle_source =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.throttle_source;
    uint8_t actual_dimm_id = throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.dimm_id;

    // ===== EXPECTED VALUES =====
    // Source code logic (comp_metrics_for_single_soc_dimm in compute_metrics.c):
    // - entry_counts: Accumulated from dimm_throttle_count (single entry -> input_entry_count)
    // - total_duration_mS: Accumulated from dimm_throttle_duration_ms (single entry -> input_duration_ms)
    // - throttle_source: Latest dimm_throttling value (single entry -> input_throttle_source)
    // - dimm_id: DIMM identifier from sensor data (single entry -> test_dimm_id)
    const uint32_t expected_entry_counts = input_entry_count;
    const uint32_t expected_duration_ms = input_duration_ms;
    const uint8_t expected_throttle_source = input_throttle_source;
    const uint8_t expected_dimm_id = test_dimm_id;

    // Print actual vs expected
    if (g_print_logs)
    {
        printf("DIMM %d Memory Throttle Data:\n", test_dimm_id);
        printf("  entry_counts:      actual=%u, expected=%u\n", actual_entry_counts, expected_entry_counts);
        printf("  duration_mS:       actual=%u, expected=%u\n", actual_duration_ms, expected_duration_ms);
        printf("  throttle_source:   actual=%u, expected=%u\n", actual_throttle_source, expected_throttle_source);
        printf("  dimm_id:           actual=%u, expected=%u\n", actual_dimm_id, expected_dimm_id);
    }

    // Assert actual vs expected
    assert_int_equal(expected_entry_counts, actual_entry_counts);
    assert_int_equal(expected_duration_ms, actual_duration_ms);
    assert_int_equal(expected_throttle_source, actual_throttle_source);
    assert_int_equal(expected_dimm_id, actual_dimm_id);

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_single_dimm_throttling ---\n");
        printf("***\n");
    }
}

// Test all 6 DIMMs with independent throttling data
// Validates that each DIMM tracks throttling independently
//
// Test Flow Summary:
// ==================
// 1. Initialize: Create DIMM sensor data for all 6 DIMMs with unique throttling values
// 2. Mock: DIMM sensor FIFO with 6 DIMM entries
// 3. Trigger: data_smpl_process_dimm_sensor_fifo()
// 4. Create package: package_create_pwr_soc_memory_throttle_record()
// 5. Assert: Each DIMM's package data matches its expected throttle values
TEST_FUNCTION(test_all_dimms_throttling, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_all_dimms_throttling ---\n");
        printf("=== ALL DIMMS MEMORY THROTTLING TEST ===\n");
    }

    // ===== INPUT VALUES =====
    // Define unique throttle data for each DIMM
    const uint16_t input_entry_counts[NUMBER_OF_DIMMS_PER_DIE] = {3, 5, 7, 2, 10, 15};
    const uint8_t input_durations_ms[NUMBER_OF_DIMMS_PER_DIE] = {50, 75, 100, 25, 150, 200};
    const uint8_t input_throttle_sources[NUMBER_OF_DIMMS_PER_DIE] = {0, 1, 2, 0, 1, 2};

    if (g_print_logs)
    {
        printf("Testing all %d DIMMs with unique throttle values:\n", NUMBER_OF_DIMMS_PER_DIE);
        for (uint8_t i = 0; i < NUMBER_OF_DIMMS_PER_DIE; i++)
        {
            printf("  DIMM %d: entries=%d, duration=%d ms, source=%d\n",
                   i,
                   input_entry_counts[i],
                   input_durations_ms[i],
                   input_throttle_sources[i]);
        }
    }

    // Create DIMM sensor data for all DIMMs
    // Note: Must be declared outside loop so pointers remain valid
    sensor_ram_dimm_info_t dimm_data[NUMBER_OF_DIMMS_PER_DIE];

    for (uint8_t dimm_id = 0; dimm_id < NUMBER_OF_DIMMS_PER_DIE; dimm_id++)
    {
        bool more_entries = (dimm_id < NUMBER_OF_DIMMS_PER_DIE - 1); // Last entry has more_entries = false

        setup_mock_dimm_throttle_data(&dimm_data[dimm_id],
                                      dimm_id,
                                      static_cast<uint64_t>(1000 + (dimm_id * 100)), // timestamp
                                      static_cast<uint16_t>(70 + dimm_id * 2),       // temp_s0
                                      static_cast<uint16_t>(68 + dimm_id * 2),       // temp_s1
                                      static_cast<uint16_t>(1000 + dimm_id * 100),   // power
                                      input_entry_counts[dimm_id],
                                      input_durations_ms[dimm_id],
                                      input_throttle_sources[dimm_id],
                                      static_cast<uint8_t>(dimm_id % 4), // frequency_id
                                      more_entries);
    }

    // Process DIMM sensor FIFO
    if (g_print_logs)
        printf("\nProcessing DIMM sensor FIFO for all DIMMs...\n");

    bool result = data_smpl_process_dimm_sensor_fifo();
    assert_true(result);

    // Create memory throttle package
    pwr_soc_record_memory_throttle_t throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_soc_memory_throttle_record(&throttle_record);

    if (g_print_logs)
    {
        printf("\nPackage created successfully, size: %u bytes\n", record_size);
        printf("\n=== RESULTS COMPARISON ===\n");
    }

    // ===== EXPECTED VALUES =====
    // Source code logic (comp_metrics_for_single_soc_dimm in compute_metrics.c):
    // - Each DIMM tracks metrics independently
    // - entry_counts: Accumulated from each DIMM's dimm_throttle_count
    // - total_duration_mS: Accumulated from each DIMM's dimm_throttle_duration_ms
    // - throttle_source: Latest dimm_throttling value for each DIMM
    // - dimm_id: DIMM identifier from sensor data
    // Since we process one entry per DIMM:
    // - expected_entry_counts[i] = input_entry_counts[i]
    // - expected_duration_ms[i] = input_durations_ms[i]
    // - expected_throttle_source[i] = input_throttle_sources[i]
    // - expected_dimm_id[i] = i (loop index)

    // Verify all DIMMs
    for (uint8_t dimm_id = 0; dimm_id < NUMBER_OF_DIMMS_PER_DIE; dimm_id++)
    {
        uint32_t expected_entry_counts = input_entry_counts[dimm_id];
        uint32_t expected_duration_ms = input_durations_ms[dimm_id];
        uint8_t expected_throttle_source = input_throttle_sources[dimm_id];
        uint8_t expected_dimm_id = dimm_id;

        uint32_t actual_entry_counts =
            throttle_record.memory_throttle_collection[dimm_id].memory_throttle_element.entry_counts;
        uint32_t actual_duration_ms =
            throttle_record.memory_throttle_collection[dimm_id].memory_throttle_element.total_duration_mS;
        uint8_t actual_throttle_source =
            throttle_record.memory_throttle_collection[dimm_id].memory_throttle_element.throttle_source;
        uint8_t actual_dimm_id = throttle_record.memory_throttle_collection[dimm_id].memory_throttle_element.dimm_id;

        if (g_print_logs)
        {
            printf("DIMM %d:\n", dimm_id);
            printf("  entry_counts:      actual=%u, expected=%u\n", actual_entry_counts, expected_entry_counts);
            printf("  duration_mS:       actual=%u, expected=%u\n", actual_duration_ms, expected_duration_ms);
            printf("  throttle_source:   actual=%u, expected=%u\n", actual_throttle_source, expected_throttle_source);
            printf("  dimm_id:           actual=%u, expected=%u\n", actual_dimm_id, expected_dimm_id);
        }

        assert_int_equal(expected_entry_counts, actual_entry_counts);
        assert_int_equal(expected_duration_ms, actual_duration_ms);
        assert_int_equal(expected_throttle_source, actual_throttle_source);
        assert_int_equal(expected_dimm_id, actual_dimm_id);
    }

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_all_dimms_throttling ---\n");
        printf("***\n");
    }
}

// Test function to verify multiple throttle event accumulation for the same DIMM
// Validates that throttle metrics accumulate correctly over multiple FIFO entries
//
// Test Flow Summary:
// ==================
// 1. Initialize: Create multiple DIMM sensor entries for the same DIMM
// 2. Mock: DIMM sensor FIFO with multiple entries for same DIMM
// 3. Trigger: data_smpl_process_dimm_sensor_fifo() multiple times
// 4. Create package: package_create_pwr_soc_memory_throttle_record()
// 5. Assert: Package data shows accumulated throttle values
TEST_FUNCTION(test_dimm_multiple_throttle_events, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_dimm_multiple_throttle_events ---\n");
        printf("=== MULTIPLE THROTTLE EVENTS TEST ===\n");
    }

    // ===== INPUT VALUES =====
    const uint8_t test_dimm_id = 2;

    // Three separate throttle events for the same DIMM
    const uint16_t event1_entry_count = 3;
    const uint8_t event1_duration_ms = 50;
    const uint8_t event1_throttle_source = 1;
    const uint64_t event1_timestamp_uS = 1000;
    const uint16_t event1_temp_s0_dC = 80;
    const uint16_t event1_temp_s1_dC = 78;
    const uint16_t event1_power_mW = 1400;
    const uint8_t event1_frequency_id = 2;

    const uint16_t event2_entry_count = 5;
    const uint8_t event2_duration_ms = 75;
    const uint8_t event2_throttle_source = 1;
    const uint64_t event2_timestamp_uS = 2000;
    const uint16_t event2_temp_s0_dC = 82;
    const uint16_t event2_temp_s1_dC = 80;
    const uint16_t event2_power_mW = 1450;
    const uint8_t event2_frequency_id = 2;

    const uint16_t event3_entry_count = 2;
    const uint8_t event3_duration_ms = 25;
    const uint8_t event3_throttle_source = 2; // Different source (last one is considered)
    const uint64_t event3_timestamp_uS = 3000;
    const uint16_t event3_temp_s0_dC = 84;
    const uint16_t event3_temp_s1_dC = 82;
    const uint16_t event3_power_mW = 1500;
    const uint8_t event3_frequency_id = 2;

    if (g_print_logs)
    {
        printf("DIMM %d - Testing accumulation of 3 throttle events:\n", test_dimm_id);
        printf("  Event 1: entries=%d, duration=%d ms, source=%d\n", event1_entry_count, event1_duration_ms, event1_throttle_source);
        printf("  Event 2: entries=%d, duration=%d ms, source=%d\n", event2_entry_count, event2_duration_ms, event2_throttle_source);
        printf("  Event 3: entries=%d, duration=%d ms, source=%d\n", event3_entry_count, event3_duration_ms, event3_throttle_source);
    }

    // Event 1
    sensor_ram_dimm_info_t dimm_data_1;
    setup_mock_dimm_throttle_data(&dimm_data_1,
                                  test_dimm_id,
                                  event1_timestamp_uS,
                                  event1_temp_s0_dC,
                                  event1_temp_s1_dC,
                                  event1_power_mW,
                                  event1_entry_count,
                                  event1_duration_ms,
                                  event1_throttle_source,
                                  event1_frequency_id,
                                  false); // more_entries
    data_smpl_process_dimm_sensor_fifo();

    // Event 2
    sensor_ram_dimm_info_t dimm_data_2;
    setup_mock_dimm_throttle_data(&dimm_data_2,
                                  test_dimm_id,
                                  event2_timestamp_uS,
                                  event2_temp_s0_dC,
                                  event2_temp_s1_dC,
                                  event2_power_mW,
                                  event2_entry_count,
                                  event2_duration_ms,
                                  event2_throttle_source,
                                  event2_frequency_id,
                                  false); // more_entries
    data_smpl_process_dimm_sensor_fifo();

    // Event 3
    sensor_ram_dimm_info_t dimm_data_3;
    setup_mock_dimm_throttle_data(&dimm_data_3,
                                  test_dimm_id,
                                  event3_timestamp_uS,
                                  event3_temp_s0_dC,
                                  event3_temp_s1_dC,
                                  event3_power_mW,
                                  event3_entry_count,
                                  event3_duration_ms,
                                  event3_throttle_source,
                                  event3_frequency_id,
                                  false); // more_entries
    data_smpl_process_dimm_sensor_fifo();

    // Create memory throttle package
    pwr_soc_record_memory_throttle_t throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_soc_memory_throttle_record(&throttle_record);

    if (g_print_logs)
        printf("\nPackage created successfully, size: %u bytes\n", record_size);

    // ===== EXPECTED VALUES =====
    // - entry_counts: Accumulated across all events -> sum(dimm_throttle_count)
    //   Calculation: 3 + 5 + 2 = 10
    // - total_duration_mS: Accumulated across all events -> sum(dimm_throttle_duration_ms)
    //   Calculation: 50 + 75 + 25 = 150 ms
    // - throttle_source: Latest dimm_throttling value (not accumulated, last value is considered)
    //   Calculation: event3_throttle_source = 2
    // - dimm_id: DIMM identifier (same for all events -> test_dimm_id)
    const uint32_t expected_entry_counts = event1_entry_count + event2_entry_count + event3_entry_count;
    const uint32_t expected_duration_ms = event1_duration_ms + event2_duration_ms + event3_duration_ms;
    const uint8_t expected_throttle_source = event3_throttle_source; // Last event's source
    const uint8_t expected_dimm_id = test_dimm_id;

    // Get actual values from package
    uint32_t actual_entry_counts =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.entry_counts;
    uint32_t actual_duration_ms =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.total_duration_mS;
    uint8_t actual_throttle_source =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.throttle_source;
    uint8_t actual_dimm_id = throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.dimm_id;

    // Print actual vs expected
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON (Accumulated from 3 events) ===\n");
        printf("DIMM %d Memory Throttle Data:\n", test_dimm_id);
        printf("  entry_counts:      actual=%u, expected=%u\n", actual_entry_counts, expected_entry_counts);
        printf("  duration_mS:       actual=%u, expected=%u\n", actual_duration_ms, expected_duration_ms);
        printf("  throttle_source:   actual=%u, expected=%u (latest)\n", actual_throttle_source, expected_throttle_source);
        printf("  dimm_id:           actual=%u, expected=%u\n", actual_dimm_id, expected_dimm_id);
    }

    // Assert actual vs expected
    assert_int_equal(expected_entry_counts, actual_entry_counts);
    assert_int_equal(expected_duration_ms, actual_duration_ms);
    assert_int_equal(expected_throttle_source, actual_throttle_source);
    assert_int_equal(expected_dimm_id, actual_dimm_id);

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("--- END test_dimm_multiple_throttle_events ---\n");
        printf("***\n");
    }
}

// Test function to verify DIMMs with no throttling activity
// Validates that DIMMs report zero throttling when no throttle events occur
//
// Test Flow Summary:
// ==================
// 1. Initialize: Create DIMM sensor data with no throttling (counts=0, duration=0)
// 2. Mock: DIMM sensor FIFO with DIMM entries showing no throttling
// 3. Trigger: data_smpl_process_dimm_sensor_fifo()
// 4. Create package: package_create_pwr_soc_memory_throttle_record()
// 5. Assert: Package data shows zero throttle values
TEST_FUNCTION(test_dimm_no_throttling, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_dimm_no_throttling ---\n");
        printf("=== NO THROTTLING TEST ===\n");
    }

    // ===== INPUT VALUES =====
    const uint8_t test_dimm_id = 4;
    const uint16_t input_entry_count = 0;     // No throttle events
    const uint8_t input_duration_ms = 0;      // No throttle duration
    const uint8_t input_throttle_source = 0;  // No active throttle source
    const uint64_t input_timestamp_uS = 1000; // Timestamp in microseconds
    const uint16_t input_temp_s0_dC = 65;     // Temperature sensor 0 in deciCelsius
    const uint16_t input_temp_s1_dC = 63;     // Temperature sensor 1 in deciCelsius
    const uint16_t input_power_mW = 1000;     // Power in milliwatts
    const uint8_t input_frequency_id = 1;     // Memory frequency ID

    // Create DIMM sensor data with no throttling
    sensor_ram_dimm_info_t dimm_data;
    setup_mock_dimm_throttle_data(&dimm_data,
                                  test_dimm_id,
                                  input_timestamp_uS,
                                  input_temp_s0_dC,
                                  input_temp_s1_dC,
                                  input_power_mW,
                                  input_entry_count,
                                  input_duration_ms,
                                  input_throttle_source,
                                  input_frequency_id,
                                  false); // more_entries

    // Process DIMM sensor FIFO
    if (g_print_logs)
        printf("\nProcessing DIMM sensor FIFO...\n");

    bool result = data_smpl_process_dimm_sensor_fifo();
    assert_true(result);

    // Create memory throttle package
    pwr_soc_record_memory_throttle_t throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_soc_memory_throttle_record(&throttle_record);

    if (g_print_logs)
        printf("Package created successfully, size: %u bytes\n", record_size);

    // ===== EXPECTED VALUES =====
    // When no throttling occurs (all input values are 0):
    // - entry_counts: No entries accumulated -> 0
    // - total_duration_mS: No duration accumulated -> 0
    // - throttle_source: No active source -> 0
    // - dimm_id: DIMM identifier from sensor data -> test_dimm_id
    const uint32_t expected_entry_counts = 0;
    const uint32_t expected_duration_ms = 0;
    const uint8_t expected_throttle_source = 0;
    const uint8_t expected_dimm_id = test_dimm_id;

    // Get actual values from package
    uint32_t actual_entry_counts =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.entry_counts;
    uint32_t actual_duration_ms =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.total_duration_mS;
    uint8_t actual_throttle_source =
        throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.throttle_source;
    uint8_t actual_dimm_id = throttle_record.memory_throttle_collection[test_dimm_id].memory_throttle_element.dimm_id;

    // Print actual vs expected
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON ===\n");
        printf("DIMM %d Memory Throttle Data (No Throttling):\n", test_dimm_id);
        printf("  entry_counts:      actual=%u, expected=%u\n", actual_entry_counts, expected_entry_counts);
        printf("  duration_mS:       actual=%u, expected=%u\n", actual_duration_ms, expected_duration_ms);
        printf("  throttle_source:   actual=%u, expected=%u\n", actual_throttle_source, expected_throttle_source);
        printf("  dimm_id:           actual=%u, expected=%u\n", actual_dimm_id, expected_dimm_id);
    }

    // Assert actual vs expected
    assert_int_equal(expected_entry_counts, actual_entry_counts);
    assert_int_equal(expected_duration_ms, actual_duration_ms);
    assert_int_equal(expected_throttle_source, actual_throttle_source);
    assert_int_equal(expected_dimm_id, actual_dimm_id);

    // ===== END FUNCTION =====
    if (g_print_logs)
    {
        printf("\n--- END test_dimm_no_throttling ---\n");
        printf("***\n");
    }
}

// Test function to verify DIMM power accumulation across all DIMMs
// Validates that individual DIMM power values are recorded correctly and total power is computed properly
//
// Test Flow Summary:
// ==================
// 1. Initialize: Create DIMM sensor data for all 6 DIMMs with power values
// 2. Mock: DIMM sensor FIFO with 6 DIMM entries
// 3. Trigger: data_smpl_process_dimm_sensor_fifo()
// 4. Create package: package_create_pwr_soc_dimm_power_record()
// 5. Assert: Each DIMM's power matches expected and total accumulates correctly
TEST_FUNCTION(test_dimm_total_power_accumulation, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_dimm_total_power_accumulation ---\n");
        printf("=== DIMM TOTAL POWER ACCUMULATION TEST ===\n");
    }

    // ===== INPUT VALUES =====
    const uint16_t dimm_power_per_dimm_mW = 5000; // 5W per DIMM

    if (g_print_logs)
    {
        printf("Testing DIMM power accumulation for all %u DIMMs:\n", NUMBER_OF_DIMMS_PER_DIE);
        printf("  Power per DIMM: %u mW\n", dimm_power_per_dimm_mW);
    }

    // Create DIMM sensor data for all DIMMs
    // Note: Must be declared outside loop so pointers remain valid
    sensor_ram_dimm_info_t dimm_data[NUMBER_OF_DIMMS_PER_DIE];

    for (uint8_t dimm_id = 0; dimm_id < NUMBER_OF_DIMMS_PER_DIE; dimm_id++)
    {
        bool more_entries = (dimm_id < NUMBER_OF_DIMMS_PER_DIE - 1); // Last entry has more_entries = false

        setup_mock_dimm_throttle_data(&dimm_data[dimm_id],
                                      dimm_id,
                                      static_cast<uint64_t>(1000 + dimm_id),   // timestamp
                                      static_cast<uint16_t>(70 + dimm_id * 2), // temp_s0
                                      static_cast<uint16_t>(68 + dimm_id * 2), // temp_s1
                                      dimm_power_per_dimm_mW,                  // power
                                      0,                                       // entry_count
                                      0,                                       // duration_ms
                                      0,                                       // throttle_source
                                      static_cast<uint8_t>(dimm_id % 4),       // frequency_id
                                      more_entries);
    }

    // Process DIMM sensor FIFO
    if (g_print_logs)
        printf("\nProcessing DIMM sensor FIFO for all %u DIMMs...\n", NUMBER_OF_DIMMS_PER_DIE);

    bool result = data_smpl_process_dimm_sensor_fifo();
    assert_true(result);

    // Create DIMM power package
    pwr_soc_record_dimm_power_t dimm_power_record = {{0}};
    uint32_t record_size = package_create_pwr_soc_dimm_power_record(&dimm_power_record);

    if (g_print_logs)
        printf("\nPackage created successfully, size: %u bytes\n", record_size);

    // ===== EXPECTED VALUES =====
    // - Each DIMM's power is stored in dimm_rt.latest_dimm[dimm_id].power_mW
    // - Total power across all DIMMs = sum of all individual DIMM powers (aggregated)
    // - Expected total = 6 DIMMs x 5000 mW = 30,000 mW
    const uint32_t expected_total_power_mW = NUMBER_OF_DIMMS_PER_DIE * dimm_power_per_dimm_mW;

    // Verify each DIMM's individual power
    if (g_print_logs)
    {
        printf("\n=== RESULTS COMPARISON ===\n");
        printf("Individual DIMM Power Values:\n");
    }

    uint32_t actual_total_power_mW = 0;
    for (uint8_t dimm_id = 0; dimm_id < NUMBER_OF_DIMMS_PER_DIE; dimm_id++)
    {
        uint16_t actual_dimm_power_mW = dimm_power_record.dimm_collection[dimm_id].dimm_element.power_mW.max_mW;
        uint16_t expected_dimm_power_mW = dimm_power_per_dimm_mW;

        actual_total_power_mW += actual_dimm_power_mW;

        if (g_print_logs)
        {
            printf("  DIMM %u: actual=%u, expected=%u\n", dimm_id, actual_dimm_power_mW, expected_dimm_power_mW);
        }

        assert_int_equal(expected_dimm_power_mW, actual_dimm_power_mW);
    }

    // Verify total power accumulation
    if (g_print_logs)
    {
        printf("\nTotal DIMM power -> actual=%u, expected=%u\n", actual_total_power_mW, expected_total_power_mW);
    }

    assert_int_equal(expected_total_power_mW, actual_total_power_mW);

    // END FUNCTION
    if (g_print_logs)
    {
        printf("\n--- END test_dimm_total_power_accumulation ---\n");
        printf("***\n");
    }
}
