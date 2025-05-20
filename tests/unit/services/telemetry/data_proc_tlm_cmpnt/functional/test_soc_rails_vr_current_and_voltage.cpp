//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_soc_rails_vr_current_and_voltage.cpp
 * Test functionality and flow of power_telemetry soc current and voltage collection
 *
 * Step-1:
 * Create and initialize test data for VR rails
 * - Set up current_data_sets array with known values for 8 VR rails
 * - Base current starts at 15mA and increments by 10mA for each rail
 * - Base voltage starts at 20mV and increments by 10mV for each rail
 * - Each iteration increments values by 1 to test min/max/avg behavior
 *
 * Step-2:
 * For each iteration (0-3):
 * - Copy test data to mock structure for current iteration
 * - Mock all sensor polling calls to return no data
 *   (temperature, voltage, core current, VR temperature, DIMM, PVT)
 * - Mock VR current polling to return valid test data
 * - Call data_proc_tlm_cmpnt_aggregate_pwr_tlm_data() to process values
 *
 * Step-3:
 * Create VR rail record:
 * - Package the processed data into pwr_soc_record_vr_rail_t structure
 * - Record contains current and voltage data for all 8 VR rails
 *
 * Step-4:
 * Verify for each rail (0-7):
 * Current verification:
 * - Iteration 0: All values equal base value (15 + index*10)
 * - Iteration 1: Latest/avg = base + 1, min = base, max = latest
 * - Iteration 2: Latest = base + 2, min = base, max = latest, avg = base + 1
 * - Iteration 3: Latest = base + 3, min = base, max = latest, avg = base + 1
 *
 * Voltage verification:
 * - Iteration 0: All values equal base value (20 + index*10)
 * - Iteration 1: Latest/avg = base + 1, min = base, max = latest
 * - Iteration 2: Latest = base + 2, min = base, max = latest, avg = base + 1
 * - Iteration 3: Latest = base + 3, min = base, max = latest, avg = base + 1
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
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
// #include <tlm_logger_i.h>
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

// Helper function to calculate expected current values based on iteration and index
static void calculate_expected_vr_current_values(int32_t iteration,
                                                 int32_t index,
                                                 int32_t* expected_min,
                                                 int32_t* expected_max,
                                                 int32_t* expected_avg,
                                                 int32_t* expected_latest)
{
    // Base values for each index from iteration 0
    int32_t base_current = 15 + (index * 10);

    // Get latest value from current_data_sets array pattern
    *expected_latest = base_current + iteration;

    if (iteration == 0)
    {
        *expected_min = base_current;
        *expected_max = base_current;
        *expected_avg = base_current;
    }
    else
    {
        *expected_min = base_current;     // Always keeps initial value as min
        *expected_max = *expected_latest; // Max is always the latest value

        // For average calculation:
        // In tlm_logger.c, tlm_calculate_mma_res() only uses weighted average when:
        // (residency_uS > time_diff_uS) && (residency_uS != 0) && (*mma_average != 0)
        // For VR current/voltage, these timing conditions cannot be met because:
        // 1. VR data comes from sensor polling without timestamps
        // 2. Unlike tile temperature or tile voltage which has update cycles, VR data is processed all most immediately
        // 3. And hence residency_uS never being greater than time_diff_uS
        if (iteration == 1)
        {
            *expected_avg = *expected_latest; // In first update, average equals latest
        }
        else
        {
            // For iterations 2 and 3, keeps the value from iteration 1 (base + 1)
            // This matches tlm_logger.c behavior where without timing conditions,
            // the average stays at the first non-zero value it receives
            *expected_avg = base_current + 1;
        }
    }
}

// Helper function to calculate expected voltage values based on iteration and index
static void calculate_expected_vr_voltage_values(int32_t iteration,
                                                 int32_t index,
                                                 int32_t* expected_min,
                                                 int32_t* expected_max,
                                                 int32_t* expected_avg,
                                                 int32_t* expected_latest)
{
    // Base values for each index from iteration 0
    int32_t base_voltage = 20 + (index * 10);

    // Get latest value from current_data_sets array pattern
    *expected_latest = base_voltage + iteration;

    if (iteration == 0)
    {
        *expected_min = base_voltage;
        *expected_max = base_voltage;
        *expected_avg = base_voltage;
    }
    else
    {
        *expected_min = base_voltage;     // Always keeps initial value as min
        *expected_max = *expected_latest; // Max is always the latest value

        // For average calculation:
        // In tlm_logger.c, tlm_calculate_mma_res() only uses weighted average when:
        // (residency_uS > time_diff_uS) && (residency_uS != 0) && (*mma_average != 0)
        // For VR current/voltage, these timing conditions cannot be met because:
        // 1. VR data comes from sensor polling without timestamps
        // 2. Unlike tile temperature or tile voltage which has update cycles, VR data is processed all most immediately
        // 3. And hence residency_uS never being greater than time_diff_uS
        if (iteration == 1)
        {
            *expected_avg = *expected_latest; // In first update, average equals latest
        }
        else
        {
            // For iterations 2 and 3, keeps the value from iteration 1 (base + 1)
            // This matches tlm_logger.c behavior where without timing conditions,
            // the average stays at the first non-zero value it receives
            *expected_avg = base_voltage + 1;
        }
    }
}

