//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_soc_pvt_temperature.cpp
 * Test functionality and flow power_telemetry soc PVT temperature collection
 *
 * Step-1:
 * Create and initialize PVT temperature data for all SoC PVT sensors.
 * Set known temperature values for each sensor in the test data set.
 *
 * Step-2:
 * Process the data:
 * Mock all unrelated sensor polling calls to return no data.
 * Mock the SoC PVT temperature polling call to return valid test data for all sensor IDs.
 * Call the data aggregation function to process the mocked sensor data.
 * Update internal data structures with the PVT temperature values for each sensor ID.
 *
 * Step-3:
 * Create PVT temperature record:
 * Package the processed PVT temperature data into a record structure.
 * The record contains temperature data for all SoC PVT sensors, indexed by sensor ID.
 *
 * Step-4:
 * Verify the results:
 * For each PVT sensor ID, verify all temperature statistics:
 * - Minimum value: Matches the initial base value (first reading)
 * - Maximum value: Matches the current reading (since values increase with each iteration)
 * - Average value: Matches the running average calculation:
 *   * First iteration: equals base value
 *   * Subsequent iterations: (base + (base+1) + (base+2) + ... + (base+iteration)) / (iteration + 1)
 *   * Values are rounded to nearest integer value
 * All values are verified through assertions to ensure they match the expected values for each sensor ID and iteration.
 *
 * Record structs are in telemetry_package_defs.h
 */

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <data_proc_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}
#define NO_OF_ITERATIONS 4

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

// Structure to hold test PVT temperature configurations
typedef struct
{
    uint16_t sensor_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
} test_pvt_temp_config_t;

// The test data is selected to cover a range of possible sensor readings, including low, mid, and high values.
// The values are arbitrary but distinctly different, so that each sensor's output can be uniquely verified in the test.
static void get_expected_pvt_temp_values(int32_t iteration, int32_t pvt_index, uint16_t* expected_min, uint16_t* expected_max, uint16_t* expected_avg)
{
    // Base values for each index from iteration 0
    static const uint16_t base_values[NUMBER_OF_SOC_TEMP_SENSORS] = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14, 44};

    // Calculate latest value for comparison
    uint16_t latest_value = base_values[pvt_index] + iteration;

    if (iteration == 0)
    {
        // First iteration: all values equal the base value
        *expected_min = base_values[pvt_index];
        *expected_max = base_values[pvt_index];
        *expected_avg = base_values[pvt_index];
    }
    else
    {
        // Min: Update if new value is lower or if current min is 0
        if (latest_value < base_values[pvt_index] || base_values[pvt_index] == 0)
        {
            *expected_min = latest_value;
        }
        else
        {
            *expected_min = base_values[pvt_index];
        }

        // Max: Update only if new value is higher
        if (latest_value > base_values[pvt_index])
        {
            *expected_max = latest_value;
        }
        else
        {
            *expected_max = base_values[pvt_index];
        }

        // Average: Calculate running average
        uint32_t total = base_values[pvt_index]; // Start with base value
        for (int32_t i = 1; i <= iteration; i++)
        {
            total += (base_values[pvt_index] + i); // Add each value up to current iteration
        }
        *expected_avg = (uint16_t)((total + (iteration + 1) / 2) / (iteration + 1)); // Round to nearest integer
    }
}

TEST_FUNCTION(test_soc_pvt_temp_collection_functional, test_setup, test_teardown)
{
    const test_pvt_temp_config_t temperature_data_sets[NO_OF_ITERATIONS] = {
        test_pvt_temp_config_t{{20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14, 44}},
        test_pvt_temp_config_t{{21, 31, 41, 51, 61, 27, 28, 22, 13, 22, 32, 14, 42, 15, 45}},
        test_pvt_temp_config_t{{22, 32, 42, 52, 62, 28, 29, 23, 14, 23, 33, 15, 43, 16, 46}},
        test_pvt_temp_config_t{{23, 33, 43, 53, 63, 29, 30, 24, 15, 24, 34, 16, 44, 17, 47}}};

    soc_pvt_temp_t mock_pvt_temp_data = {0};

    for (int32_t iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        // 1. Set up mock data
        mock_pvt_temp_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
        {
            mock_pvt_temp_data.sensor_temp_dC[pvt_index] = temperature_data_sets[iteration].sensor_temp_dC[pvt_index];
        }

        will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

        // 2. Set up polling expectations
        // Tile_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries

        // Current polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries

        // Voltage polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries

        // SoC vr rail temperature - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // more_entries

        // SoC vr rail current - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // more_entries

        // dimm sensor info - no data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries

        for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
        {
            // Set PVT temperature data from config
            mock_pvt_temp_data.sensor_temp_dC[pvt_index] = temperature_data_sets[iteration].sensor_temp_dC[pvt_index];
        }

        // Pvt_Temperature polling - with data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &mock_pvt_temp_data);

        // 3. Process the data
        data_proc_tlm_cmpnt_process_input_data();

        // 4. Create temperature record
        pwr_soc_record_sensor_temp_t pvt_temp_record;
        package_create_pwr_soc_sensor_temp_record(&pvt_temp_record);

        for (int32_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
        {
            uint16_t expected_min = 0;
            uint16_t expected_max = 0;
            uint16_t expected_avg = 0;

            get_expected_pvt_temp_values(iteration, pvt_index, &expected_min, &expected_max, &expected_avg);

            bool print_logs = false;
            if (print_logs)
            {
                printf("\n----------------------------------------\n");
                printf("PVT Sensor     : %d:\n", pvt_index);
                printf("Input value    : %d\n", mock_pvt_temp_data.sensor_temp_dC[pvt_index]);
                printf("Expected Min   : %d\n", expected_min);
                printf("Expected Max   : %d\n", expected_max);
                printf("Expected Avg   : %d\n", expected_avg);
                printf("Actual Min     : %d\n",
                       pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.min_dC);
                printf("Actual Max     : %d\n",
                       pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.max_dC);
                printf("Actual Avg     : %d\n",
                       pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.average_dC);
                printf("----------------------------------------\n");
            }

            // Verify all statistics
            assert_int_equal(pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.min_dC, expected_min);
            assert_int_equal(pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.max_dC, expected_max);
            assert_int_equal(pvt_temp_record.sensor_temp_collection[pvt_index].sensor_temp_element.average_dC, expected_avg);
        }
    }
}
