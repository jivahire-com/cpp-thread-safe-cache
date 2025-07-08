//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cstate_data.cpp
 * Test functionality and flow for power telemetry C-state data collection
 *
 * Test Steps:
 * -----------
 * Step-1:
 * Create and initialize C-state data using pstate_telem_t structure
 * Set up varied C-state transitions: C0→C1→C1→C2→C3→C1→C0→C1 (8 iterations)
 * - Tests same C-state accumulation (C1 repeated at iteration 2)
 * - Tests multiple entries to same C-state (C1 entered 3 times, C0 entered 2 times)
 * - Tests entry_count > 1 scenarios (C1 entry_count = 2 at iteration 5, = 3 at iteration 7)
 * Send timestamps as packets with standard 500ms intervals and calculate expected residency
 *
 * Step-2:
 * Process the data
 * Call the data aggregation function to process the mocked C-state data.
 * Update internal data structures with the C-state values.
 * Calculate residency and entry counts based on state changes and timestamp differences.
 * Verify that entry_count increments only on actual C-state changes, not repeated entries.
 *
 * Step-3:
 * Create C-state record
 * Package the processed C-state data into a record structure.
 * The record contains the C-state ID, residency time, and entry count for each core.
 *
 * Step-4:
 * Verify the results
 * Check that the C-state values, residency times, and entry counts match the expected values.
 * Verify C-state residency accumulation when same C-state receives multiple packets.
 * Assert that the C-state changes are properly tracked.
 * Validate entry count logic:
 * - C-state 0: entry_count = 1 (entered twice, but first entry doesn't count)
 * - C-state 1: entry_count = 3 (entered three times: iterations 1, 5, 7)
 * - C-state 2: entry_count = 1 (entered once at iteration 3)
 * - C-state 3: entry_count = 1 (entered once at iteration 4)
 * Verify that repeated C-state entries (iteration 2) do not increment entry_count.
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
#include <data_sampling_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h>
#include <telemetry_data_struct.h>
#include <telemetry_package_defs.h>

// Declare external variables
extern int g_enable_mock_pstate; // Declare the external variable for PSTATE mock control
}

#define NO_OF_ITERATIONS 8 // Increased from 5 to 8 to add more C-state transitions

// Define the test info structure
struct cstate_test_info
{
    uint8_t core_id;
    uint8_t cstate_value;
    uint8_t pstate_value; // P-state also added as part of pstate_telem_t packet
};

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    data_proc_tlm_cmpnt_init(0);
    g_enable_mock_pstate = 1; // Enable P-state mocking

    // Reset computed metrics using already defined standard functions
    comp_metrics_reset_2_mins_metrics();
    comp_metrics_reset_24_hrs_metrics();

    // Reset core state for clean test
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core[core_id].latest_cstate = 0;
        core[core_id].cstate_timestamp_uS = 0;
        core[core_id].flags.cstate_change = 0;
        core[core_id].latest_pstate = 0;
        core[core_id].pstate_timestamp_uS = 0;
    }

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    g_enable_mock_pstate = 0; // Disable P-state mocking
    return 0;
}

// Helper function to set up mock sensor polling with no data
static void setup_mock_sensor_polling_no_data(void)
{
    // Set up mocks
    // 1. Tile temperature polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);     // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries

    // 2. Tile voltage polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, 0);     // tile_index
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries

    // 3. Current polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);     // core_index
    will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries

    // 4. VR temperature polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, false); // more_entries

    // 5. VR current polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, false); // more_entries

    // 6. SoC PVT temperature polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false); // more_entries

    // 7. DIMM info polling - no data
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, false); // more_entries
}