TEST_FUNCTION(test_soc_rails_vr_current_and_voltage_collection_functional, test_setup, test_teardown)
{
    vr_current_t mock_temp_data = {0};

    // Test data structure representing VR rail values for each iteration
    // Values chosen to:
    // 1. Have distinct base values for each rail (increment by 10) for easy tracking and validation
    // 2. Increment by 1 each iteration to test min/max/avg behavior
    // 3. Keep voltage values offset by +5 from current for clear separation
    struct
    {
        uint16_t vr_current_mA[MAX_NUM_OF_VR_RAILS];
        uint16_t vr_voltage_mV[MAX_NUM_OF_VR_RAILS];
    } current_data_sets[] = {{{15, 25, 35, 45, 55, 65, 75, 85}, {20, 30, 40, 50, 60, 70, 80, 90}},
                             {{16, 26, 36, 46, 56, 66, 76, 86}, {21, 31, 41, 51, 61, 71, 81, 91}},
                             {{17, 27, 37, 47, 57, 67, 77, 87}, {22, 32, 42, 52, 62, 72, 82, 92}},
                             {{18, 28, 38, 48, 58, 68, 78, 88}, {23, 33, 43, 53, 63, 73, 83, 93}}};

    for (int32_t iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        for (int32_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
        {
            mock_temp_data.vr_current_mA[index] = current_data_sets[iteration].vr_current_mA[index];
            mock_temp_data.vr_voltage_mV[index] = current_data_sets[iteration].vr_voltage_mV[index];
        }

        // Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries

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

        // dimm sensor info - no data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries

        // Pvt_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

        // VR current - with data
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, true);            // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false);           // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, &mock_temp_data); // data pointer

        data_proc_tlm_cmpnt_process_input_data();

        pwr_soc_record_vr_rail_t vr_rail_record;
        package_create_pwr_soc_vr_rail_record(&vr_rail_record);

        for (int32_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
        {
            // Calculate expected values for current
            int32_t expected_vr_current_min = 0;
            int32_t expected_vr_current_max = 0;
            int32_t expected_vr_current_avg = 0;
            int32_t expected_vr_current_latest = 0;

            calculate_expected_vr_current_values(iteration,
                                                 index,
                                                 &expected_vr_current_min,
                                                 &expected_vr_current_max,
                                                 &expected_vr_current_avg,
                                                 &expected_vr_current_latest);

            // Calculate expected values for voltage
            int32_t expected_vr_voltage_min = 0;
            int32_t expected_vr_voltage_max = 0;
            int32_t expected_vr_voltage_avg = 0;
            int32_t expected_vr_voltage_latest = 0;

            calculate_expected_vr_voltage_values(iteration,
                                                 index,
                                                 &expected_vr_voltage_min,
                                                 &expected_vr_voltage_max,
                                                 &expected_vr_voltage_avg,
                                                 &expected_vr_voltage_latest);

            bool print_logs = false;
            if (print_logs)
            {
                printf("Iterator - [%d], Index - [%d]\n", iteration, index);
                printf(" Current          Expected    Actual\n");
                printf("Min_mA:              %d         %d\n",
                       expected_vr_current_min,
                       vr_rail_record.rail_collection[index].rail_element.current.min_mA);
                printf("Max_mA:              %d         %d\n",
                       expected_vr_current_max,
                       vr_rail_record.rail_collection[index].rail_element.current.max_mA);
                printf("Average_mA:          %d         %d\n",
                       expected_vr_current_avg,
                       vr_rail_record.rail_collection[index].rail_element.current.average_mA);
                printf("Latest_mA:           %d         %d\n",
                       expected_vr_current_latest,
                       vr_rail_record.rail_collection[index].rail_element.current.latest_value_mA);

                printf("\n Voltage          Expected    Actual\n");
                printf("Min_mV:              %d         %d\n",
                       expected_vr_voltage_min,
                       vr_rail_record.rail_collection[index].rail_element.voltage.min_mV);
                printf("Max_mV:              %d         %d\n",
                       expected_vr_voltage_max,
                       vr_rail_record.rail_collection[index].rail_element.voltage.max_mV);
                printf("Average_mV:          %d         %d\n",
                       expected_vr_voltage_avg,
                       vr_rail_record.rail_collection[index].rail_element.voltage.average_mV);
                printf("Latest_mV:           %d         %d\n",
                       expected_vr_voltage_latest,
                       vr_rail_record.rail_collection[index].rail_element.voltage.latest_value_mV);
            }

            // Current assertions
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.min_mA, expected_vr_current_min);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.max_mA, expected_vr_current_max);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.average_mA, expected_vr_current_avg);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.latest_value_mA,
                             expected_vr_current_latest);

            // Voltage assertions
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.min_mV, expected_vr_voltage_min);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.max_mV, expected_vr_voltage_max);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.average_mV, expected_vr_voltage_avg);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.latest_value_mV,
                             expected_vr_voltage_latest);
        }
    }
}
