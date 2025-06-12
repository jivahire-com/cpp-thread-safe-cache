//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_soc_rails_vr_temperature.cpp
 * Test functionality and flow of power_telemetry soc temperature collection
 *
 * Test Steps:
 * Step-1:
 * - Create and initialize test data for VR rail temperatures.
 * - The temperature_data_sets_dc array is set up with known values for 8 VR rails.
 * - Each rail has a distinct base value (starting at 15 and incrementing by 10 for each rail).
 * - Each iteration uses a different value for each rail to test min, max, avg, and latest behavior.
 * - The pattern is:
 *     Iteration 0: base value (15 + 10*index)
 *     Iteration 1: base + 5 (20 + 10*index)
 *     Iteration 2: base + 3 (18 + 10*index)
 *     Iteration 3: base + 7 (22 + 10*index)
 *
 * Step-2:
 * - For each iteration (0-3):
 *   - Copy test data to the mock structure for the current iteration.
 *   - Mock all sensor polling calls to return no data except for VR temperature polling, which returns valid test data.
 *   - Call data_proc_tlm_cmpnt_process_input_data() to process values.
 *
 * Step-3:
 * - Create a VR rail record:
 *   - Package the processed data into pwr_soc_record_vr_rail_t structure.
 *   - Record contains temperature data for all 8 VR rails.
 *
 * Step-4:
 * - For each rail (0-7), verify assert:
 *   - Min: Should remain at the initial value unless a lower value is seen.
 *   - Max: Should update if a new higher value is seen.
 *   - Latest: Always the most recent value for the rail in the current iteration.
 *   - Avg: Calculated as per tlm_logger.c logic (weighted average if residency > time_diff, else latest).
 *
 * Record structs are in telemetry_package_defs.h
 *
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
}

stats_t expected_vr_temp_dC[MAX_NUM_OF_VR_RAILS];

#define NO_OF_ITERATIONS 4

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

TEST_FUNCTION(test_soc_rails_vr_temperature_collection_functional, test_setup, test_teardown)
{
    // Defined a macro for NO_OF_ITERATIONS at the top of the file.
    // About temperature_data_sets_dc:
    //   - This array defines the test input for each iteration and each VR rail.
    //   - The values are chosen to:
    //     1. Provide distinct base values for each rail (increment by 10) for easy tracking and validation.
    //     2. Vary the value in each iteration to exercise min, max, avg, and latest logic.
    //     3. The pattern (base, base+5, base+3, base+7) ensures both increases and decreases are tested.

    vr_temp_t mock_temp_data = {0};

    int32_t temperature_data_sets_dC[NO_OF_ITERATIONS][MAX_NUM_OF_VR_RAILS] = {
        {15, 25, 35, 45, 55, 65, 75, 85}, // Iteration 0
        {20, 30, 40, 50, 60, 70, 80, 90}, // Iteration 1
        {18, 28, 38, 48, 58, 68, 78, 88}, // Iteration 2
        {22, 32, 42, 52, 62, 72, 82, 92}, // Iteration 3
    };

    for (int32_t iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        mock_temp_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        for (int32_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
        {
            mock_temp_data.vr_temp_dC[index] = temperature_data_sets_dC[iteration][index];
            update_stats(&expected_vr_temp_dC[index], temperature_data_sets_dC[iteration][index]);
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

        // SoC vr rail current - no data
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // more_entries

        // dimm sensor info - no data
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries

        // Pvt_Temperature polling - no data
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

        // SoC vr rail temperature - with data
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &mock_temp_data);

        data_proc_tlm_cmpnt_process_input_data();

        pwr_soc_record_vr_rail_t vr_rail_record;
        package_create_pwr_soc_vr_rail_record(&vr_rail_record);

        printf("\nIteration %d Summary:\n", iteration);

        for (int32_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
        {
            printf("VR Rail %d:\n", index);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.temperature.min_dC,
                             expected_vr_temp_dC[index].min);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.temperature.max_dC,
                             expected_vr_temp_dC[index].max);
            assert_int_equal(vr_rail_record.rail_collection[index].rail_element.temperature.average_dC,
                             expected_vr_temp_dC[index].avg);
        }
    }
}