// Helper function to calculate expected residency based on iteration
static uint32_t calculate_expected_cstate_residency(const uint64_t* timestamps_microseconds, int current_iteration)
{
    uint32_t expected_residency = 0;

    switch (current_iteration)
    {
    case 0: // C-state 0: first entry
        expected_residency = 0;
        break;
    case 1: // C-state 1: first entry (new C-state)
        expected_residency = (timestamps_microseconds[1] - timestamps_microseconds[0]) / 1000; // 500ms
        break;
    case 2: // C-state 1: same C-state (cumulative residency from first occurrence)
        expected_residency = (timestamps_microseconds[1] - timestamps_microseconds[0]) +
                             (timestamps_microseconds[2] - timestamps_microseconds[1]); // 500 + 500 = 1000ms
        expected_residency = expected_residency / 1000; // Convert to milliseconds
        break;
    case 3: // C-state 2: first entry (new C-state)
        expected_residency = (timestamps_microseconds[3] - timestamps_microseconds[2]) / 1000; // 500ms
        break;
    case 4: // C-state 3: first entry (new C-state)
        expected_residency = (timestamps_microseconds[4] - timestamps_microseconds[3]) / 1000; // 500ms
        break;
    case 5: // C-state 1: second entry (return to C-state 1) - entry_count should be 2
        expected_residency = (timestamps_microseconds[1] - timestamps_microseconds[0]) +
                             (timestamps_microseconds[2] - timestamps_microseconds[1]) +
                             (timestamps_microseconds[5] - timestamps_microseconds[4]); // 500 + 500 + 500 = 1500ms
        expected_residency = expected_residency / 1000; // Convert to milliseconds
        break;
    case 6: // C-state 0: second entry (return to C-state 0) - entry_count should be 1
        expected_residency = (timestamps_microseconds[6] - timestamps_microseconds[5]) / 1000; // 500ms
        break;
    case 7: // C-state 1: third entry (return to C-state 1 again) - entry_count should be 3
        expected_residency = (timestamps_microseconds[1] - timestamps_microseconds[0]) +
                             (timestamps_microseconds[2] - timestamps_microseconds[1]) +
                             (timestamps_microseconds[5] - timestamps_microseconds[4]) +
                             (timestamps_microseconds[7] - timestamps_microseconds[6]); // 500 + 500 + 500 + 500 = 2000ms
        expected_residency = expected_residency / 1000; // Convert to milliseconds
        break;
    default:
        expected_residency = 0;
        break;
    }

    return expected_residency;
}

