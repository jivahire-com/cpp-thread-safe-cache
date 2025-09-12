//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file log_DIMM_power_integration_test.cpp
 * Integration test for verifying DIMM power and frequency ID logging.
 *
 * Test Steps:
 * -----------
 * 1. Create and initialize DIMM sensor data with valid power and frequency ID values for each DIMM module.
 * 2. Process the data using the DIMM sensor FIFO function.
 * 3. Create a DIMM power record and a DIMM runtime record from the processed data.
 * 4. Verify that the power and frequency ID values in the records match the input values.
 * 5. Test multiple power samples for a single DIMM to validate calculated average, minimum, and maximum power values.
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779093

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <data_utilities_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

#define NO_OF_ITERATIONS 4

// Global variable to control debug printing
static bool g_print_logs = true;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// Simple test: Validates that input DIMM power and frequency ID values pass through unchanged to output
// records Test function to verify DIMM power and frequency ID logging
TEST_FUNCTION(log_DIMM_power_and_frequency_id_integration_test, test_setup, test_teardown)
{
    if (g_print_logs)
        printf("\n\n--- START log_DIMM_power_and_frequency_id_integration_test---\n\n");

    // These input values simulate realistic DIMM sensor readings for power and frequency ID.
    sensor_ram_dimm_info_t dimm_info[] = {
        {.dimm_temp_s0_dC = 26, .dimm_temp_s1_dC = 28, .dimm_power_mW = 100, .dimm_id = 0, .dimm_throttling = 0, .dimm_memory_frequency_id = 0},
        {.dimm_temp_s0_dC = 30, .dimm_temp_s1_dC = 32, .dimm_power_mW = 120, .dimm_id = 1, .dimm_throttling = 0, .dimm_memory_frequency_id = 1},
        {.dimm_temp_s0_dC = 35, .dimm_temp_s1_dC = 38, .dimm_power_mW = 150, .dimm_id = 2, .dimm_throttling = 1, .dimm_memory_frequency_id = 2},
        {.dimm_temp_s0_dC = 40, .dimm_temp_s1_dC = 42, .dimm_power_mW = 180, .dimm_id = 3, .dimm_throttling = 1, .dimm_memory_frequency_id = 3}};

    sensor_ram_dimm_info_t mock_dimm_data = {0};

    for (int32_t iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        mock_dimm_data = dimm_info[iteration];

        // Only mock the DIMM sensor, no need to mock other sensors
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &mock_dimm_data);

        // Directly process only the DIMM sensor FIFO
        data_smpl_process_dimm_sensor_fifo();

        // Validate DIMM power using the correct record and member
        pwr_soc_record_dimm_power_t dimm_power_record;
        package_create_pwr_soc_dimm_power_record(&dimm_power_record);

        int32_t expected_power = dimm_info[iteration].dimm_power_mW;
        int32_t actual_power = dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.max_mW;
        assert_int_equal(expected_power, dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.max_mW);
        assert_int_equal(expected_power, dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.min_mW);
        assert_int_equal(expected_power, dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.average_mW);

        // Validate DIMM frequency ID using the runtime record
        inst_soc_record_dimm_runtime_t dimm_rt_record;
        package_create_inst_soc_dimm_runtime_record(&dimm_rt_record);

        int32_t expected_freq_id = dimm_info[iteration].dimm_memory_frequency_id;
        int32_t actual_freq_id = dimm_rt_record.dimm_collection[iteration].dimm_rt_element.memory_freq_id;
        assert_int_equal(expected_freq_id, actual_freq_id);

        // Print debug info if enabled
        if (g_print_logs)
        {
            printf("Iteration %d: Power expected=%d actual=%d | max=%d min=%d avg=%d | FreqID expected=%d "
                   "actual=%d\n",
                   iteration,
                   expected_power,
                   actual_power,
                   dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.max_mW,
                   dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.min_mW,
                   dimm_power_record.dimm_collection[iteration].dimm_element.power_mW.average_mW,
                   expected_freq_id,
                   actual_freq_id);
        }
    }
    if (g_print_logs)
    {
        printf("--- END log_DIMM_power_and_frequency_id_integration_test ---\n");
        printf("***\n");
    }
}

// Test function to verify DIMM power average calculation
// This test simulates multiple samples for a single DIMM and checks the average, min, and max power values.
TEST_FUNCTION(log_DIMM_power_average_calculation_test, test_setup, test_teardown)
{
    if (g_print_logs)
        printf("\n\n--- START log_DIMM_power_average_calculation_test---\n\n");

    // Simulate multiple samples for dimm_id = 0 to test average, min, and max calculation
    int32_t power_samples[] = {100, 200, 300};
    int sample_count = sizeof(power_samples) / sizeof(power_samples[0]);
    printf("Test provided %d samples\n", sample_count);
    uint8_t test_dimm_id = 0;

    sensor_ram_dimm_info_t mock_dimm_data = {0};
    for (int i = 0; i < sample_count; ++i)
    {
        mock_dimm_data.dimm_id = test_dimm_id;
        mock_dimm_data.dimm_power_mW = power_samples[i];
        mock_dimm_data.dimm_temp_s0_dC = 30;
        mock_dimm_data.dimm_temp_s1_dC = 32;
        mock_dimm_data.dimm_throttling = 0;
        mock_dimm_data.dimm_memory_frequency_id = 1;

        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &mock_dimm_data);

        // Process the DIMM sensor FIFO for each sample
        data_smpl_process_dimm_sensor_fifo();
    }
    // Validate DIMM power metrics for dimm_id = 0
    pwr_soc_record_dimm_power_t dimm_power_record;
    package_create_pwr_soc_dimm_power_record(&dimm_power_record);

    // Calculated expected values based on provided samples {100, 200, 300}
    int32_t expected_min = 100;
    int32_t expected_max = 300;
    // Running average calculation explanation:
    // Step 1: Start with 100 → average = 100, samples = 1
    // Step 2: Add 200 → average = (100 * 1 + 200 + 2 / 2) / 2 = (300 + 1) / 2 = 150
    // Step 3: Add 300 → average = (150 * 2 + 300 + 3 / 2) / 3 = (600 + 1) / 3 = 200
    // Final expected average would be 200
    int32_t expected_avg = power_samples[0];
    for (int i = 1; i < sample_count; ++i)
    {
        expected_avg = (expected_avg * i + power_samples[i] + (i + 1) / 2) / (i + 1);
    }

    assert_int_equal(expected_max, dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.max_mW);
    assert_int_equal(expected_min, dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.min_mW);
    assert_int_equal(expected_avg, dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.average_mW);

    if (g_print_logs)
    {
        printf("DIMM %d: Power samples = [%d, %d, %d]\n", test_dimm_id, power_samples[0], power_samples[1], power_samples[2]);

        printf("Min Power - Expected: %d, Actual: %d\n",
               expected_min,
               dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.min_mW);

        printf("Max Power - Expected: %d, Actual: %d\n",
               expected_max,
               dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.max_mW);

        printf("Avg Power - Expected: %d, Actual: %d\n",
               expected_avg,
               dimm_power_record.dimm_collection[test_dimm_id].dimm_element.power_mW.average_mW);
    }
    if (g_print_logs)
    {
        printf("--- END log_DIMM_power_average_calculation_test ---\n");
        printf("***\n");
    }
}
