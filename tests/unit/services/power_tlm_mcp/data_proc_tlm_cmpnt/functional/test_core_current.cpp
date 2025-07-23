//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_current.cpp
 * Test functionality and flow of power_telemetry core current collection
 *
 * Step-1:
 * Create and initialize current data for core 0
 *   - Set valid current values (avg, min, max) for the core
 *   - Set voltage, power, and pstate information
 *   - Convert raw sensor values to milliamps using CORE_CURRENT_CONVERSION_FACTOR
 *   - Initialize mock data structure with these values
 *
 * Step-2:
 * Process the data
 *   - Call data aggregation function to process the mocked sensor data
 *   - Update internal data structures with current values
 *   - Process min/max/avg calculations:
 *     * Latest value = avg_current * CORE_CURRENT_CONVERSION_FACTOR
 *     * Min value updates when latest < current min or min == 0
 *     * Max value updates when latest > current max
 *     * Avg value uses weighted calculations based on time windows
 *
 * Step-3:
 * Create current record
 *   - Package the processed current data into a record structure
 *   - Contains current measurements with conversion factors applied
 *   - Includes latest, min, max, and average values in milliamps
 *
 * Step-4:
 * Verify the results
 *   - Check latest value matches converted average current
 *   - Verify minimum current values are correctly retained
 *   - Validate maximum current values are properly updated
 *   - Ensure average calculations follow the weighted average logic
 *   - Compare and assert all values against expected calculated values
 *
 * Record structs are in telemetry_package_defs.h
 */

// @SSI_functional_Test
// @SSI_functional_Test:power_telemetry

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// Add these function declarations
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();

    // enable all cores for these tests
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        core_is_active[core] = true;
    }

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

typedef struct
{
    int32_t avg;    // Average voltage
    int32_t min;    // Minimum voltage
    int32_t max;    // Maximum voltage
    int32_t volt;   // Voltage value
    int32_t pwr;    // Power value
    int32_t pstate; // Performance state
} test_config_t;

// Define different current sets for testing
// Each configuration tests a specific scenario:
//
// Iteration 0 - Initial Values:
// Raw values: avg=100, min=10, max=150
// Converted to mA (value * CORE_CURRENT_CONVERSION_FACTOR):
// Latest=2650mA (from avg), Min=2650mA, Max=2650mA, Avg=2650mA
// To test baseline starting values; all values equal the latest value
//
// Iteration 1 - Increasing Trend:
// Raw values: avg=110, min=20, max=160
// Converted to mA: Latest=2915mA, Min=265mA (kept), Max=4240mA (updated), Avg=2915mA
// To test handling of increasing currents and min values retention
//
// Iteration 2 - Peak Values:
// Raw values: avg=120, min=30, max=170
// Converted to mA: Latest=3180mA, Min=265mA (kept), Max=4505mA (updated), Avg=3180mA
// To test behavior at maximum currents and weighted average calculation
//
// Iteration 3 - Decreasing Trend:
// Raw values: avg=115, min=25, max=165
// Converted to mA: Latest=3047mA, Min=265mA (kept), Max=4505mA (kept), Avg=3047mA
// To test handling of current decrease and weighted average updates
const test_config_t test_configs[] = {{.avg = 5, .min = 5, .max = 5, .volt = 100, .pwr = 50, .pstate = 13},
                                      {.avg = 6, .min = 6, .max = 6, .volt = 110, .pwr = 60, .pstate = 14},
                                      {.avg = 7, .min = 7, .max = 7, .volt = 120, .pwr = 70, .pstate = 15},
                                      {.avg = 8, .min = 8, .max = 8, .volt = 115, .pwr = 65, .pstate = 14}};

// Calculate expected latest value (average current is used as latest value)
static int32_t calculate_expected_latest(int32_t avg_value)
{
    return avg_value * CORE_CURRENT_CONVERSION_FACTOR;
}

// Calculate expected min value based on iteration logic
static int32_t calculate_expected_min(int32_t min_value, int32_t prev_min, int32_t iteration)
{
    // Convert the raw min value to mA
    int32_t current_min_mA = min_value * CORE_CURRENT_CONVERSION_FACTOR;

    if (iteration == 0)
    {
        return current_min_mA; // First iteration, min = current min value
    }
    // For subsequent iterations, keep previous min if current is higher
    return (current_min_mA < prev_min || prev_min == 0) ? current_min_mA : prev_min;
}

