//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tile_voltage.cpp
 * Test functionality and flow power_telemetry tile voltage collection
 *
 * Step-1:
 * Create and initialize voltage data for tile 0
 * Set valid voltage values with different values for core0, core1, VCPU, and VSYS in tile 0
 * Raw sensor values are converted to millivolts using DOUT2MILLIVOLTS macro
 *
 * Step-2:
 * Process the data:
 * Call data aggregation function to process the mocked sensor data
 * Update internal data structures with voltage values
 * Calculate min, max, and average values for each voltage measurement
 *
 * Step-3:
 * Create voltage record:
 * Package the processed voltage data into a record structure
 * Record contains voltage data for both cores, VCPU, and VSYS
 *
 * Step-4:
 * Verify the results:
 * Check voltage values (latest, min, max, average) for each core
 * Verify VCPU and VSYS voltage measurements
 * Ensure values match expected calculations based on weighted averages
 * Validate min/max value retention across iterations
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
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// Calculate latest value (convert raw sensor value to millivolts)
static int32_t calculate_expected_latest(int32_t raw_value)
{
    return DOUT2MILLIVOLTS(raw_value);
}

// Calculate expected min value based on iteration logic
static int32_t calculate_expected_min(int32_t latest_value, int32_t prev_min, int32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, min = latest value
    }
    // For subsequent iterations, keep previous min if latest is higher
    return (latest_value < prev_min || prev_min == 0) ? latest_value : prev_min;
}

// Calculate expected max value based on iteration logic
static int32_t calculate_expected_max(int32_t latest_value, int32_t prev_max, int32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, max = latest value
    }
    // For subsequent iterations, take the higher value
    return (latest_value > prev_max) ? latest_value : prev_max;
}

// Calculate expected average based on iteration logic
static int32_t calculate_expected_avg(int32_t latest_value, int32_t prev_avg, int32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, avg = latest value
    }
    if (iteration == 1)
    {
        return latest_value; // For iteration 1, return latest value directly
    }

    // For iteration 2:
    // time_counter_uS / residency_uS: 8000, time_diff_uS: 4000
    if (iteration == 2)
    {
        uint64_t time_diff_uS = 4000;
        uint64_t total_time = 8000;

        // Calculate weighted contributions
        uint64_t weighted_prev_avg = static_cast<uint64_t>(prev_avg) * time_diff_uS;
        uint64_t weighted_latest = static_cast<uint64_t>(latest_value) * time_diff_uS;

        // Calculate total and round to nearest integer
        return static_cast<int32_t>((weighted_prev_avg + weighted_latest) / total_time);
    }

    // For iteration 3:
    // time_counter_uS / residency_uS: 12000, time_diff_uS: 4000
    if (iteration == 3)
    {
        uint64_t prev_time = 8000;
        uint64_t time_diff_uS = 4000;
        uint64_t total_time = 12000;

        // Calculate weighted contributions
        uint64_t weighted_prev_avg = static_cast<uint64_t>(prev_avg) * prev_time;
        uint64_t weighted_latest = static_cast<uint64_t>(latest_value) * time_diff_uS;

        // Calculate total and round to nearest integer
        return static_cast<int32_t>((weighted_prev_avg + weighted_latest) / total_time);
    }

    return prev_avg; // Default case
}

// Add these function declarations
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

// Structure to hold test voltage configurations
typedef struct
{
    int32_t core0_vcore; // Core 0 voltage
    int32_t core1_vcore; // Core 1 voltage
    int32_t vcpu;        // Tile VCPU voltage
    int32_t vsys;        // Tile VSYS voltage
} test_voltage_config_t;

