//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_log_soc_dimm_info.cpp
 * Test functionality and flow power_telemetry SoC dimm sensor info collection
 *
 * Test Steps:
 * -----------
 * Step-1:
 * Create and initialize DIMM sensor data
 * Set valid DIMM temperature, power, throttling, and frequency ID values for each DIMM module.
 * Initialize mock data structure with these values.
 *
 * Step-2:
 * Process the data
 * Call the data aggregation function to process the mocked DIMM sensor data.
 * Update internal data structures with the DIMM values.
 *
 * Step-3:
 * Create DIMM record
 * Package the processed DIMM data into a record structure.
 * The record contains the latest temperature, power, throttling, and frequency ID for each DIMM module.
 *
 * Step-4:
 * Verify the results
 * Check that the latest values for S0 and S1 temperature, power, throttling, and frequency ID match the input.
 * Assert that all values are copied as-is from the input, since there is no transformation, conversion, or aggregation logic for these fields in tlm_logger.c.
 * Validate and assert all values accordingly.
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
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
#include <tlm_logger_i.h>
}

#define NO_OF_ITERATIONS 4

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

// Helper function to calculate expected values based on iteration
static void calculate_expected_values(int32_t iteration,
                                      int32_t* s0_latest,
                                      int32_t* s1_latest,
                                      int32_t* power,
                                      int32_t* throttling,
                                      int32_t* frequency_id)
{
    // Since there's no transformation or aggregation logic, we are returning the input values based on iteration.
    switch (iteration)
    {
    case 0:
        *s0_latest = 26;
        *s1_latest = 28;
        *power = 100;
        *throttling = 0;
        *frequency_id = 0;
        break;
    case 1:
        *s0_latest = 30;
        *s1_latest = 32;
        *power = 120;
        *throttling = 0;
        *frequency_id = 1;
        break;
    case 2:
        *s0_latest = 35;
        *s1_latest = 38;
        *power = 150;
        *throttling = 1;
        *frequency_id = 2;
        break;
    case 3:
        *s0_latest = 40;
        *s1_latest = 42;
        *power = 180;
        *throttling = 1;
        *frequency_id = 3;
        break;
    default:
        // Default values are set to 0.
        *s0_latest = 0;
        *s1_latest = 0;
        *power = 0;
        *throttling = 0;
        *frequency_id = 0;
    }
}

TEST_FUNCTION(test_tlm_logger_log_dimm_information, test_setup, test_teardown)
{

    /* Note: dimm_id will less than NUMBER_OF_DIMM_MODULES(12) */
    // Expected values are set to the input values for each iteration.
    // As there is no transformation, conversion, or aggregation logic for these fields in tlm_logger,
    // they are copied as-is and validated and asserted accordingly.
    sensor_ram_dimm_info_t dimm_info[] = {
        {.dimm_temp_s0_dC = 26, .dimm_temp_s1_dC = 28, .dimm_power_mW = 100, .dimm_id = 0, .dimm_throttling = 0, .dimm_memory_frequency_id = 0},
        {.dimm_temp_s0_dC = 30, .dimm_temp_s1_dC = 32, .dimm_power_mW = 120, .dimm_id = 1, .dimm_throttling = 0, .dimm_memory_frequency_id = 1},
        {.dimm_temp_s0_dC = 35, .dimm_temp_s1_dC = 38, .dimm_power_mW = 150, .dimm_id = 2, .dimm_throttling = 1, .dimm_memory_frequency_id = 2},
        {.dimm_temp_s0_dC = 40, .dimm_temp_s1_dC = 42, .dimm_power_mW = 180, .dimm_id = 3, .dimm_throttling = 1, .dimm_memory_frequency_id = 3}};

    sensor_ram_dimm_info_t mock_dimm_data = {0};

    for (int32_t iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        mock_dimm_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        mock_dimm_data.dimm_temp_s0_dC = dimm_info[iteration].dimm_temp_s0_dC;
        mock_dimm_data.dimm_temp_s1_dC = dimm_info[iteration].dimm_temp_s1_dC;
        mock_dimm_data.dimm_power_mW = dimm_info[iteration].dimm_power_mW;
        mock_dimm_data.dimm_id = dimm_info[iteration].dimm_id;
        mock_dimm_data.dimm_throttling = dimm_info[iteration].dimm_throttling;
        mock_dimm_data.dimm_memory_frequency_id = dimm_info[iteration].dimm_memory_frequency_id;

        // Temperature polling - no data
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

        // Pvt_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

        // dimm sensor info - with data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &mock_dimm_data);

        // Process the data
        data_proc_tlm_cmpnt_aggregate_pwr_tlm_data();

        // Create dimm record
        pwr_soc_record_dimm_t dimm_record;
        package_create_pwr_soc_dimm_record(&dimm_record);

        // Calculate expected values using helper function
        int32_t expected_s0_latest = 0, expected_s1_latest = 0;
        int32_t expected_power = 0, expected_throttling = 0;
        int32_t expected_frequency_id = 0;

        calculate_expected_values(iteration, &expected_s0_latest, &expected_s1_latest, &expected_power, &expected_throttling, &expected_frequency_id);

        bool print_logs = true;
        if (print_logs)
        {
            printf("            Iteration - %d\n", iteration);
            printf("            Experted     Actual\n");
            printf("S0 latest      %d          %d\n",
                   expected_s0_latest,
                   dimm_record.dimm_collection[iteration].dimm_element.s0.latest_value_dC);
            printf("S1 latest      %d          %d\n",
                   expected_s1_latest,
                   dimm_record.dimm_collection[iteration].dimm_element.s1.latest_value_dC);
            printf("Frequency_id   %d           %d\n",
                   expected_frequency_id,
                   dimm_record.dimm_collection[iteration].dimm_element.dimm_memory_frequency_id);
            printf("Power          %d         %d\n",
                   expected_power,
                   dimm_record.dimm_collection[iteration].dimm_element.dimm_power_mW);
            printf("Throttling     %d           %d\n",
                   expected_throttling,
                   dimm_record.dimm_collection[iteration].dimm_element.dimm_throttling);
            printf("                         \n");
        }

        // Assertions using calculated expected values from function calculate_expected_values
        assert_int_equal(expected_s0_latest, dimm_record.dimm_collection[iteration].dimm_element.s0.latest_value_dC);
        assert_int_equal(expected_s1_latest, dimm_record.dimm_collection[iteration].dimm_element.s1.latest_value_dC);
        assert_int_equal(expected_frequency_id, dimm_record.dimm_collection[iteration].dimm_element.dimm_memory_frequency_id);
        assert_int_equal(expected_power, dimm_record.dimm_collection[iteration].dimm_element.dimm_power_mW);
        assert_int_equal(expected_throttling, dimm_record.dimm_collection[iteration].dimm_element.dimm_throttling);
    }
}