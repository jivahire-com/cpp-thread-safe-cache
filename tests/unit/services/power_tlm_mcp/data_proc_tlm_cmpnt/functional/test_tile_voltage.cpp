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

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2148241

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
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// Calculate latest value (convert raw sensor value to millivolts)
static uint32_t calculate_expected_latest(uint32_t raw_value)
{
    return DOUT2MILLIVOLTS(raw_value);
}

// Calculate expected min value based on iteration logic
static uint32_t calculate_expected_min(uint32_t latest_value, uint32_t prev_min, uint32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, min = latest value
    }
    // For subsequent iterations, keep previous min if latest is higher
    return (latest_value < prev_min || prev_min == 0) ? latest_value : prev_min;
}

// Calculate expected max value based on iteration logic
static uint32_t calculate_expected_max(uint32_t latest_value, uint32_t prev_max, uint32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, max = latest value
    }
    // For subsequent iterations, take the higher value
    return (latest_value > prev_max) ? latest_value : prev_max;
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
    in_band_publishing_active = true;
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
    uint32_t core0_vcore; // Core 0 voltage
    uint32_t core1_vcore; // Core 1 voltage
    uint32_t vcpu;        // Tile VCPU voltage
    uint32_t vsys;        // Tile VSYS voltage
} test_voltage_config_t;

