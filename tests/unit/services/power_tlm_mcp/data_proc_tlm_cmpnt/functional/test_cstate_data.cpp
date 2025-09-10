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
#include <sensor_fifo_service.h>
#include <telemetry_data_struct.h>
#include <telemetry_package_defs.h>

// Declare external variables
extern int g_enable_mock_pstate; // Declare the external variable for PSTATE mock control
#define NO_OF_ITERATIONS 8       // Increased from 5 to 8 to add more C-state transitions

// Define the test info structure
struct cstate_test_info
{
    uint8_t core_id;
    uint8_t cstate_value;
    uint8_t pstate_value; // P-state also added as part of pstate_telem_t packet
};

// Set up test data - C-state changes with corresponding P-states
// Added more iterations instead of just 4 to test entry_count > 1 scenarios
struct cstate_test_info cstate_info[NO_OF_ITERATIONS] = {
    {0, 0, 10}, // T0 Core 0, C-state 0, P-state 10
    {0, 1, 10}, // T1 Core 0, C-state 1, P-state 10
    {0, 1, 10}, // T2 Core 0, C-state 1 (same C-state, different timestamp) - for accumulation test
    {0, 0, 10}, // T3 Core 0, C-state 0, P-state 10
    {0, 3, 10}, // T4 Core 0, C-state 3, P-state 10
    {0, 0, 10}, // T5 Core 0, C-state 0 (third entry) - entry_count should be 3
    {0, 1, 10}, // T6 Core 0, C-state 1 (second entry) - entry_count should be 1
    {0, 1, 10}  // T7 Core 0, C-state 1 (same) - entry_count should be 1
};

// Define timestamps in microseconds
// Using standard 500ms intervals for predictable calculations for testing
uint64_t timestamps_microseconds[NO_OF_ITERATIONS] = {
    1000000, // T0: 1000ms
    1500000, // T1: 1500ms
    2000000, // T2: 2000ms
    2500000, // T3: 2500ms
    3000000, // T4: 3000ms
    3500000, // T5: 3500ms
    4000000, // T6: 4000ms
    4500000  // T7: 4500ms
};

uint32_t expected_residency_mS[NUMBER_OF_CSTATES] = {0};
}

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    data_proc_tlm_cmpnt_init(0);
    g_enable_mock_pstate = 1; // Enable P-state mocking

    // Reset computed metrics using already defined standard functions
    comp_metrics_reset_local_2_min_metrics();
    // reset 24 hrs metrics
    comp_metrics_reset_24_hrs_metrics();

    // Reset core state for clean test
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_cstate = 0;
        core_rt[core_id].cstate_res_timestamp_uS = 0;
        core_rt[core_id].latest_pstate = 0;
        core_rt[core_id].pstate_res_timestamp_uS = 0;
        core_rt[core_id].status_flags.pkt_cstate_is_valid = false;
        core_rt[core_id].status_flags.pkt_pstate_is_valid = false;
    }
    setup_snsr_fifo_is_empty();

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
    g_enable_mock_pstate = 0; // Disable P-state mocking
    return 0;
}

