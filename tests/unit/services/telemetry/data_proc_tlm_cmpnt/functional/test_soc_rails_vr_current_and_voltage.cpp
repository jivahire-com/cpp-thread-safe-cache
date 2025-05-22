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
#include <compute_metrics_i.h>
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
    comp_metrics_reset_2_mins_metrics();
    comp_metrics_reset_24_hrs_metrics();
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

stats_t expected_vr_current_mA[MAX_NUM_OF_VR_RAILS];
stats_t expected_vr_voltage_mV[MAX_NUM_OF_VR_RAILS];

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

            update_stats(&expected_vr_current_mA[index], current_data_sets[iteration].vr_current_mA[index]);
            update_stats(&expected_vr_voltage_mV[index], current_data_sets[iteration].vr_voltage_mV[index]);
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

        printf("\nIteration %d Summary:\n", iteration);

        for (int32_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
        {
            printf("VR Rail %d:\n", index);
            // Current assertions
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.min_mA,
                             expected_vr_current_mA[index].min);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.max_mA,
                             expected_vr_current_mA[index].max);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.current.average_mA,
                             expected_vr_current_mA[index].avg);

            // Voltage assertions
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.min_mV,
                             expected_vr_voltage_mV[index].min);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.max_mV,
                             expected_vr_voltage_mV[index].max);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.voltage.average_mV,
                             expected_vr_voltage_mV[index].avg);
        }
    }
}
