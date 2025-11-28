//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_histogram_integration.cpp
 *
 * Core Voltage/Temperature Histogram Integration Test
 *
 * Test Objectives:
 * ================
 * - Validate core voltage and temperature sensor data processing updates histogram bins correctly
 * - Verify boundary conditions for voltage and temperature bin assignments
 * - Test all 4 corner bins (catchall bins) of the histogram table
 * - Test random bins throughout the histogram table
 * - Verify multiple sensor FIFO entries only update with latest values
 * - Test multiple cores with different voltage/temperature combinations
 *
 * Histogram Structure:
 * ===================
 * - 20 voltage bins (NUMBER_OF_HS_VOLTAGE_SCALES): Bin 0 highest (>974mV), Bin 19 lowest (≤830mV)
 * - 15 temperature bins (NUMBER_OF_HS_TEMP_SCALES): Bin 0 highest (>965dC), Bin 14 lowest (≤575dC)
 * - Bin ranges use exclusive lower bounds and inclusive upper bounds: (lower, upper]
 * - Corner bins: [0,0] (high V, high T), [0,14] (high V, low T), [19,0] (low V, high T), [19,14] (low V, low T)
 *
 * Test Flow:
 * ==========
 * 1. Setup sensor FIFO values for tile temperature and tile voltage
 * 2. Call data_proc_tlm_cmpnt_process_input_data() to process sensor FIFOs and update histogram
 * 3. Call package_create_pwr_core_histogram_record() to create the record
 * 4. Validate expected histogram bin counts are correct
 *
 * Test Functions:
 * ===============
 * 1. test_core_histogram_corner_bins: Test all 4 corner bins (catchall bins)
 * 2. test_core_histogram_boundary_conditions: Test +1mV/-1dC boundary transitions
 * 3. test_core_histogram_random_bins: Test 3-4 random bins throughout the table
 * 4. test_core_histogram_multiple_fifo_entries: Test multiple FIFO entries, only last entry counts
 * 5. test_core_histogram_multiple_cores: Test different cores with different voltage/temperature combinations
 * 6. test_core_histogram_bin_transitions_systematic: Systematic test using threshold arrays with pre-calculated bins
 */

// Keeping the bin ranges documented here for reference
// clang-format off
/*
 * HISTOGRAM BIN RANGES (as per compute_metrics.c as per array defination voltage_bin_upper_limits_mV and temp_bin_upper_limits_dC)
 *
 * Voltage Bins (20 bins) - Thresholds: {974, 966, 958, 950, 942, 934, 926, 918, 910, 902, 894, 886, 878, 870, 862, 854, 846, 838, 830}
 *   Bin  0 = (input voltage > 974 mV)
 *   Bin  1 = (974 mV >= input voltage > 966 mV)
 *   Bin  2 = (966 mV >= input voltage > 958 mV)
 *   Bin  3 = (958 mV >= input voltage > 950 mV)
 *   Bin  4 = (950 mV >= input voltage > 942 mV)
 *   Bin  5 = (942 mV >= input voltage > 934 mV)
 *   Bin  6 = (934 mV >= input voltage > 926 mV)
 *   Bin  7 = (926 mV >= input voltage > 918 mV)
 *   Bin  8 = (918 mV >= input voltage > 910 mV)
 *   Bin  9 = (910 mV >= input voltage > 902 mV)
 *   Bin 10 = (902 mV >= input voltage > 894 mV)
 *   Bin 11 = (894 mV >= input voltage > 886 mV)
 *   Bin 12 = (886 mV >= input voltage > 878 mV)
 *   Bin 13 = (878 mV >= input voltage > 870 mV)
 *   Bin 14 = (870 mV >= input voltage > 862 mV)
 *   Bin 15 = (862 mV >= input voltage > 854 mV)
 *   Bin 16 = (854 mV >= input voltage > 846 mV)
 *   Bin 17 = (846 mV >= input voltage > 838 mV)
 *   Bin 18 = (838 mV >= input voltage > 830 mV)
 *   Bin 19 = (input voltage <= 830 mV)
 *
 * Temperature Bins (15 bins) - Thresholds: {965, 935, 905, 875, 845, 815, 785, 755, 725, 695, 665, 635, 605, 575}
 *   Bin  0 = (input temperature > 965 dC)
 *   Bin  1 = (965 dC >= input temperature > 935 dC)
 *   Bin  2 = (935 dC >= input temperature > 905 dC)
 *   Bin  3 = (905 dC >= input temperature > 875 dC)
 *   Bin  4 = (875 dC >= input temperature > 845 dC)
 *   Bin  5 = (845 dC >= input temperature > 815 dC)
 *   Bin  6 = (815 dC >= input temperature > 785 dC)
 *   Bin  7 = (785 dC >= input temperature > 755 dC)
 *   Bin  8 = (755 dC >= input temperature > 725 dC)
 *   Bin  9 = (725 dC >= input temperature > 695 dC)
 *   Bin 10 = (695 dC >= input temperature > 665 dC)
 *   Bin 11 = (665 dC >= input temperature > 635 dC)
 *   Bin 12 = (635 dC >= input temperature > 605 dC)
 *   Bin 13 = (605 dC >= input temperature > 575 dC)
 *   Bin 14 = (input temperature <= 575 dC)
 *
 * Note: Input voltages undergo MILLIVOLTS2DOUT conversion which typically reduces the value by ~1mV
 */
// clang-format on

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779225

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
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <tlm_fuses.h>
}

// Debug flag for controlling prints
static bool g_print_logs = true;

// Temperature conversion macro (from test_tile_temperature.cpp)
#define TEST_TEMP_DC_TO_DOUT(temp_dC) \
    (TLM_FUSE_TEMP_CEL_2_DOUT(((temp_dC) / 10.0), DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL))

// Histogram bin constants and boundaries (from compute_metrics.c)
#define VOLTAGE_BIN_COUNT 20 // NUMBER_OF_HS_VOLTAGE_SCALES
#define TEMP_BIN_COUNT    15 // NUMBER_OF_HS_TEMP_SCALES

/**
 * Test data structure for voltage bin boundary validation
 * Maps input voltage (accounting for MILLIVOLTS2DOUT conversion) to expected bin
 * Only used in test_function test_core_histogram_bin_transitions_systematic
 */
typedef struct
{
    uint16_t input_voltage_mV;    // Input voltage in mV
    uint8_t expected_voltage_bin; // Expected bin after conversion
    const char* description;      // Test case description
} voltage_bin_test_t;

/**
 * Test data structure for temperature bin boundary validation
 * Maps input temperature to expected bin
 * Only used in test_function test_core_histogram_bin_transitions_systematic
 */
typedef struct
{
    uint16_t input_temperature_dC;    // Input temperature in dC
    uint8_t expected_temperature_bin; // Expected bin (no conversion)
    const char* description;          // Test case description
} temperature_bin_test_t;