// Calculate expected max value based on iteration logic
static int32_t calculate_expected_max(int32_t max_value, int32_t prev_max, int32_t iteration)
{
    // Convert the raw max value to mA
    int32_t current_max_mA = max_value * CORE_CURRENT_CONVERSION_FACTOR;

    if (iteration == 0)
    {
        return current_max_mA; // First iteration, max = current max value
    }
    // For subsequent iterations, take the higher value
    return (current_max_mA > prev_max) ? current_max_mA : prev_max;
}

// Calculate expected average based on iteration logic
static int32_t calculate_expected_avg(int32_t latest_value, int32_t prev_avg, int32_t iteration)
{
    // Convert the raw average value to mA
    latest_value = latest_value * CORE_CURRENT_CONVERSION_FACTOR;

    if (iteration == 0)
    {
        return latest_value; // First iteration, avg = latest value
    }
    int32_t total = (prev_avg * iteration) + latest_value;
    int32_t updated_avg = (total + (iteration + 1) / 2) / (iteration + 1);

    return updated_avg;
}

TEST_FUNCTION(test_core_current_collection_functional, test_setup, test_teardown)
{
    // Track previous values for calculations
    static int32_t prev_min = 0, prev_max = 0, prev_avg = 0;

    // Create mock current data for core 0
    core_current_t mock_current_data = {0};

    for (int32_t iteration = 0; iteration < 4; iteration++)
    {
        mock_current_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();

        mock_current_data.data.avg = test_configs[iteration].avg;
        mock_current_data.data.min = test_configs[iteration].min;
        mock_current_data.data.max = test_configs[iteration].max;
        mock_current_data.data.volt = test_configs[iteration].volt;
        mock_current_data.data.pwr = test_configs[iteration].pwr;
        mock_current_data.data.pstate = test_configs[iteration].pstate;
        mock_current_data.data.change = 0; // No change bit set in our test cases

        // Calculate expected values using helper functions
        int32_t expected_latest_mA = calculate_expected_latest(mock_current_data.data.avg);
        int32_t expected_min_mA = calculate_expected_min(mock_current_data.data.min, prev_min, iteration);
        int32_t expected_max_mA = calculate_expected_max(mock_current_data.data.max, prev_max, iteration);
        int32_t expected_average_mA = calculate_expected_avg(mock_current_data.data.avg, prev_avg, iteration);

        // Store current values for next iteration
        prev_min = expected_min_mA;
        prev_max = expected_max_mA;
        prev_avg = expected_average_mA;

        will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

        // Current polling - with data
        will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
        will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);     // core_index
        will_return(__wrap_sensor_fifo_svc_poll_core_current, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_core_current, &mock_current_data);

        // Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries
        // Voltage polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries

        // SoC vr rail current - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // more_entries

        // SoC vr rail temperature - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // more_entriess

        // dimm sensor info - no data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries

        // Pvt_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

        // Process the data
        data_proc_tlm_cmpnt_process_input_data();

        // Create Current record
        pwr_core_record_current_t current_record;
        package_create_pwr_core_current_record(&current_record);

        bool print_log = true;
        if (print_log)
        {
            printf("Iteration - %d\n", iteration);

            printf("              Expected    Actual  \n");
            printf("Min_mA:          %d         %d\n",
                   expected_min_mA,
                   current_record.current_collection->current_element.min_mA);
            printf("Max_mA:          %d        %d\n",
                   expected_max_mA,
                   current_record.current_collection->current_element.max_mA);
            printf("Average_mA:      %d        %d\n",
                   expected_average_mA,
                   current_record.current_collection->current_element.average_mA);
            printf("Latest_value_mA: %d        %d\n",
                   expected_latest_mA,
                   current_record.current_collection->current_element.latest_value_mA);
            printf("------------------------------------\n");
        }

        assert_int_equal(current_record.current_collection->current_element.latest_value_mA, expected_latest_mA);
        assert_int_equal(current_record.current_collection->current_element.min_mA, expected_min_mA);
        assert_int_equal(current_record.current_collection->current_element.max_mA, expected_max_mA);
        assert_int_equal(current_record.current_collection->current_element.average_mA, expected_average_mA);
    }
}