TEST_FUNCTION(test_tile_voltage_collection_functional, test_setup, test_teardown)
{
    // Track previous values for calculations
    static int32_t prev_core0_min = 0, prev_core0_max = 0, prev_core0_avg = 0;
    static int32_t prev_core1_min = 0, prev_core1_max = 0, prev_core1_avg = 0;
    static int32_t prev_vcpu_min = 0, prev_vcpu_max = 0, prev_vcpu_avg = 0;
    static int32_t prev_vsys_min = 0, prev_vsys_max = 0, prev_vsys_avg = 0;

    // Create mock voltage data for tile 0
    tile_voltage_t mock_voltage_data = {0};

    // Test configurations for voltage measurements across iterations
    // Raw sensor values are converted to millivolts using DOUT2MILLIVOLTS macro (multiplied by 1000)
    const test_voltage_config_t test_configs[] = {
        // Iteration 0: Initial baseline value
        // Core 0: 16 -> 16000mV, Core 1: 13 -> 13000mV, VCPU: 10 -> 10000mV, VSYS: 7 -> 7000mV
        // All the metrics (Latest, Min, Max, Avg) will be equal to these initial values
        {.core0_vcore = 16, .core1_vcore = 13, .vcpu = 10, .vsys = 7},

        // Iteration 1: to test increasing voltage trend
        // Core 0: 18 -> 18000mV (Latest/Max/Avg), keeps Min=16000
        // Core 1: 15 -> 15000mV (Latest/Max/Avg), keeps Min=13000
        // VCPU: 12 -> 12000mV (Latest/Max/Avg), keeps Min=10000
        // VSYS: 9 -> 9000mV (Latest/Max/Avg), keeps Min=7000
        {.core0_vcore = 18, .core1_vcore = 15, .vcpu = 12, .vsys = 9},

        // Iteration 2: to test peak voltage values
        // Core 0: 20 -> 20000mV (Latest/Max), Min=16000, Avg=19000 (weighted calculation)
        // Core 1: 17 -> 17000mV (Latest/Max), Min=13000, Avg=16000 (weighted calculation)
        // VCPU: 14 -> 14000mV (Latest/Max), Min=10000, Avg=13000 (weighted calculation)
        // VSYS: 11 -> 11000mV (Latest/Max), Min=7000, Avg=10000 (weighted calculation)
        {.core0_vcore = 20, .core1_vcore = 17, .vcpu = 14, .vsys = 11},

        // Iteration 3: to test decreasing voltage trend
        // Core 0: 18 -> 18000mV (Latest), keeps Min=16000/Max=20000, Avg=18666 (weighted)
        // Core 1: 15 -> 15000mV (Latest), keeps Min=13000/Max=17000, Avg=15666 (weighted)
        // VCPU: 12 -> 12000mV (Latest), keeps Min=10000/Max=14000, Avg=12666 (weighted)
        // VSYS: 9 -> 9000mV (Latest), keeps Min=7000/Max=11000, Avg=9666 (weighted)
        {.core0_vcore = 18, .core1_vcore = 15, .vcpu = 12, .vsys = 9}};

    for (int32_t iteration = 0; iteration < 4; iteration++)
    {
        // The timestamp function handles the timing
        mock_voltage_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();

        // Set voltage data from config
        mock_voltage_data.data.vcore0 = test_configs[iteration].core0_vcore;
        mock_voltage_data.data.vcore1 = test_configs[iteration].core1_vcore;
        mock_voltage_data.data.vcpu = test_configs[iteration].vcpu;
        mock_voltage_data.data.vsys = test_configs[iteration].vsys;

        // Calculate latest values
        int32_t expected_core0_voltage = calculate_expected_latest(mock_voltage_data.data.vcore0);
        int32_t expected_core1_voltage = calculate_expected_latest(mock_voltage_data.data.vcore1);
        int32_t expected_vcpu_voltage = calculate_expected_latest(mock_voltage_data.data.vcpu);
        int32_t expected_vsys_voltage = calculate_expected_latest(mock_voltage_data.data.vsys);

        // Calculate min values
        int32_t expected_core0_min = calculate_expected_min(expected_core0_voltage, prev_core0_min, iteration);
        int32_t expected_core1_min = calculate_expected_min(expected_core1_voltage, prev_core1_min, iteration);
        int32_t expected_vcpu_min = calculate_expected_min(expected_vcpu_voltage, prev_vcpu_min, iteration);
        int32_t expected_vsys_min = calculate_expected_min(expected_vsys_voltage, prev_vsys_min, iteration);

        // Calculate max values
        int32_t expected_core0_max = calculate_expected_max(expected_core0_voltage, prev_core0_max, iteration);
        int32_t expected_core1_max = calculate_expected_max(expected_core1_voltage, prev_core1_max, iteration);
        int32_t expected_vcpu_max = calculate_expected_max(expected_vcpu_voltage, prev_vcpu_max, iteration);
        int32_t expected_vsys_max = calculate_expected_max(expected_vsys_voltage, prev_vsys_max, iteration);

        // Calculate average values
        int32_t expected_core0_avg = calculate_expected_avg(expected_core0_voltage, prev_core0_avg, iteration);
        int32_t expected_core1_avg = calculate_expected_avg(expected_core1_voltage, prev_core1_avg, iteration);
        int32_t expected_vcpu_avg = calculate_expected_avg(expected_vcpu_voltage, prev_vcpu_avg, iteration);
        int32_t expected_vsys_avg = calculate_expected_avg(expected_vsys_voltage, prev_vsys_avg, iteration);

        // Store current values for next iteration
        prev_core0_min = expected_core0_min;
        prev_core0_max = expected_core0_max;
        prev_core0_avg = expected_core0_avg;
        prev_core1_min = expected_core1_min;
        prev_core1_max = expected_core1_max;
        prev_core1_avg = expected_core1_avg;
        prev_vcpu_min = expected_vcpu_min;
        prev_vcpu_max = expected_vcpu_max;
        prev_vcpu_avg = expected_vcpu_avg;
        prev_vsys_min = expected_vsys_min;
        prev_vsys_max = expected_vsys_max;
        prev_vsys_avg = expected_vsys_avg;

        // Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries

        // Current polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries

        // SoC vr rail temperature - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // more_entries

        // SoC vr rail current - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // more_entries

        // dimm sensor info - no data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries

        // Pvt_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

        // Voltage polling - with data
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &mock_voltage_data);

        // Process the data
        data_proc_tlm_cmpnt_process_input_data();

        // Create voltage record
        pwr_core_record_voltage_t voltage_record;
        package_create_pwr_core_voltage_record(&voltage_record);

        bool print_logs = false;
        if (print_logs)
        {
            // Print32_t debug info
            printf("\nIteration %d Summary:\n", iteration);
            printf("----------------------------------------\n");
            printf("                     Expected    Actual\n");
            printf("Core 0 Voltage:\n");
            printf("  Latest (mV):      %8d   %8d\n",
                   expected_core0_voltage,
                   voltage_record.voltage_collection[0].voltage_element.latest_value_mV);
            printf("  Min (mV):         %8d   %8d\n",
                   expected_core0_min,
                   voltage_record.voltage_collection[0].voltage_element.min_mV);
            printf("  Max (mV):         %8d   %8d\n",
                   expected_core0_max,
                   voltage_record.voltage_collection[0].voltage_element.max_mV);
            printf("  Avg (mV):         %8d   %8d\n",
                   expected_core0_avg,
                   voltage_record.voltage_collection[0].voltage_element.average_mV);

            printf("\nCore 1 Voltage:\n");
            printf("  Latest (mV):      %8d   %8d\n",
                   expected_core1_voltage,
                   voltage_record.voltage_collection[1].voltage_element.latest_value_mV);
            printf("  Min (mV):         %8d   %8d\n",
                   expected_core1_min,
                   voltage_record.voltage_collection[1].voltage_element.min_mV);
            printf("  Max (mV):         %8d   %8d\n",
                   expected_core1_max,
                   voltage_record.voltage_collection[1].voltage_element.max_mV);
            printf("  Avg (mV):         %8d   %8d\n",
                   expected_core1_avg,
                   voltage_record.voltage_collection[1].voltage_element.average_mV);

            printf("\nTile VCPU Voltage:\n");
            printf("  Latest (mV):      %8d   %8d\n", expected_vcpu_voltage, tile[0].vcpu.latest_value_mV);
            printf("  Min (mV):         %8d   %8d\n", expected_vcpu_min, tile[0].vcpu.min_mV);
            printf("  Max (mV):         %8d   %8d\n", expected_vcpu_max, tile[0].vcpu.max_mV);
            printf("  Avg (mV):         %8d   %8d\n", expected_vcpu_avg, tile[0].vcpu.average_mV);

            printf("\nTile VSYS Voltage:\n");
            printf("  Latest (mV):      %8d   %8d\n", expected_vsys_voltage, tile[0].vsys.latest_value_mV);
            printf("  Min (mV):         %8d   %8d\n", expected_vsys_min, tile[0].vsys.min_mV);
            printf("  Max (mV):         %8d   %8d\n", expected_vsys_max, tile[0].vsys.max_mV);
            printf("  Avg (mV):         %8d   %8d\n", expected_vsys_avg, tile[0].vsys.average_mV);
            printf("----------------------------------------\n");
        }

        // Core 0
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.latest_value_mV, expected_core0_voltage);
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.min_mV, expected_core0_min);
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.max_mV, expected_core0_max);
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.average_mV, expected_core0_avg);

        // Core 1
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.latest_value_mV, expected_core1_voltage);
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.min_mV, expected_core1_min);
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.max_mV, expected_core1_max);
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.average_mV, expected_core1_avg);

        // VCPU
        assert_int_equal(tile[0].vcpu.latest_value_mV, expected_vcpu_voltage);
        assert_int_equal(tile[0].vcpu.min_mV, expected_vcpu_min);
        assert_int_equal(tile[0].vcpu.max_mV, expected_vcpu_max);
        assert_int_equal(tile[0].vcpu.average_mV, expected_vcpu_avg);

        // VSYS
        assert_int_equal(tile[0].vsys.latest_value_mV, expected_vsys_voltage);
        assert_int_equal(tile[0].vsys.min_mV, expected_vsys_min);
        assert_int_equal(tile[0].vsys.max_mV, expected_vsys_max);
        assert_int_equal(tile[0].vsys.average_mV, expected_vsys_avg);
    }
}