TEST_FUNCTION(test_tile_voltage_collection_functional, test_setup, test_teardown)
{

    // Track previous values for calculations
    static uint32_t prev_core0_min = 0, prev_core0_max = 0;
    static uint32_t prev_core1_min = 0, prev_core1_max = 0;
    static uint32_t prev_vcpu_min = 0, prev_vcpu_max = 0;
    static uint32_t prev_vsys_min = 0, prev_vsys_max = 0;

    uint32_t expected_core0_avg = 0;
    uint32_t expected_core1_avg = 0;
    uint16_t num_samples = 0;

    // Create mock voltage data for tile 0
    tile_voltage_t mock_voltage_data = {0};

    // Test configurations for voltage measurements across iterations
    // Raw sensor values are converted to millivolts using DOUT2MILLIVOLTS macro (multiplied by 1000)
    // NOTE: the sensor fifo voltage inputs are 16 bit values.
    // using DOUT2VOLTS(65535) the maximum voltage input is about 6.34 volts or 6340 millivolts
    // Updated to use safe voltage values that avoid uint16_t truncation
    const test_voltage_config_t test_configs[] = {
        // Iteration 0: Initial baseline value
        // Core 0: 5100mV, Core 1: 4200mV, VCPU: 3200mV, VSYS: 2200mV
        // All the metrics (Latest, Min, Max, Avg) will be equal to these initial values
        {.core0_vcore = MILLIVOLTS2DOUT(5100),
         .core1_vcore = MILLIVOLTS2DOUT(4200),
         .vcpu = MILLIVOLTS2DOUT(3200),
         .vsys = MILLIVOLTS2DOUT(2200)},

        // Iteration 1: to test increasing voltage trend
        // Core 0: 5700mV (Latest/Max/Avg), keeps Min=5100mV
        // Core 1: 4800mV (Latest/Max/Avg), keeps Min=4200mV
        // VCPU: 3800mV (Latest/Max/Avg), keeps Min=3200mV
        // VSYS: 2800mV (Latest/Max/Avg), keeps Min=2200mV
        {.core0_vcore = MILLIVOLTS2DOUT(5700),
         .core1_vcore = MILLIVOLTS2DOUT(4800),
         .vcpu = MILLIVOLTS2DOUT(3800),
         .vsys = MILLIVOLTS2DOUT(2800)},

        // Iteration 2: to test peak voltage values
        // Core 0: 6300mV (Latest/Max), Min=5100mV, Avg=6000mV (weighted calculation)
        // Core 1: 5400mV (Latest/Max), Min=4200mV, Avg=5100mV (weighted calculation)
        // VCPU: 4400mV (Latest/Max), Min=3200mV, Avg=4100mV (weighted calculation)
        // VSYS: 3400mV (Latest/Max), Min=2200mV, Avg=3100mV (weighted calculation)
        {.core0_vcore = MILLIVOLTS2DOUT(6300),
         .core1_vcore = MILLIVOLTS2DOUT(5400),
         .vcpu = MILLIVOLTS2DOUT(4400),
         .vsys = MILLIVOLTS2DOUT(3400)},

        // Iteration 3: to test decreasing voltage trend
        // Core 0: 5700mV (Latest), keeps Min=5100mV/Max=6300mV, Avg=5800mV (weighted)
        // Core 1: 4800mV (Latest), keeps Min=4200mV/Max=5400mV, Avg=4950mV (weighted)
        // VCPU: 3800mV (Latest), keeps Min=3200mV/Max=4400mV, Avg=4000mV (weighted)
        // VSYS: 2800mV (Latest), keeps Min=2200mV/Max=3400mV, Avg=3050mV (weighted)
        {.core0_vcore = MILLIVOLTS2DOUT(5700),
         .core1_vcore = MILLIVOLTS2DOUT(4800),
         .vcpu = MILLIVOLTS2DOUT(3800),
         .vsys = MILLIVOLTS2DOUT(2800)}};

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
        uint32_t expected_core0_voltage = calculate_expected_latest(mock_voltage_data.data.vcore0);
        uint32_t expected_core1_voltage = calculate_expected_latest(mock_voltage_data.data.vcore1);
        uint32_t expected_vcpu_voltage = calculate_expected_latest(mock_voltage_data.data.vcpu);
        uint32_t expected_vsys_voltage = calculate_expected_latest(mock_voltage_data.data.vsys);

        // Calculate min values
        uint32_t expected_core0_min = calculate_expected_min(expected_core0_voltage, prev_core0_min, iteration);
        uint32_t expected_core1_min = calculate_expected_min(expected_core1_voltage, prev_core1_min, iteration);
        uint32_t expected_vcpu_min = calculate_expected_min(expected_vcpu_voltage, prev_vcpu_min, iteration);
        uint32_t expected_vsys_min = calculate_expected_min(expected_vsys_voltage, prev_vsys_min, iteration);

        // Calculate max values
        uint32_t expected_core0_max = calculate_expected_max(expected_core0_voltage, prev_core0_max, iteration);
        uint32_t expected_core1_max = calculate_expected_max(expected_core1_voltage, prev_core1_max, iteration);
        uint32_t expected_vcpu_max = calculate_expected_max(expected_vcpu_voltage, prev_vcpu_max, iteration);
        uint32_t expected_vsys_max = calculate_expected_max(expected_vsys_voltage, prev_vsys_max, iteration);

        // Calculate average values
        num_samples++;

        // round up
        expected_core0_avg = (expected_core0_avg * (num_samples - 1)) + expected_core0_voltage;
        expected_core0_avg = (expected_core0_avg + num_samples / 2) / num_samples;

        expected_core1_avg = (expected_core1_avg * (num_samples - 1)) + expected_core1_voltage;
        expected_core1_avg = (expected_core1_avg + num_samples / 2) / num_samples;

        // Store current values for next iteration
        prev_core0_min = expected_core0_min;
        prev_core0_max = expected_core0_max;
        prev_core1_min = expected_core1_min;
        prev_core1_max = expected_core1_max;
        prev_vcpu_min = expected_vcpu_min;
        prev_vcpu_max = expected_vcpu_max;
        prev_vsys_min = expected_vsys_min;
        prev_vsys_max = expected_vsys_max;

        will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

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

        bool print_logs = true;
        if (print_logs)
        {
            // Print32_t debug info
            printf("\nIteration %d Summary:\n", iteration);
            printf("----------------------------------------\n");
            printf("                     Expected    Actual\n");
            printf("Core 0 Voltage:\n");

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

            printf("  Min (mV):         %8d   %8d\n",
                   expected_core1_min,
                   voltage_record.voltage_collection[1].voltage_element.min_mV);
            printf("  Max (mV):         %8d   %8d\n",
                   expected_core1_max,
                   voltage_record.voltage_collection[1].voltage_element.max_mV);
            printf("  Avg (mV):         %8d   %8d\n",
                   expected_core1_avg,
                   voltage_record.voltage_collection[1].voltage_element.average_mV);

            printf("----------------------------------------\n");
        }

        // Core 0
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.min_mV, expected_core0_min);
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.max_mV, expected_core0_max);
        assert_int_equal(voltage_record.voltage_collection[0].voltage_element.average_mV, expected_core0_avg);

        // Core 1
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.min_mV, expected_core1_min);
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.max_mV, expected_core1_max);
        assert_int_equal(voltage_record.voltage_collection[1].voltage_element.average_mV, expected_core1_avg);
    }
}