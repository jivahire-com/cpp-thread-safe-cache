//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tile_temperature.cpp
 * Test functionality and flow power_telemetry tile temperature collection
 *
* Step-1:
* Create and initialize temperature data for tile 0
* Set valid temperature values for both cores in tile 0

* Step-2:
* Process the data:
* Call data aggregation function to process the mocked sensor data
* Update internal data structures with temperature values

* Step-3:
* Create temperature record:
* Package the processed temperature data into a record structure
* Record contains temperature data for both cores

* Step-4:
* Verify the results:
* Check number of collections and provider IDs
* Verify temperature values (latest, min, average) for each core (latest should be max)
* Ensure values match expected calculations

* Record structs are in telemetry_package_defs.h
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2148230

#include "telemetry_functional.h"

#include <CMockaWrapper.h>

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
#include <tlm_fuses.h>
}

#define TEST_TEMP_CEL_2_DOUT(temp) \
    (TLM_FUSE_TEMP_CEL_2_DOUT((temp), DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL))

#define CELSIUS_TO_DECI_CELSIUS(temp) ((temp) * 10) // Convert Celsius to deciCelsius

#define ROUND_UP_CELSIUS_TO_DECI_CELSIUS(temp) ((uint16_t)((((temp) * 10.0) + 0.5)))

#define assert_uint16_within(expected, actual, tolerance)                 \
    assert_true((int32_t)(actual) >= (int32_t)(expected) - (tolerance) && \
                (int32_t)(actual) <= (int32_t)(expected) + (tolerance))

// Structure to hold test temperature configurations
typedef struct
{
    uint16_t core0_temp0;
    uint16_t core0_temp1;
    uint16_t core0_temp2;
    uint16_t core1_temp3;
    uint16_t core1_temp4;
    uint16_t core1_temp5;
} test_temp_config_t;

// Helper functions for min/max calculations
static inline int32_t max3(int32_t a, int32_t b, int32_t c)
{
    int32_t max = a;
    if (b > max)
        max = b;
    if (c > max)
        max = c;
    return max;
}

// Calculate latest value (max of three temperatures)
static int32_t calculate_expected_latest(int32_t temp0, int32_t temp1, int32_t temp2)
{
    return max3(temp0, temp1, temp2);
}

// Calculate expected min value based on iteration logic
static int32_t calculate_expected_min(int32_t latest_value, int32_t prev_min, int32_t iteration)
{
    if (iteration == 0)
    {
        return latest_value; // First iteration, min = latest value
    }
    // For subsequent iterations, keep previous min if latest is higher
    return prev_min; // In our test cases, prev_min is always kept
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
    int32_t total = (prev_avg * iteration) + latest_value;
    int32_t updated_avg = (total + (iteration + 1) / 2) / (iteration + 1);

    return updated_avg;
}

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0);

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