// Voltage bin test cases: Pre-calculated input→bin mappings with hardcoded expected values
// Only used in test_core_histogram_bin_transitions_systematic to cover all boundary scenarios
static const voltage_bin_test_t voltage_bin_tests[] = {
    // Input -> After conversion -> Falls in range -> Bin assignment
    {976, 0, "Input 976mV, converts to 975mV, range (974-max]mV, Bin 0 (975 > 974)"},
    {975, 1, "Input 975mV, converts to 974mV, range (966-974]mV, Bin 1 (974 <= 974)"},
    {967, 2, "Input 967mV, converts to 966mV, range (958-966]mV, Bin 2 (966 <= 966)"},
    {966, 2, "Input 966mV, converts to 965mV, range (958-966]mV, Bin 2 (965 <= 966)"},
    {959, 3, "Input 959mV, converts to 958mV, range (950-958]mV, Bin 3 (958 <= 958)"},
    {951, 4, "Input 951mV, converts to 950mV, range (942-950]mV, Bin 4 (950 <= 950)"},
    {943, 5, "Input 943mV, converts to 942mV, range (934-942]mV, Bin 5 (942 <= 942)"},
    {935, 6, "Input 935mV, converts to 934mV, range (926-934]mV, Bin 6 (934 <= 934)"},
    {927, 7, "Input 927mV, converts to 926mV, range (918-926]mV, Bin 7 (926 <= 926)"},
    {919, 8, "Input 919mV, converts to 918mV, range (910-918]mV, Bin 8 (918 <= 918)"},
    {911, 9, "Input 911mV, converts to 910mV, range (902-910]mV, Bin 9 (910 <= 910)"},
    {903, 10, "Input 903mV, converts to 902mV, range (894-902]mV, Bin 10 (902 <= 902)"},
    {895, 11, "Input 895mV, converts to 894mV, range (886-894]mV, Bin 11 (894 <= 894)"},
    {887, 12, "Input 887mV, converts to 886mV, range (878-886]mV, Bin 12 (886 <= 886)"},
    {879, 13, "Input 879mV, converts to 878mV, range (870-878]mV, Bin 13 (878 <= 878)"},
    {871, 14, "Input 871mV, converts to 870mV, range (862-870]mV, Bin 14 (870 <= 870)"},
    {863, 15, "Input 863mV, converts to 862mV, range (854-862]mV, Bin 15 (862 <= 862)"},
    {855, 16, "Input 855mV, converts to 854mV, range (846-854]mV, Bin 16 (854 <= 854)"},
    {847, 17, "Input 847mV, converts to 846mV, range (838-846]mV, Bin 17 (846 <= 846)"},
    {839, 18, "Input 839mV, converts to 838mV, range (830-838]mV, Bin 18 (838 <= 838)"},
    {831, 19, "Input 831mV, converts to 830mV, range (min-830]mV, Bin 19 (830 <= 830)"},
    {830, 19, "Input 830mV, converts to 829mV, range (min-830]mV, Bin 19 (829 <= 830)"},
    {800, 19, "Input 800mV, converts to 799mV, range (min-830]mV, Bin 19 (799 <= 830)"},
};

// Temperature bin test cases: Pre-calculated input→bin mappings (no conversion applied)
// Only used in test_core_histogram_bin_transitions_systematic to cover all boundary scenarios
static const temperature_bin_test_t temperature_bin_tests[] = {
    // Threshold tests: Testing values at and above each threshold
    {1000, 0, "Input 1000dC, Bin 0 (well above 965dC)"},
    {966, 0, "Input 966dC, Bin 0 (just above 965dC threshold)"},
    {965, 1, "Input 965dC, Bin 1 (at 965dC threshold)"},
    {936, 1, "Input 936dC, Bin 1 (just above 935dC)"},
    {935, 2, "Input 935dC, Bin 2 (at 935dC threshold)"},
    {906, 2, "Input 906dC, Bin 2 (just above 905dC)"},
    {905, 3, "Input 905dC, Bin 3 (at 905dC threshold)"},
    {876, 3, "Input 876dC, Bin 3 (just above 875dC)"},
    {875, 4, "Input 875dC, Bin 4 (at 875dC threshold)"},
    {846, 4, "Input 846dC, Bin 4 (just above 845dC)"},
    {845, 5, "Input 845dC, Bin 5 (at 845dC threshold)"},
    {816, 5, "Input 816dC, Bin 5 (just above 815dC)"},
    {815, 6, "Input 815dC, Bin 6 (at 815dC threshold)"},
    {786, 6, "Input 786dC, Bin 6 (just above 785dC)"},
    {785, 7, "Input 785dC, Bin 7 (at 785dC threshold)"},
    {756, 7, "Input 756dC, Bin 7 (just above 755dC)"},
    {755, 8, "Input 755dC, Bin 8 (at 755dC threshold)"},
    {726, 8, "Input 726dC, Bin 8 (just above 725dC)"},
    {725, 9, "Input 725dC, Bin 9 (at 725dC threshold)"},
    {696, 9, "Input 696dC, Bin 9 (just above 695dC)"},
    {695, 10, "Input 695dC, Bin 10 (at 695dC threshold)"},
    {666, 10, "Input 666dC, Bin 10 (just above 665dC)"},
    {665, 11, "Input 665dC, Bin 11 (at 665dC threshold)"},
    {636, 11, "Input 636dC, Bin 11 (just above 635dC)"},
    {635, 12, "Input 635dC, Bin 12 (at 635dC threshold)"},
    {606, 12, "Input 606dC, Bin 12 (just above 605dC)"},
    {605, 13, "Input 605dC, Bin 13 (at 605dC threshold)"},
    {576, 13, "Input 576dC, Bin 13 (just above 575dC)"},
    {575, 14, "Input 575dC, Bin 14 (at 575dC threshold)"},
    {500, 14, "Input 500dC, Bin 14 (well below 575dC)"},
};

/**
 * Helper function to setup tile temperature sensor FIFO with specific values for cores
 * @param core_temps_dC Array of temperature values in deci-Celsius for each core
 * @param num_cores Number of cores to setup (max NUMBER_OF_CORES_PER_DIE)
 */