// Helper function to set up mock sensor polling with no data
static void setup_mock_sensor_polling_no_data(void)
{
    will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);
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
static void calculate_expected_cstate_residency(int current_iteration)
{

    uint32_t c0_residency_at_T1_mS = (timestamps_microseconds[1] - timestamps_microseconds[0]) / 1000;
    uint32_t c1_residency_at_T2_mS = (timestamps_microseconds[2] - timestamps_microseconds[1]) / 1000;
    uint32_t c1_residency_at_T3_mS =
        c1_residency_at_T2_mS + ((timestamps_microseconds[3] - timestamps_microseconds[2]) / 1000);
    uint32_t c0_residency_at_T4_mS =
        c0_residency_at_T1_mS + ((timestamps_microseconds[4] - timestamps_microseconds[3]) / 1000);
    uint32_t c3_residency_at_T5_mS = (timestamps_microseconds[5] - timestamps_microseconds[4]) / 1000;
    uint32_t c0_residency_at_T6_mS =
        c0_residency_at_T4_mS + ((timestamps_microseconds[6] - timestamps_microseconds[5]) / 1000);
    uint32_t c1_residency_at_T7_mS =
        c1_residency_at_T3_mS + ((timestamps_microseconds[7] - timestamps_microseconds[6]) / 1000);

    switch (current_iteration)
    {
    case 0:
        break;
    case 1:
        expected_residency_mS[CSTATE_C0] = c0_residency_at_T1_mS;
        break;
    case 2:
        expected_residency_mS[CSTATE_C1] = c1_residency_at_T2_mS;
        break;
    case 3:
        expected_residency_mS[CSTATE_C1] = c1_residency_at_T3_mS;
        break;
    case 4:
        expected_residency_mS[CSTATE_C0] = c0_residency_at_T4_mS;
        break;
    case 5:
        expected_residency_mS[CSTATE_C3] = c3_residency_at_T5_mS;
        break;
    case 6:
        expected_residency_mS[CSTATE_C0] = c0_residency_at_T6_mS;
        break;
    case 7:
        expected_residency_mS[CSTATE_C1] = c1_residency_at_T7_mS;
        break;
    default:
        break;
    }
}

TEST_FUNCTION(test_tlm_logger_log_cstate_information, test_setup, test_teardown)
{
    bool print_logs = true;
    uint8_t current_cstate = 0;
    if (print_logs)
    {
        printf("\nTEST START: test_tlm_logger_log_cstate_information\n");
    }

    // Convert to ticks
    uint64_t timestamps_ticks[NO_OF_ITERATIONS];
    for (int i = 0; i < NO_OF_ITERATIONS; i++)
    {
        timestamps_ticks[i] = (timestamps_microseconds[i] * 2400000) / 1000000;
    }

    uint8_t core_id = 0;
    pstate_telem_t mock_pstate_data = {0};

    // Initialize core state for proper C-state transitions
    core_rt[core_id].latest_cstate = 0; // Start with C-state 0
    core_rt[core_id].cstate_res_timestamp_uS = 0;

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
        current_cstate = cstate_info[iteration].cstate_value;

        // Calculate expected residency using helper function
        calculate_expected_cstate_residency(iteration);

        // Calculate expected entry count based on iteration
        uint32_t expected_entry_count = 0;
        switch (iteration)
        {
        case 0: // C-state 0: first entry
            expected_entry_count = 1;
            break;
        case 1: // C-state 1: first entry (new C-state)
            expected_entry_count = 1;
            break;
        case 2: // C-state 1: same C-state (no change)
            expected_entry_count = 1;
            break;
        case 3: // C-state 0:
            expected_entry_count = 2;
            break;
        case 4: // C-state 3: first entry (new C-state)
            expected_entry_count = 1;
            break;
        case 5: // C-state 0: third entry
            expected_entry_count = 3;
            break;
        case 6: // C-state 1: second entry
            expected_entry_count = 2;
            break;
        case 7: // C-state 1: continue in Cstate 1
            expected_entry_count = 2;
            break;
        }

        if (print_logs)
        {
            printf("Current C-state: %d (expected=%d) "
                   "Current C-State entry_count=%d "
                   "(expected=%d)\n",
                   current_cstate,
                   cstate_info[iteration].cstate_value,
                   cstate_array[current_cstate].entry_count,
                   expected_entry_count);
            for (int cstate_val = 0; cstate_val < NUMBER_OF_CSTATES; cstate_val++)
            {
                printf("cstate = %d, expected = %d, actual = %d \n",
                       cstate_val,
                       expected_residency_mS[cstate_val],
                       cstate_array[cstate_val].residency_mS);
            }
        }

        // Verify C-state, residency and entry count for current C-state
        assert_int_equal(current_cstate, cstate_info[iteration].cstate_value);
        assert_int_equal(expected_entry_count, cstate_array[current_cstate].entry_count);

        for (int cstate_val = 0; cstate_val < NUMBER_OF_CSTATES; cstate_val++)
        {
            assert_int_equal(expected_residency_mS[cstate_val], cstate_array[cstate_val].residency_mS);
        }
    }
}