TEST_FUNCTION(test_tile_temperature_collection_functional, test_setup, test_teardown)
{
    // Track previous values for calculations
    static int32_t prev_core0_min_cel = 0, prev_core0_max_cel = 0, prev_core0_avg_dC = 0;
    static int32_t prev_core1_min_cel = 0, prev_core1_max_cel = 0, prev_core1_avg_dC = 0;

    // Create and initialize mock temperature data for tile 0
    tile_temp_t mock_temp_data = {0};

    // Define different temperature sets for testing
    // Defining different temperature sets for testing
    // Each configuration tries to test a specific scenario:
    //
    // Iteration 0 - Initial Values:
    // Core 0: [70,75,72] -> Latest=75 (max), Min=75, Max=75, Avg=75
    // Core 1: [80,85,82] -> Latest=85 (max), Min=85, Max=85, Avg=85
    // Purpose: Establishing a baseline starting values; all values equal the latest (max) value
    //
    // Iteration 1 - Increasing Trend based values:
    // Core 0: [80,85,82] -> Latest=85, Min=75 (kept), Max=85 (updated), Avg=85
    // Core 1: [90,95,92] -> Latest=95, Min=85 (kept), Max=95 (updated), Avg=95
    // Purpose: Tests handling of increasing temperatures and min value retention when newer readings are higher than current minimum
    //
    // Iteration 2 - Peak Values:
    // Core 0: [90,95,92] -> Latest=95, Min=75 (kept), Max=95 (updated), Avg=90
    // Core 1: [100,105,102] -> Latest=105, Min=85 (kept), Max=105 (updated), Avg=100
    // Purpose: Tests system behavior at maximum temperatures and weighted average calculation
    //
    // Iteration 3 - Decreasing Trend based values:
    // Core 0: [75,80,77] -> Latest=80, Min=75 (kept), Max=95 (kept), Avg=86
    // Core 1: [85,90,87] -> Latest=90, Min=85 (kept), Max=105 (kept), Avg=96
    // Purpose: Validates handling of temperature decrease and weighted average updates
    const test_temp_config_t test_configs[] = {
        {.core0_temp0 = 70, .core0_temp1 = 75, .core0_temp2 = 72, .core1_temp3 = 80, .core1_temp4 = 85, .core1_temp5 = 82},
        {.core0_temp0 = 80, .core0_temp1 = 85, .core0_temp2 = 82, .core1_temp3 = 90, .core1_temp4 = 95, .core1_temp5 = 92},
        {.core0_temp0 = 90, .core0_temp1 = 95, .core0_temp2 = 92, .core1_temp3 = 100, .core1_temp4 = 105, .core1_temp5 = 102},
        {.core0_temp0 = 75, .core0_temp1 = 80, .core0_temp2 = 77, .core1_temp3 = 85, .core1_temp4 = 90, .core1_temp5 = 87},
    };

    for (int32_t iteration = 0; iteration < 4; iteration++)
    {
        // Let the timestamp function handle timing - it adds 1000μs each time
        // Starting from 100, so timestamps will be: 1100, 2100, 3100, 4100
        mock_temp_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();

        // Set temperature data from config
        mock_temp_data.temp1.temp0 = test_configs[iteration].core0_temp0;
        mock_temp_data.temp1.temp1 = test_configs[iteration].core0_temp1;
        mock_temp_data.temp1.temp2 = test_configs[iteration].core0_temp2;
        mock_temp_data.temp1.temp3 = test_configs[iteration].core1_temp3;
        mock_temp_data.temp2.temp4 = test_configs[iteration].core1_temp4;
        mock_temp_data.temp2.temp5 = test_configs[iteration].core1_temp5;

        // Calculate latest values
        int32_t expected_core0_latest_cel = calculate_expected_latest(mock_temp_data.temp1.temp0,
                                                                      mock_temp_data.temp1.temp1,
                                                                      mock_temp_data.temp1.temp2);

        int32_t expected_core1_latest_cel = calculate_expected_latest(mock_temp_data.temp1.temp3,
                                                                      mock_temp_data.temp2.temp4,
                                                                      mock_temp_data.temp2.temp5);

        // Calculate min values
        int32_t expected_core0_min_cel = calculate_expected_min(expected_core0_latest_cel, prev_core0_min_cel, iteration);
        int32_t expected_core1_min_cel = calculate_expected_min(expected_core1_latest_cel, prev_core1_min_cel, iteration);

        // Calculate max values
        int32_t expected_core0_max_cel = calculate_expected_max(expected_core0_latest_cel, prev_core0_max_cel, iteration);
        int32_t expected_core1_max_cel = calculate_expected_max(expected_core1_latest_cel, prev_core1_max_cel, iteration);

        // Calculate average values
        int32_t expected_core0_avg_dC =
            calculate_expected_avg(expected_core0_latest_cel * 10, prev_core0_avg_dC, iteration);
        int32_t expected_core1_avg_dC =
            calculate_expected_avg(expected_core1_latest_cel * 10, prev_core1_avg_dC, iteration);

        // Store current values for next iteration
        prev_core0_min_cel = expected_core0_min_cel;
        prev_core0_max_cel = expected_core0_max_cel;
        prev_core0_avg_dC = expected_core0_avg_dC;
        prev_core1_min_cel = expected_core1_min_cel;
        prev_core1_max_cel = expected_core1_max_cel;
        prev_core1_avg_dC = expected_core1_avg_dC;

        // Set valid bits and max temperature data
        mock_temp_data.temp0.temp_valid = 1;
        mock_temp_data.temp0.max_temp = max3(expected_core0_max_cel, expected_core1_max_cel, INT_MIN);
        mock_temp_data.temp0.max_id = 4;

        // Now that we have the mock temp data setup as celsius, convert it to DOUT to reflect what we expect from SCF RAM.
        mock_temp_data.temp0.max_temp = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp0.max_temp);
        mock_temp_data.temp1.temp0 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp1.temp0);
        mock_temp_data.temp1.temp1 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp1.temp1);
        mock_temp_data.temp1.temp2 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp1.temp2);
        mock_temp_data.temp1.temp3 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp1.temp3);
        mock_temp_data.temp2.temp4 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp2.temp4);
        mock_temp_data.temp2.temp5 = TEST_TEMP_CEL_2_DOUT(mock_temp_data.temp2.temp5);

        will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

        // Set up expectations for tile 0
        // Temperature polling - with data
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);               // tile_index = 0
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);            // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false);           // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &mock_temp_data); // temperature_data pointer

        // Voltage polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries

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

        // Process the data
        data_proc_tlm_cmpnt_process_input_data();

        // Create temperature record
        pwr_core_record_temperature_t temperature_record;
        package_create_pwr_core_temperature_record(&temperature_record);

        // // Printing current sensor values and package results in a cleaner format
        bool print_logs = true;
        if (print_logs)
        {
            printf("\nIteration %d Summary:\n", iteration);
            printf("----------------------------------------\n");
            printf("Current Sensor Readings:\n");
            printf("Core 0: %d, %d, %d\n",
                   mock_temp_data.temp1.temp0,
                   mock_temp_data.temp1.temp1,
                   mock_temp_data.temp1.temp2);
            printf("Core 1: %d, %d, %d\n",
                   mock_temp_data.temp1.temp3,
                   mock_temp_data.temp2.temp4,
                   mock_temp_data.temp2.temp5);

            printf("\nPackaged Temperature Values:\n");
            printf("Core 0:\n");

            printf("  Min   : %d C %s\n",
                   temperature_record.temperature_collection[0].temperature_element.min_dC,
                   temperature_record.temperature_collection[0].temperature_element.min_dC ==
                           ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core0_min_cel)
                       ? "PASS"
                       : "FAIL");

            printf("  Max   : %d C %s\n",
                   temperature_record.temperature_collection[0].temperature_element.max_dC,
                   temperature_record.temperature_collection[0].temperature_element.max_dC ==
                           ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core0_max_cel)
                       ? "PASS"
                       : "FAIL");

            printf("  Avg   : %d C %s\n",
                   temperature_record.temperature_collection[0].temperature_element.average_dC,
                   temperature_record.temperature_collection[0].temperature_element.average_dC == expected_core0_avg_dC
                       ? "PASS"
                       : "FAIL");

            printf("\nCore 1:\n");

            printf("  Min   : %d C %s\n",
                   temperature_record.temperature_collection[1].temperature_element.min_dC,
                   temperature_record.temperature_collection[1].temperature_element.min_dC ==
                           ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core1_min_cel)
                       ? "PASS"
                       : "FAIL");

            printf("  Max   : %d C %s\n",
                   temperature_record.temperature_collection[1].temperature_element.max_dC,
                   temperature_record.temperature_collection[1].temperature_element.max_dC ==
                           ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core1_max_cel)
                       ? "PASS"
                       : "FAIL");

            printf("  Avg   : %d C %s\n",
                   temperature_record.temperature_collection[1].temperature_element.average_dC,
                   temperature_record.temperature_collection[1].temperature_element.average_dC == expected_core1_avg_dC
                       ? "PASS"
                       : "FAIL");

            printf("----------------------------------------\n");

            // Add debug prints to verify conditions
            printf("\nDebug Information for Iteration %d:\n", iteration);
            printf("----------------------------------------\n");
            printf("Time and Average Values:\n");
            printf("Core 0:\n");
            printf("  previous_average: %d\n",
                   temperature_record.temperature_collection[0].temperature_element.average_dC);
            printf("  latest_value: %d\n", ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core0_max_cel));

            if (iteration > 0)
            {
                uint64_t prev_timestamp = mock_temp_data.timestamp - 1000;
                printf("  Weighted Average Calculation:\n");
                printf("    weighted_prev_avg = %d * %llu = %llu\n",
                       temperature_record.temperature_collection[0].temperature_element.average_dC,
                       prev_timestamp,
                       (uint64_t)temperature_record.temperature_collection[0].temperature_element.average_dC * prev_timestamp);
                printf("    weighted_latest = %d * 1000 = %d\n", expected_core0_max_cel, expected_core0_max_cel * 1000);
                printf("    calculated_avg = (%llu + %d) / %llu = %d\n",
                       (uint64_t)temperature_record.temperature_collection[0].temperature_element.average_dC * prev_timestamp,
                       expected_core0_max_cel * 1000,
                       mock_temp_data.timestamp,
                       expected_core0_avg_dC);
            }

            printf("\nCore 1:\n");
            printf("  previous_average: %d\n",
                   temperature_record.temperature_collection[1].temperature_element.average_dC);
            printf("  latest_value: %d\n", expected_core1_max_cel);

            if (iteration > 0)
            {
                uint64_t prev_timestamp = mock_temp_data.timestamp - 1000;
                printf("  Weighted Average Calculation:\n");
                printf("    weighted_prev_avg = %d * %llu = %llu\n",
                       temperature_record.temperature_collection[1].temperature_element.average_dC,
                       prev_timestamp,
                       (uint64_t)temperature_record.temperature_collection[1].temperature_element.average_dC * prev_timestamp);
                printf("    weighted_latest = %d * 1000 = %d\n", expected_core1_max_cel, expected_core1_max_cel * 1000);
                printf("    calculated_avg = (%llu + %d) / %llu = %d\n",
                       (uint64_t)temperature_record.temperature_collection[1].temperature_element.average_dC * prev_timestamp,
                       expected_core1_max_cel * 1000,
                       mock_temp_data.timestamp,
                       expected_core1_avg_dC);
            }
        }

        assert_int_equal(temperature_record.temperature_collection[0].temperature_element.min_dC,
                         ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core0_min_cel));

        assert_int_equal(temperature_record.temperature_collection[0].temperature_element.max_dC,
                         ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core0_max_cel));
        // average_dC: Calculated using weighted average when conditions are met
        assert_uint16_within(expected_core0_avg_dC,
                             temperature_record.temperature_collection[0].temperature_element.average_dC,
                             10); // Allow tolerance for rounding errors

        // core 1 assertions
        assert_int_equal(temperature_record.temperature_collection[1].temperature_element.min_dC,
                         ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core1_min_cel));
        assert_int_equal(temperature_record.temperature_collection[1].temperature_element.max_dC,
                         ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_core1_max_cel));
        assert_uint16_within(expected_core1_avg_dC,
                             temperature_record.temperature_collection[1].temperature_element.average_dC,
                             10); // Allow tolerance for rounding errors
    }
}