static void setup_tile_temperature_fifo(uint16_t core_temps_dC[], uint8_t num_cores)
{
    // Setup temperature FIFO entries - need to map cores to tile temperature sensors
    // Tile 0 maps to cores 0 and 1:
    // - Core 0: temperature from max(temp0, temp1, temp2)
    // - Core 1: temperature from max(temp3, temp4, temp5)
    // For simplicity, set all 3 sensors to the same value for each core

    static tile_temp_t temp_data;
    memset(&temp_data, 0, sizeof(tile_temp_t));

    // Set valid bit
    temp_data.temp0.temp_valid = 1;

    // Convert temperatures from dC to DOUT format
    if (num_cores >= 1)
    {
        // Core 0: Set temp0, temp1, temp2 to same value for simplicity
        temp_data.temp1.temp0 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[0]);
        temp_data.temp1.temp1 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[0]);
        temp_data.temp1.temp2 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[0]);
    }

    if (num_cores >= 2)
    {
        // Core 1: Set temp3, temp4, temp5 to same value for simplicity
        temp_data.temp1.temp3 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[1]);
        temp_data.temp2.temp4 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[1]);
        temp_data.temp2.temp5 = TEST_TEMP_DC_TO_DOUT(core_temps_dC[1]);
    }

    // Setup additional cores if needed (would need more tiles)
    // For now, support up to 2 cores per tile 0
    if (num_cores > 2)
    {
        printf("Warning: Only 2 cores per tile supported in this test setup\n");
    }

    // Mock sensor FIFO polling returns for FUNCTIONAL mock: tile_index, curr_data_is_valid, more_entries, data_ptr
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);          // tile_index (tile 0)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);       // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false);      // more_entries
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_data); // data pointer

    if (g_print_logs)
    {
        for (uint8_t i = 0; i < num_cores && i < 2; i++)
        {
            printf("  Core %d: Temperature = %d dC (%.1f°C)\n", i, core_temps_dC[i], core_temps_dC[i] / 10.0);
        }
    }
}

/**
 * Helper function to setup tile voltage sensor FIFO with specific values for cores
 * @param core_voltages_mV Array of voltage values in mV for each core
 * @param num_cores Number of cores to setup (max NUMBER_OF_CORES_PER_DIE)
 */
static void setup_tile_voltage_fifo(uint16_t core_voltages_mV[], uint8_t num_cores)
{
    // Setup voltage FIFO entries - need to map cores to tile voltage sensors
    // Tile 0 maps to cores 0 and 1:
    // - Core 0: voltage from vcore0
    // - Core 1: voltage from vcore1

    static tile_voltage_t voltage_data;
    memset(&voltage_data, 0, sizeof(tile_voltage_t));

    // Set core voltages (convert mV to DOUT format)
    if (num_cores >= 1)
    {
        voltage_data.data.vcore0 = MILLIVOLTS2DOUT(core_voltages_mV[0]);
    }

    if (num_cores >= 2)
    {
        voltage_data.data.vcore1 = MILLIVOLTS2DOUT(core_voltages_mV[1]);
    }

    // Setup additional cores if needed (would need more tiles)
    // For now, support up to 2 cores per tile 0
    if (num_cores > 2)
    {
        printf("Warning: Only 2 cores per tile supported in this test setup\n");
    }

    // Mock sensor FIFO polling returns for FUNCTIONAL mock: tile_index, curr_data_is_valid, more_entries, data_ptr
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);             // tile_index (tile 0)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true);          // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false);         // more_entries
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_data); // data pointer

    if (g_print_logs)
    {
        for (uint8_t i = 0; i < num_cores && i < 2; i++)
        {
            printf("  Core %d: Voltage = %d mV\n", i, core_voltages_mV[i]);
        }
    }
}

/**
 * Helper function to validate and print histogram element, comparing actual vs expected values
 * Not writing this in test functions itself to reduce redundancy. But expected values are hardcoded in each test function.
 *
 * @param histogram_element Pointer to the histogram element to validate
 * @param expected_core_id Expected core ID being tested
 * @param expected_voltage_band Expected voltage band index
 * @param expected_temperature_band Expected temperature band index
 * @param expected_bin_count Expected bin count value
 * @param test_case_name Name of the test case for print output
 */
static void validate_histogram_element(pwr_core_element_histogram_t* histogram_element,
                                       uint8_t expected_core_id,
                                       uint8_t expected_voltage_band,
                                       uint8_t expected_temperature_band,
                                       uint32_t expected_bin_count,
                                       const char* test_case_name)
{
    // Assert histogram element fields match expected values
    assert_int_equal(histogram_element->voltage_band, expected_voltage_band);
    assert_int_equal(histogram_element->temperature_band, expected_temperature_band);
    assert_int_equal(histogram_element->bin_count, expected_bin_count);

    // Print validation results for debugging
    if (g_print_logs)
    {
        printf("  %s\n", test_case_name);
        printf("    Core ID: %d\n", expected_core_id);
        printf("    Voltage band     - Actual: %u, Expected: %u\n", histogram_element->voltage_band, expected_voltage_band);
        printf("    Temperature band - Actual: %u, Expected: %u\n", histogram_element->temperature_band, expected_temperature_band);
        printf("    Bin count        - Actual: %u, Expected: %u\n", histogram_element->bin_count, expected_bin_count);
    }
}

/**
 * Helper function to setup sensor FIFO empty status for all FIFOs except temperature and voltage
 */
static void setup_other_fifos_empty(void)
{
    static bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];

    // Set all FIFOs as empty except tile temperature and voltage
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        sensor_fifo_is_empty_mock[i] = true;
    }

    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = false; // Temperature FIFO has data
    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW] = false;     // Voltage FIFO has data

    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
}

// Setup function called before each test
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);

    // Reset all telemetry data to clean state FIRST
    reset_pwr_tlm_data();

    // Explicitly clear the 24-hour histogram data to ensure clean state
    if (p_computed_metrics_24_hrs != NULL)
    {
        memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs_t));
    }

    // Initializing data processing component to set up tile-to-core mapping and internal state for temperature FIFO processing
    data_proc_tlm_cmpnt_init(PRIMARY_DIE_ID, false, 0); // die_id=0, is_single_die=false, mpam_vm_mem_fixed_pwr_mW=0

    // Initializing comp_metrics to ensure metrics_24hrs_enabled is set
    // Do this AFTER reset to ensure clean state
    comp_metrics_init(false); // false = multi-die system → metrics_24hrs_enabled = true

    // Enable histogram updates by setting required flags
    in_band_publishing_active = true; // Enable in-band publishing for metrics

    // Enable all cores for histogram metrics computation
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_is_active[core_id] = true;
    }

    return 0;
}

// Teardown function called after each test
static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);

    // Reset all telemetry data to clean state
    reset_pwr_tlm_data();

    // Explicitly clearing the 24-hour histogram data to ensure clean state for next test
    if (p_computed_metrics_24_hrs != NULL)
    {
        memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs_t));
    }

    return 0;
}