TEST_FUNCTION(test_tlm_logger_log_cstate_information, test_setup, test_teardown)
{
    bool print_logs = true;
    if (print_logs)
    {
        printf("\nTEST START: test_tlm_logger_log_cstate_information\n");
    }

    // Set up test data - C-state changes with corresponding P-states
    // Added more iterations instead of just 4 to test entry_count > 1 scenarios
    struct cstate_test_info cstate_info[NO_OF_ITERATIONS] = {
        {0, 0, 10}, // Core 0, C-state 0, P-state 10
        {0, 1, 10}, // Core 0, C-state 1, P-state 10
        {0, 1, 10}, // Core 0, C-state 1 (same C-state, different timestamp) - for accumulation test
        {0, 2, 10}, // Core 0, C-state 2, P-state 10
        {0, 3, 10}, // Core 0, C-state 3, P-state 10
        {0, 1, 10}, // Core 0, C-state 1 (second entry) - entry_count should be 2
        {0, 0, 10}, // Core 0, C-state 0 (second entry) - entry_count should be 1
        {0, 1, 10}  // Core 0, C-state 1 (third entry) - entry_count should be 3
    };

    // Define timestamps in microseconds
    // Using standard 500ms intervals for predictable calculations for testing
    uint64_t timestamps_microseconds[NO_OF_ITERATIONS] = {
        1000000, // T0: 1000ms - C-state 0 enters
        1500000, // T1: 1500ms - C-state 1 enters (500ms later)
        2000000, // T2: 2000ms - Same C-state 1 (500ms later)
        2500000, // T3: 2500ms - C-state 2 enters (500ms later)
        3000000, // T4: 3000ms - C-state 3 enters (500ms later)
        3500000, // T5: 3500ms - C-state 1 enters again (500ms later)
        4000000, // T6: 4000ms - C-state 0 enters again (500ms later)
        4500000  // T7: 4500ms - C-state 1 enters again (500ms later)
    };

    // Convert to ticks
    uint64_t timestamps_ticks[NO_OF_ITERATIONS];
    for (int i = 0; i < NO_OF_ITERATIONS; i++)
    {
        timestamps_ticks[i] = (timestamps_microseconds[i] * 2400000) / 1000000;
    }

    uint8_t core_id = 0;
    pstate_telem_t mock_pstate_data = {0};

    // Initialize core state for proper C-state transitions
    core[core_id].latest_cstate = 0; // Start with C-state 0
    core[core_id].cstate_timestamp_uS = 0;
    core[core_id].flags.cstate_change = 0;

    for (int iteration = 0; iteration < NO_OF_ITERATIONS; iteration++)
    {
        if (print_logs)
        {
            printf("Step %d: C-state %d enters at T%d (%llums)\n",
                   iteration + 1,
                   cstate_info[iteration].cstate_value,
                   iteration,
                   timestamps_microseconds[iteration] / 1000);
        }

        // Set up mock data for current iteration
        mock_pstate_data.timestamp = timestamps_ticks[iteration];
        mock_pstate_data.data.cstate = cstate_info[iteration].cstate_value;
        mock_pstate_data.data.pstate = cstate_info[iteration].pstate_value;
        mock_pstate_data.data.core = core_id;
        mock_pstate_data.data.throttle_status = NO_THROTTLING;
        mock_pstate_data.data.plimit = 20;

        // Set up mock PSTATE polling calls
        will_return(__wrap_gtimer_prodfw_get_frequency, 2400000);
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);

        setup_mock_sensor_polling_no_data();

        // Process the data
        data_proc_tlm_cmpnt_process_input_data();

        // Create C-state record after processing
        pwr_core_record_cstate_t cstate_record = {{0}};
        package_create_pwr_core_cstate_record(&cstate_record);
        pwr_core_element_cstate_t* cstate_array = &cstate_record.cstate_collection[core_id].cstate_element[0];

        // Get current C-state for this iteration
        uint8_t current_cstate = cstate_info[iteration].cstate_value;

        // Calculate expected residency using helper function
        uint32_t expected_residency = calculate_expected_cstate_residency(timestamps_microseconds, iteration);

        // Calculate expected entry count based on iteration
        uint32_t expected_entry_count = 0;
        switch (iteration)
        {
        case 0: // C-state 0: first entry
            expected_entry_count = 0;
            break;
        case 1: // C-state 1: first entry (new C-state)
            expected_entry_count = 1;
            break;
        case 2: // C-state 1: same C-state (no change)
            expected_entry_count = 1;
            break;
        case 3: // C-state 2: first entry (new C-state)
            expected_entry_count = 1;
            break;
        case 4: // C-state 3: first entry (new C-state)
            expected_entry_count = 1;
            break;
        case 5: // C-state 1: second entry (return to C-state 1)
            expected_entry_count = 2;
            break;
        case 6: // C-state 0: second entry (return to C-state 0)
            expected_entry_count = 1;
            break;
        case 7: // C-state 1: third entry (return to C-state 1 again)
            expected_entry_count = 3;
            break;
        }

        if (print_logs)
        {
            printf("Current C-state: %d (expected=%d), residency=%d ms (expected=%d ms), entry_count=%d "
                   "(expected=%d)\n",
                   current_cstate,
                   cstate_info[iteration].cstate_value,
                   cstate_array[current_cstate].residency_mS,
                   expected_residency,
                   cstate_array[current_cstate].entry_count,
                   expected_entry_count);
        }

        // Verify C-state, residency and entry count for current C-state
        assert_int_equal(current_cstate, cstate_info[iteration].cstate_value);
        assert_int_equal(expected_residency, cstate_array[current_cstate].residency_mS);
        assert_int_equal(expected_entry_count, cstate_array[current_cstate].entry_count);
    }
}