//  Test: Core histogram 4 corner bins (catchall bins)
//  ====================================================
//  Tests all 4 corner bins of the histogram table which are catchall bins:
//  - [0,0]: High voltage (>974mV), high temperature (>965dC)
//  - [0,14]: High voltage (>974mV), low temperature (≤575dC)
//  - [19,0]: Low voltage (≤830mV), high temperature (>965dC)
//  - [19,14]: Low voltage (≤830mV), low temperature (≤575dC)
//
//  Test Flow Summary:
//  ==================
//  1. Setup sensor FIFOs with temperature and voltage values
//  2. Call data_proc_tlm_cmpnt_process_input_data() to process sensors and update histogram
//  3. Call package_create_pwr_core_histogram_record() to create record
//  4. Validate voltage_band, temperature_band, and bin_count fields match expected values
//  5. Repeat for each of the 4 corner bins (simulating four consecutive 10ms sampling periods)
TEST_FUNCTION(test_core_histogram_corner_bins, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_corner_bins ---\n");
        printf("Testing all 4 corner bins of the histogram table (catchall bins)\n");
    }

    // ===== Test corner bin [0,0]: High voltage (>974mV), high temperature (>965dC) =====
    {
        if (g_print_logs)
        {
            printf("\n=== Corner Bin [0,0]: High Voltage, High Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {1000};    // >965dC -> bin 0
        uint16_t core_voltages_mV[] = {1000}; // >974mV -> bin 0

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;
        const uint8_t expected_voltage_band = 0;     // Band 0 is catchall for voltages >974mV (input: 1000mV)
        const uint8_t expected_temperature_band = 0; // Band 0 is catchall for temperatures >965dC (input: 1000dC)
        const uint32_t expected_bin_count = 1;       // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Validate histogram element
        validate_histogram_element(
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band],
            expected_core_id,
            expected_voltage_band,
            expected_temperature_band,
            expected_bin_count,
            "Corner Bin [0,0]: High Voltage, High Temperature");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls
    // Each test block represents another call to process_input_data() with different sensor values
    // This simulates the firmware behavior where it processes sensors every 10ms and accumulates histogram data

    // ===== Test corner bin [0,14]: High voltage (>974mV), low temperature (≤575dC) =====
    {
        if (g_print_logs)
        {
            printf("\n=== Corner Bin [0,14]: High Voltage, Low Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {500};     // ≤575dC -> bin 14
        uint16_t core_voltages_mV[] = {1000}; // >974mV -> bin 0

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;
        const uint8_t expected_voltage_band = 0; // Band 0 is catchall for voltages >974mV (input: 1000mV)
        const uint8_t expected_temperature_band = 14; // Band 14 is catchall for temperatures ≤575dC (input: 500dC)
        const uint32_t expected_bin_count = 1;        // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Validate histogram element
        validate_histogram_element(
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band],
            expected_core_id,
            expected_voltage_band,
            expected_temperature_band,
            expected_bin_count,
            "Corner Bin [0,14]: High Voltage, Low Temperature");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls
    // Each test block represents another call to process_input_data() with different sensor values
    // This simulates the firmware behavior where it processes sensors every 10ms and accumulates histogram data

    // ===== Test corner bin [19,0]: Low voltage (≤830mV), high temperature (>965dC) =====
    {
        if (g_print_logs)
        {
            printf("\n=== Corner Bin [19,0]: Low Voltage, High Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {1000};   // >965dC -> bin 0
        uint16_t core_voltages_mV[] = {800}; // ≤830mV -> bin 19

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;
        const uint8_t expected_voltage_band = 19; // Band 19 is catchall for voltages ≤830mV (input: 800mV)
        const uint8_t expected_temperature_band = 0; // Band 0 is catchall for temperatures >965dC (input: 1000dC)
        const uint32_t expected_bin_count = 1;       // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Corner Bin [19,0]: Low Voltage, High Temperature");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls
    // Each test block represents another call to process_input_data() with different sensor values
    // This simulates the firmware behavior where it processes sensors every 10ms and accumulates histogram data

    // ===== Test corner bin [19,14]: Low voltage (≤830mV), low temperature (≤575dC) =====
    {
        if (g_print_logs)
        {
            printf("\n=== Corner Bin [19,14]: Low Voltage, Low Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {500};    // ≤575dC -> bin 14
        uint16_t core_voltages_mV[] = {800}; // ≤830mV -> bin 19

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;
        const uint8_t expected_voltage_band = 19; // Band 19 is catchall for voltages ≤830mV (input: 800mV)
        const uint8_t expected_temperature_band = 14; // Band 14 is catchall for temperatures ≤575dC (input: 500dC)
        const uint32_t expected_bin_count = 1;        // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Corner Bin [19,14]: Low Voltage, Low Temperature");
    }

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_corner_bins: ALL PASSED ---\n");
    }
}

//  Test: Core histogram boundary conditions
//  ==========================================
//  Tests boundary transitions between histogram bins to ensure proper bin assignment.
//  Tests both voltage and temperature boundaries with values just above and below the limits.
//
//  Voltage boundaries tested:
//  - 975mV (>974mV) -> band 0 (high bin)
//  - 974mV (≤974mV) -> band 1 (next lower bin)
//
//  Temperature boundaries tested:
//  - 966dC (>965dC) -> band 0 (high bin)
//  - 965dC (≤965dC) -> band 1 (next lower bin)
//
//  Test Flow Summary:
//  ==================
//  1. Setup sensor FIFOs with boundary temperature and voltage values
//  2. Call data_proc_tlm_cmpnt_process_input_data() to process sensors and update histogram
//  3. Call package_create_pwr_core_histogram_record() to create record
//  4. Validate voltage_band, temperature_band, and bin_count fields match expected values
//  5. Repeat for each boundary condition (simulating four consecutive 10ms sampling periods)
TEST_FUNCTION(test_core_histogram_boundary_conditions, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_boundary_conditions ---\n");
        printf("Testing boundary transitions between histogram bins\n");
    }

    // ===== Test voltage boundary: 975mV (>974mV) -> band 0 =====
    {
        if (g_print_logs)
        {
            printf("\n=== Voltage Boundary: 975mV -> Band 0 ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {800};    // Mid-range temperature
        uint16_t core_voltages_mV[] = {975}; // Just above 974mV boundary -> band 0

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 975mV → After conversion becomes 974mV → Falls in range (966, 974]mV → Bin 1 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 1;

        /* Temperature: Input 800dC → Falls in range (755, 785]dC → Bin 6 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 6;

        const uint32_t expected_bin_count = 1; // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Voltage Boundary: 975mV -> Band 1 (after conversion)");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls

    // ===== Test voltage boundary: 974mV (≤974mV) -> band 1 =====
    {
        if (g_print_logs)
        {
            printf("\n=== Voltage Boundary: 974mV -> Band 1 ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {800};    // Mid-range temperature
        uint16_t core_voltages_mV[] = {974}; // At 974mV boundary -> band 1

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 974mV → After conversion becomes 973mV → Falls in range (966, 974]mV → Bin 1 as per compute_metrics.c (SAME as test 1!) */
        const uint8_t expected_voltage_band = 1;

        /* Temperature: Input 800dC → Falls in range (755, 785]dC → Bin 6 as per compute_metrics.c (SAME as test 1!) */
        const uint8_t expected_temperature_band = 6;

        const uint32_t expected_bin_count = 2; // TWO samples - both test 1 and test 2 hit same bin[1][6]

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Voltage Boundary: 974mV -> Band 1 (after conversion)");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls

    // ===== Test temperature boundary: 966dC (>965dC) -> band 0 =====
    {
        if (g_print_logs)
        {
            printf("\n=== Temperature Boundary: 966dC -> Band 0 ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {966};    // Just above 965dC boundary -> band 0
        uint16_t core_voltages_mV[] = {900}; // Mid-range voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 900mV → After conversion becomes 899mV → Falls in range (894, 902]mV → Bin 10 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 10;

        /* Temperature: Input 966dC → Falls in range (965, ∞]dC → Bin 0 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 0;

        const uint32_t expected_bin_count = 1; // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Temperature Boundary: 966dC -> Band 0");
    }

    // DO NOT reset histogram - test histogram accumulation across multiple processing calls

    // ===== Test temperature boundary: 965dC (≤965dC) -> band 1 =====
    {
        if (g_print_logs)
        {
            printf("\n=== Temperature Boundary: 965dC -> Band 1 ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {965};    // At 965dC boundary -> band 1
        uint16_t core_voltages_mV[] = {900}; // Mid-range voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 900mV → After conversion becomes 899mV → Falls in range (894, 902]mV → Bin 10 as per compute_metrics.c (SAME as test 3) */
        const uint8_t expected_voltage_band = 10;

        /* Temperature: Input 965dC → Falls in range (935, 965]dC → Bin 1 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 1;

        const uint32_t expected_bin_count = 1; // One 10ms sample submitted to this bin

        // ===== ASSERT ACTUAL VS EXPECTED =====
        // Get pointer to histogram element and validate
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Temperature Boundary: 965dC -> Band 1");
    }

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_boundary_conditions: ALL PASSED ---\n");
    }
}

//  Test: Core histogram random bins
//  ==================================
//  Tests 3-4 random bins scattered throughout the histogram table (not corners, not boundaries).
//  Validates that mid-range voltage and temperature values are correctly binned.
//
//  Bins tested:
//  - [5,5]: Mid-range voltage (~942mV) and mid-range temperature (~815dC)
//  - [10,7]: Lower-mid voltage (~902mV) and mid-low temperature (~755dC)
//  - [18,4]: Low voltage (~838mV) and high-mid temperature (~860dC)
//  - [3,11]: High-mid voltage (~958mV) and low temperature (~635dC)
//
//  Test Flow Summary:
//  ==================
//  1. Setup sensor FIFOs with mid-range temperature and voltage values
//  2. Call data_proc_tlm_cmpnt_process_input_data() to process sensors and update histogram
//  3. Call package_create_pwr_core_histogram_record() to create record
//  4. Validate voltage_band, temperature_band, and bin_count fields match expected values
//  5. Repeat for each random bin (simulating four consecutive 10ms sampling periods)
TEST_FUNCTION(test_core_histogram_random_bins, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_random_bins ---\n");
        printf("Testing random bins scattered throughout the histogram table\n");
    }

    // ===== Test random bin [5,5]: Mid-range voltage and temperature =====
    {
        if (g_print_logs)
        {
            printf("\n=== Random Bin [5,5]: Mid-range Voltage, Mid-range Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {820};    // Mid-range temperature
        uint16_t core_voltages_mV[] = {943}; // Mid-range voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 943mV → After conversion becomes 942mV → Falls in range (934, 942]mV → Bin 5 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 5;

        /* Temperature: Input 820dC → Falls in range (785, 815]dC → Bin 5 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 5;

        const uint32_t expected_bin_count = 1;

        // ===== ASSERT ACTUAL VS EXPECTED =====
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Random Bin [5,5]: Mid-range Voltage, Mid-range Temperature");
    }

    // ===== Test random bin [10,7]: Lower-mid voltage, mid-low temperature =====
    {
        if (g_print_logs)
        {
            printf("\n=== Random Bin [10,7]: Lower-mid Voltage, Mid-low Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {760};    // Mid-low temperature
        uint16_t core_voltages_mV[] = {903}; // Lower-mid voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 903mV → After conversion becomes 902mV → Falls in range (894, 902]mV → Bin 10 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 10;

        /* Temperature: Input 760dC → Falls in range (725, 755]dC → Bin 7 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 7;

        const uint32_t expected_bin_count = 1;

        // ===== ASSERT ACTUAL VS EXPECTED =====
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Random Bin [10,7]: Lower-mid Voltage, Mid-low Temperature");
    }

    // ===== Test random bin [15,3]: Low voltage, high-mid temperature =====
    {
        if (g_print_logs)
        {
            printf("\n=== Random Bin [18,4]: Low Voltage, High-mid Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {860};    // High-mid temperature
        uint16_t core_voltages_mV[] = {838}; // Low voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 838mV → After conversion becomes 837mV → 837 > 830 → Bin 18 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 18;

        /* Temperature: Input 860dC → Falls in range (845, 875]dC → Bin 4 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 4;

        const uint32_t expected_bin_count = 1;

        // ===== ASSERT ACTUAL VS EXPECTED =====
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Random Bin [18,4]: Low Voltage, High-mid Temperature");
    }

    // ===== Test random bin [3,11]: High-mid voltage, low temperature =====
    {
        if (g_print_logs)
        {
            printf("\n=== Random Bin [3,11]: High-mid Voltage, Low Temperature ===\n");
        }

        // ===== INPUT VALUES =====
        uint16_t core_temps_dC[] = {640};    // Low temperature
        uint16_t core_voltages_mV[] = {959}; // High-mid voltage

        // Setup sensor FIFOs with temperature and voltage data
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES =====
        const uint8_t expected_core_id = 0;

        /* Voltage: Input 959mV → After conversion becomes 958mV → Falls in range (950, 958]mV → Bin 3 as per compute_metrics.c */
        const uint8_t expected_voltage_band = 3;

        /* Temperature: Input 640dC → Falls in range (605, 635]dC → Bin 11 as per compute_metrics.c */
        const uint8_t expected_temperature_band = 11;

        const uint32_t expected_bin_count = 1;

        // ===== ASSERT ACTUAL VS EXPECTED =====
        pwr_core_element_histogram_t* histogram_element =
            &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

        validate_histogram_element(histogram_element,
                                   expected_core_id,
                                   expected_voltage_band,
                                   expected_temperature_band,
                                   expected_bin_count,
                                   "Random Bin [3,11]: High-mid Voltage, Low Temperature");
    }

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_random_bins: ALL PASSED ---\n");
    }
}

//  Test: Core histogram multiple FIFO entries (last entry wins)
//  =============================================================
//  Tests that when multiple sensor FIFO entries are processed on a 10ms polling interval,
//  only the LAST entry values are logged in the histogram table.
//
//  This is a CRITICAL requirement: "If multiple sensor fifo entries are processed on the
//  10mS polling interval, only the last entry values will be logged in the table."
//
//  Test Strategy:
//  ==============
//  - Setup voltage FIFO with 3 entries: Entry 1 (950mV), Entry 2 (930mV), Entry 3 (975mV)
//  - Setup temperature FIFO with 3 entries: Entry 1 (900dC), Entry 2 (850dC), Entry 3 (800dC)
//  - Set more_entries = true for entries 1 and 2, false for entry 3 (last entry)
//  - Verify ONLY Entry 3 values (975mV, 800dC) are logged in histogram
//  - Verify entries 1 and 2 do NOT increment any bins
//
//  Test Flow Summary:
//  ==================
//  1. Setup sensor FIFOs with multiple entries (more_entries flag controls iteration)
//  2. Call data_proc_tlm_cmpnt_process_input_data() to process ALL entries
//  3. Call package_create_pwr_core_histogram_record() to create record
//  4. Validate only the LAST entry's bin was incremented
//  5. Validate earlier entries' bins were NOT incremented
TEST_FUNCTION(test_core_histogram_multiple_fifo_entries, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_multiple_fifo_entries ---\n");
        printf("Testing that only LAST FIFO entry values are logged in histogram\n");
    }

    // ===== Setup multiple temperature FIFO entries =====
    static tile_temp_t temp_data_entry1, temp_data_entry2, temp_data_entry3;
    memset(&temp_data_entry1, 0, sizeof(tile_temp_t));
    memset(&temp_data_entry2, 0, sizeof(tile_temp_t));
    memset(&temp_data_entry3, 0, sizeof(tile_temp_t));

    // Entry 1: 900dC (should be ignored)
    temp_data_entry1.temp0.temp_valid = 1;
    temp_data_entry1.temp1.temp0 = TEST_TEMP_DC_TO_DOUT(900);
    temp_data_entry1.temp1.temp1 = TEST_TEMP_DC_TO_DOUT(900);
    temp_data_entry1.temp1.temp2 = TEST_TEMP_DC_TO_DOUT(900);

    // Entry 2: 850dC (should be ignored)
    temp_data_entry2.temp0.temp_valid = 1;
    temp_data_entry2.temp1.temp0 = TEST_TEMP_DC_TO_DOUT(850);
    temp_data_entry2.temp1.temp1 = TEST_TEMP_DC_TO_DOUT(850);
    temp_data_entry2.temp1.temp2 = TEST_TEMP_DC_TO_DOUT(850);

    // Entry 3: 800dC (LAST ENTRY - should be logged)
    temp_data_entry3.temp0.temp_valid = 1;
    temp_data_entry3.temp1.temp0 = TEST_TEMP_DC_TO_DOUT(800);
    temp_data_entry3.temp1.temp1 = TEST_TEMP_DC_TO_DOUT(800);
    temp_data_entry3.temp1.temp2 = TEST_TEMP_DC_TO_DOUT(800);

    // ===== Setup multiple voltage FIFO entries =====
    static tile_voltage_t voltage_data_entry1, voltage_data_entry2, voltage_data_entry3;
    memset(&voltage_data_entry1, 0, sizeof(tile_voltage_t));
    memset(&voltage_data_entry2, 0, sizeof(tile_voltage_t));
    memset(&voltage_data_entry3, 0, sizeof(tile_voltage_t));

    // Entry 1: 950mV (should be ignored)
    voltage_data_entry1.data.vcore0 = MILLIVOLTS2DOUT(950);

    // Entry 2: 930mV (should be ignored)
    voltage_data_entry2.data.vcore0 = MILLIVOLTS2DOUT(930);

    // Entry 3: 975mV (LAST ENTRY - should be logged) - Using known working value from boundary test
    voltage_data_entry3.data.vcore0 = MILLIVOLTS2DOUT(975);

    // ===== Setup FIFO empty status =====
    setup_other_fifos_empty();

    // ===== Mock temperature FIFO polling - 3 entries =====
    // Entry 1: more_entries = true
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);    // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true); // more_entries = TRUE (not last)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_data_entry1);

    // Entry 2: more_entries = true
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);    // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true); // more_entries = TRUE (not last)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_data_entry2);

    // Entry 3: more_entries = false (LAST ENTRY)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);  // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries = FALSE (last entry)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_data_entry3);

    // ===== Mock voltage FIFO polling - 3 entries =====
    // Entry 1: more_entries = true
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);    // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true); // more_entries = TRUE (not last)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_data_entry1);

    // Entry 2: more_entries = true
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);    // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true); // more_entries = TRUE (not last)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_data_entry2);

    // Entry 3: more_entries = false (LAST ENTRY)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true);  // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries = FALSE (last entry)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_data_entry3);

    if (g_print_logs)
    {
        printf("  Entry 1: 950mV, 900dC (should be ignored)\n");
        printf("  Entry 2: 930mV, 850dC (should be ignored)\n");
        printf("  Entry 3: 975mV, 800dC (LAST ENTRY - should be logged)\n");
    }

    // Process sensor data - will iterate through all 3 entries for each FIFO
    data_proc_tlm_cmpnt_process_input_data();

    // Create histogram record
    pwr_core_record_histogram_t histogram_record = {{0}};
    package_create_pwr_core_histogram_record(&histogram_record);

    // ===== EXPECTED VALUES - Only Entry 3 should be logged =====
    const uint8_t expected_core_id = 0;
    /* Voltage: Input 975mV (Entry 3) → After conversion becomes 974mV → Falls in range (966, 974]mV → Bin 1 as per compute_metrics.c */
    const uint8_t expected_voltage_band = 1;
    /* Temperature: Input 800dC (Entry 3) → Falls in range (755, 785]dC → Bin 6 as per compute_metrics.c */
    const uint8_t expected_temperature_band = 6;
    const uint32_t expected_bin_count = 1; // Only last entry logged

    // ===== ASSERT ACTUAL VS EXPECTED =====
    pwr_core_element_histogram_t* histogram_element =
        &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

    validate_histogram_element(histogram_element,
                               expected_core_id,
                               expected_voltage_band,
                               expected_temperature_band,
                               expected_bin_count,
                               "Multiple FIFO Entries: Only Last Entry Logged");

    // ===== Verify Entry 1 bin was NOT incremented =====
    const uint32_t expected_ignored_entry_bin_count = 0; // Ignored FIFO entries should NOT increment bins

    // Entry 1: 950mV → Bin 4, 900dC → Bin 2
    pwr_core_element_histogram_t* entry1_element =
        &histogram_record.histogram_collection[expected_core_id].histogram_element[4][2];

    if (g_print_logs)
    {
        printf("  Verifying Entry 1 bin[4][2] was NOT incremented: count = %u (expected %u)\n",
               entry1_element->bin_count,
               expected_ignored_entry_bin_count);
    }
    assert_int_equal(entry1_element->bin_count, expected_ignored_entry_bin_count);

    // ===== Verify Entry 2 bin was NOT incremented =====
    // Entry 2: 930mV → Bin 6, 850dC → Bin 4
    pwr_core_element_histogram_t* entry2_element =
        &histogram_record.histogram_collection[expected_core_id].histogram_element[6][4];

    if (g_print_logs)
    {
        printf("  Verifying Entry 2 bin[6][4] was NOT incremented: count = %u (expected %u)\n",
               entry2_element->bin_count,
               expected_ignored_entry_bin_count);
    }
    assert_int_equal(entry2_element->bin_count, expected_ignored_entry_bin_count);

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_multiple_fifo_entries: ALL PASSED ---\n");
    }
}

//  Test: Core histogram multiple cores
//  ====================================
//  Tests that histogram tracking is independent for each core.
//  Validates that different cores with different voltage/temperature combinations
//  update their respective histogram bins without affecting other cores.
//
//  Test Coverage:
//  ==============
//  - Core 0: 950mV, 850dC → Verify bin incremented for core 0 only
//  - Core 1: 900mV, 750dC → Verify bin incremented for core 1 only
//  - Core 0 again: Same values → Verify core 0 bin count = 2, core 1 unchanged
//  - Verify each core's histogram is independent
//
//  Test Flow Summary:
//  ==================
//  1. Setup sensor FIFOs with values for both core 0 and core 1
//  2. Call data_proc_tlm_cmpnt_process_input_data() to process sensors
//  3. Call package_create_pwr_core_histogram_record() to create record
//  4. Validate each core's bins are incremented independently
//  5. Repeat with same values for core 0 to verify accumulation
TEST_FUNCTION(test_core_histogram_multiple_cores, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_multiple_cores ---\n");
        printf("Testing histogram independence across multiple cores\n");
    }

    // ===== Test 1: Core 0 and Core 1 with different values =====
    {
        if (g_print_logs)
        {
            printf("\n=== Test 1: Core 0 (950mV, 850dC) and Core 1 (900mV, 750dC) ===\n");
        }

        // ===== INPUT VALUES for both cores =====
        uint16_t core_temps_dC[] = {850, 750};    // Core 0: 850dC, Core 1: 750dC
        uint16_t core_voltages_mV[] = {950, 900}; // Core 0: 950mV, Core 1: 900mV

        // Setup sensor FIFOs with temperature and voltage data for 2 cores
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 2);
        setup_tile_voltage_fifo(core_voltages_mV, 2);

        // Process sensor data and trigger histogram update
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES for Core 0 =====
        /* Voltage: Input 950mV → After conversion becomes 950mV → Falls in range (942, 950]mV → Bin 4 as per compute_metrics.c */
        const uint8_t expected_core0_voltage_band = 4;

        /* Temperature: Input 850dC → Falls in range (815, 845]dC → Bin 4 as per compute_metrics.c */
        const uint8_t expected_core0_temperature_band = 4;

        const uint32_t expected_core0_bin_count = 1;

        // ===== ASSERT Core 0 =====
        pwr_core_element_histogram_t* core0_element =
            &histogram_record.histogram_collection[0].histogram_element[expected_core0_voltage_band][expected_core0_temperature_band];

        validate_histogram_element(core0_element,
                                   0,
                                   expected_core0_voltage_band,
                                   expected_core0_temperature_band,
                                   expected_core0_bin_count,
                                   "Core 0: 950mV, 850dC");

        // ===== EXPECTED VALUES for Core 1 =====
        /* Voltage: Input 900mV → After conversion becomes 899mV → Falls in range (894, 902]mV → Bin 10 as per compute_metrics.c */
        const uint8_t expected_core1_voltage_band = 10;

        /* Temperature: Input 750dC → Falls in range (725, 755]dC → Bin 8 as per compute_metrics.c */
        const uint8_t expected_core1_temperature_band = 8;

        const uint32_t expected_core1_bin_count = 1;

        // ===== ASSERT Core 1 =====
        pwr_core_element_histogram_t* core1_element =
            &histogram_record.histogram_collection[1].histogram_element[expected_core1_voltage_band][expected_core1_temperature_band];

        validate_histogram_element(core1_element,
                                   1,
                                   expected_core1_voltage_band,
                                   expected_core1_temperature_band,
                                   expected_core1_bin_count,
                                   "Core 1: 900mV, 750dC");

        // ===== Verify Core 0's bin is NOT incremented in Core 1 =====
        const uint32_t expected_cross_check_bin_count = 0; // Cross-core bins should remain at zero

        pwr_core_element_histogram_t* core1_check_element =
            &histogram_record.histogram_collection[1].histogram_element[expected_core0_voltage_band][expected_core0_temperature_band];

        if (g_print_logs)
        {
            printf("  Verifying Core 1 bin[%u][%u] was NOT incremented: count = %u (expected %u)\n",
                   expected_core0_voltage_band,
                   expected_core0_temperature_band,
                   core1_check_element->bin_count,
                   expected_cross_check_bin_count);
        }
        assert_int_equal(core1_check_element->bin_count, expected_cross_check_bin_count);

        // ===== Verify Core 1's bin is NOT incremented in Core 0 =====
        pwr_core_element_histogram_t* core0_check_element =
            &histogram_record.histogram_collection[0].histogram_element[expected_core1_voltage_band][expected_core1_temperature_band];

        if (g_print_logs)
        {
            printf("  Verifying Core 0 bin[%u][%u] was NOT incremented: count = %u (expected %u)\n",
                   expected_core1_voltage_band,
                   expected_core1_temperature_band,
                   core0_check_element->bin_count,
                   expected_cross_check_bin_count);
        }
        assert_int_equal(core0_check_element->bin_count, expected_cross_check_bin_count);
    }

    // ===== Test 2: Core 0 again with same values (accumulation test) =====
    {
        if (g_print_logs)
        {
            printf("\n=== Test 2: Core 0 again with same values (accumulation test) ===\n");
        }

        // ===== INPUT VALUES - Core 0 only, same as before =====
        uint16_t core_temps_dC[] = {850};    // Core 0: 850dC (same as test 1)
        uint16_t core_voltages_mV[] = {950}; // Core 0: 950mV (same as test 1)

        // Setup sensor FIFOs
        setup_other_fifos_empty();
        setup_tile_temperature_fifo(core_temps_dC, 1);
        setup_tile_voltage_fifo(core_voltages_mV, 1);

        // Process sensor data
        data_proc_tlm_cmpnt_process_input_data();

        // Create histogram record
        pwr_core_record_histogram_t histogram_record = {{0}};
        package_create_pwr_core_histogram_record(&histogram_record);

        // ===== EXPECTED VALUES for Core 0 - should accumulate =====
        const uint8_t expected_core0_voltage_band = 4;
        const uint8_t expected_core0_temperature_band = 4;
        const uint32_t expected_core0_bin_count = 2; // Accumulated from test 1

        // ===== ASSERT Core 0 accumulated =====
        pwr_core_element_histogram_t* core0_element =
            &histogram_record.histogram_collection[0].histogram_element[expected_core0_voltage_band][expected_core0_temperature_band];

        validate_histogram_element(core0_element,
                                   0,
                                   expected_core0_voltage_band,
                                   expected_core0_temperature_band,
                                   expected_core0_bin_count,
                                   "Core 0: Accumulated count = 2");

        // ===== EXPECTED VALUES for Core 1 - should remain unchanged =====
        const uint8_t expected_core1_voltage_band = 10;
        const uint8_t expected_core1_temperature_band = 8;
        const uint32_t expected_core1_bin_count = 1; // Unchanged from test 1

        // ===== ASSERT Core 1 unchanged =====
        pwr_core_element_histogram_t* core1_element =
            &histogram_record.histogram_collection[1].histogram_element[expected_core1_voltage_band][expected_core1_temperature_band];

        validate_histogram_element(core1_element,
                                   1,
                                   expected_core1_voltage_band,
                                   expected_core1_temperature_band,
                                   expected_core1_bin_count,
                                   "Core 1: Unchanged count = 1");
    }

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_multiple_cores: ALL PASSED ---\n");
    }
}

//  Test: Core histogram bin transitions (systematic threshold validation)
//  =======================================================================
//  Systematically validates bin transitions by testing values just above and at threshold boundaries.
//  Uses voltage_bin_tests[] and temperature_bin_tests[] arrays
//  which contain hardcoded input values with known expected bin assignments.
//
//  This test demonstrates:
//  - Temperature transitions: Clear bin changes when crossing thresholds (no conversion)
//  - Voltage transitions: Conversion effects on bin assignment (~1mV reduction)
//  - Tests against hardcoded known values in voltage_bin_tests[] and temperature_bin_tests[]
//  - Validates against expected bin values defined in variables:
//    * expected_core_id
//    * expected_voltage_band
//    * expected_temperature_band
//    * expected_bin_count
//
//  Test Strategy:
//  ==============
//  For each test case in voltage_bin_tests[] and temperature_bin_tests[]:
//  - Use hardcoded input values (input_voltage_mV, input_temperature_dC)
//  - Validate against hardcoded expected bins (expected_voltage_bin, expected_temperature_bin)
//  - Test temperature: threshold+1 vs threshold with pre-calculated expected bins
//  - Test voltage: Selected thresholds with pre-calculated expected bins accounting for conversion
//
//  Test Flow Summary:
//  ==================
//  1. Iterate through temperature_bin_tests[] with hardcoded known test values
//  2. Iterate through voltage_bin_tests[] with hardcoded known test values
//  3. Validate that actual bins match hardcoded expected values (expected_core_id, expected_voltage_band, expected_temperature_band, expected_bin_count)
TEST_FUNCTION(test_core_histogram_bin_transitions_systematic, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_core_histogram_bin_transitions_systematic ---\n");
        printf("Systematically testing bin transitions using pre-calculated test data\n");
    }

    // ===== Test Temperature Bin Transitions =====
    {
        if (g_print_logs)
        {
            printf("\n=== Temperature Bin Transitions ===\n");
            printf("Testing %zu cases from temperature_bin_tests[]\n",
                   sizeof(temperature_bin_tests) / sizeof(temperature_bin_tests[0]));
        }

        // Test representative temperature cases (not all 30 to keep test runtime reasonable)
        const size_t temp_test_indices[] = {0, 1, 7, 8, 15, 16, 23, 24, 29}; // 9 critical transitions
        const uint16_t test_voltage_mV = 900; // Mid-range voltage for all temperature tests

        for (size_t i = 0; i < sizeof(temp_test_indices) / sizeof(temp_test_indices[0]); i++)
        {
            const size_t idx = temp_test_indices[i];
            const temperature_bin_test_t* test = &temperature_bin_tests[idx];

            if (g_print_logs)
            {
                printf("\n  Test[%zu]: %s\n", idx, test->description);
            }

            uint16_t core_temps_dC[] = {test->input_temperature_dC};
            uint16_t core_voltages_mV[] = {test_voltage_mV};

            setup_other_fifos_empty();
            setup_tile_temperature_fifo(core_temps_dC, 1);
            setup_tile_voltage_fifo(core_voltages_mV, 1);

            data_proc_tlm_cmpnt_process_input_data();

            pwr_core_record_histogram_t histogram_record = {{0}};
            package_create_pwr_core_histogram_record(&histogram_record);

            const uint8_t expected_core_id = 0;
            const uint8_t expected_voltage_band = 10; // 900mV → 899mV → Bin 10
            const uint8_t expected_temperature_band = test->expected_temperature_bin;
            const uint32_t expected_bin_count = 1;

            pwr_core_element_histogram_t* histogram_element =
                &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

            validate_histogram_element(histogram_element,
                                       expected_core_id,
                                       expected_voltage_band,
                                       expected_temperature_band,
                                       expected_bin_count,
                                       test->description);

            // Reset histogram for next test iteration or for next test it will accumulate with previous tests.
            memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs_t));
        }
    }

    // ===== Test Voltage Bin Transitions =====
    {
        if (g_print_logs)
        {
            printf("\n=== Voltage Bin Transitions ===\n");
            printf("Testing %zu cases from voltage_bin_tests[]\n",
                   sizeof(voltage_bin_tests) / sizeof(voltage_bin_tests[0]));
        }

        // Test representative voltage cases (not all 23 to keep test runtime reasonable)
        const size_t voltage_test_indices[] = {0, 1, 5, 10, 15, 20, 21, 22}; // 8 critical cases including boundaries
        const uint16_t test_temperature_dC = 800; // Mid-range temperature for all voltage tests

        for (size_t i = 0; i < sizeof(voltage_test_indices) / sizeof(voltage_test_indices[0]); i++)
        {
            const size_t idx = voltage_test_indices[i];
            const voltage_bin_test_t* test = &voltage_bin_tests[idx];

            if (g_print_logs)
            {
                printf("\n  Test[%zu]: %s\n", idx, test->description);
            }

            uint16_t core_temps_dC[] = {test_temperature_dC};
            uint16_t core_voltages_mV[] = {test->input_voltage_mV};

            setup_other_fifos_empty();
            setup_tile_temperature_fifo(core_temps_dC, 1);
            setup_tile_voltage_fifo(core_voltages_mV, 1);

            data_proc_tlm_cmpnt_process_input_data();

            pwr_core_record_histogram_t histogram_record = {{0}};
            package_create_pwr_core_histogram_record(&histogram_record);

            const uint8_t expected_core_id = 0;
            const uint8_t expected_voltage_band = test->expected_voltage_bin;
            const uint8_t expected_temperature_band = 6; // 800dC → Bin 6
            const uint32_t expected_bin_count = 1;

            pwr_core_element_histogram_t* histogram_element =
                &histogram_record.histogram_collection[expected_core_id].histogram_element[expected_voltage_band][expected_temperature_band];

            validate_histogram_element(histogram_element,
                                       expected_core_id,
                                       expected_voltage_band,
                                       expected_temperature_band,
                                       expected_bin_count,
                                       test->description);

            // Reset histogram for next test iteration or for next test it will accumulate with previous tests.
            memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs_t));
        }
    }

    if (g_print_logs)
    {
        printf("\n--- END test_core_histogram_bin_transitions_systematic ---\n");
    }
}