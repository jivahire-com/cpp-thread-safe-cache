//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_throttling_data.cpp
 * Test functionality and flow for power telemetry throttling data collection
 *
 * Test Flow:
 * ----------
 * Set up mock current data (power values)
 * Set up mock PSTATE data with throttling status
 * Mock sensor polling calls
 *  Call: data_smpl_process_pstate_sensor_fifo()
 *  Call: data_smpl_process_core_current_sensor_fifo()
 *  Only call data_smpl_finalize_pwr_pkg_metrics() for throttling sources still active at the package boundary (START sent, no END).
 *  Call: package_create_pwr_core_throttle_record()
 * Verify/Assert results
 *
 * Test Functions:
 * ---------------
 * 1. test_core_current_throttling_functional: Tests current throttling start/end flow
 * 2. test_core_multiple_current_throttling_cycles: Tests multiple current throttling cycles with duplicates
 * 3. test_core_multiple_temperature_throttling_cycles: Tests temperature throttling cycles
 * 4. test_core_simultaneous_current_and_temperature_throttling: Tests concurrent throttling types
 * 5. test_core_simultaneous_current_and_temperature_throttling_with_no_throttling: Tests NO_THROTTLING end
 * 6. test_core_overlapping_current_and_temperature_throttling_with_current_restart: Tests restart scenarios
 * 7. test_core_multiple_adaptive_clocking_throttling_cycles: Tests adaptive clocking with duplicates
 * 8. test_core_multiple_rack_throttling_cycles: Tests rack throttling with priorities
 * 9. test_core_simultaneous_rack_and_current_throttling: Tests rack and current concurrent throttling
 * 10. test_core_multiple_current_overrun_cycles: Tests current overrun throttling
 * 11. test_core_multiple_adaptive_clocking_overrun_cycles: Tests adaptive clocking overrun throttling
 *
 * Throttling Types Available:
 * ===========================
 *
 * All Throttle Sources (throttle_source_t enum):
 * - THROTTLE_SOURCE_CURRENT = 0              Covered
 * - THROTTLE_SOURCE_TEMPERATURE = 1          Covered
 * - THROTTLE_SOURCE_RACK_LIMIT = 2           Covered
 * - THROTTLE_SOURCE_ADAPTIVE_CLK = 4         Covered
 * - THROTTLE_SOURCE_CURRENT_OVERRUN = 5      Covered
 * - THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN = 6 Covered
 * - THROTTLE_SOURCE_VR_HOT = 3               Deferred
 *
 * Notes:
 * - Throttling status comes from PSTATE telemetry packets
 * - Each throttling type has start/end events and residency tracking
 * - Entry count increments on throttling start events
 * - Residency accumulates during throttling periods and is finalized when cycles complete
 *   Do NOT call data_smpl_finalize_pwr_pkg_metrics() to finalize residency calculation after a throttle END event, as residency is finalized by the event itself.
 * - It is only called at the package boundary to finalize any active throttling sources that did not receive an END event. Timestamp for this is integrated by calling mock_timestamp function just before the call.
 * - Multiple throttling types can be active simultaneously
 * - Duplicate START events are ignored for entry_count but contribute to residency
 * - Tests validate functional scenarios, multiple cycles, concurrent operation
 * - For overrun scenarios, only overrun_count is being validated and asserted.
 * - VR_HOT is deferred.
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2768959

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
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
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
extern uint32_t time_t0;
extern int g_enable_mock_pstate;
}

#define NUMBER_OF_CORES_TO_TEST (4)

// Global variable to control debug printing
static bool g_print_logs = true;

// Structure to hold test throttling configurations
typedef struct
{
    uint8_t pstate;          // PSTATE value
    uint8_t plimit;          // Power limit value
    uint8_t throttle_status; // Throttling status from pstate_throttle_status_t
    uint8_t vm_throttle_pri; // Rack priority (for rack throttling)
    uint64_t timestamp;      // Timestamp for the throttling event
    uint16_t power_mW;       // Power value in mW
} test_throttling_config_t;

// Structure to hold test current data configurations
typedef struct
{
    uint32_t avg;   // Average current (non-negative)
    uint32_t min;   // Minimum current (non-negative)
    uint32_t max;   // Maximum current (non-negative)
    uint32_t volt;  // Voltage value in mV (non-negative)
    uint32_t pwr;   // Power value (non-negative)
    uint8_t pstate; // PSTATE from current packet (small, non-negative)
} test_current_config_t;

// Structure to hold expected throttling values
typedef struct
{
    uint8_t throttle_type;  // Throttle type (throttle_source_t)
    uint32_t entry_count;   // Number of times throttling started
    uint32_t residency_mS;  // Total residency time
    uint16_t overrun_count; // Number of overrun events
    uint8_t max_pstate;     // Maximum PSTATE during throttling
    uint8_t avg_pstate;     // Average PSTATE during throttling
} expected_throttling_values_t;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    // Reset all telemetry data structures
    reset_pwr_tlm_data();

    // Initialize core active status - needed for throttling metrics
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        core_is_active[core_id] = true; // Mark cores as active for compute metrics
    }

    // Initialize core data structure with default values
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        // Initialize core state
        core_rt[core_id].pstate_from_pstate_pkt = 0x00;
        core_rt[core_id].latest_plimit = 0;
        core_rt[core_id].pstate_res_timestamp_uS = 0;
        core_rt[core_id].status_flags.throttle_is_active = false;
        core_rt[core_id].latest_rack_throttle_priority = 0;

        // Initialize throttling data for each core in computed_metrics_2_mins
        for (uint8_t throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
        {
            core_rt[core_id].throttle_source_tracker[throttle_source] = false;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.min = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.max = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg.summation = 0;
        }
    }

    // Reset mock timestamp to initial value
    time_t0 = 100;
    // Enable PSTATE mock for all tests
    g_enable_mock_pstate = 1;

    in_band_publishing_active = true;

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    // Disable PSTATE mock after test completion
    g_enable_mock_pstate = 0;
    // Clean up any resources if needed
    reset_pwr_tlm_data();
    return 0;
}

// Helper function to set up mock gtimer frequency calls
static void setup_mock_gtimer_frequency(void)
{
    // Set frequency to 1 MHz so 1 tick = 1 μs - using will_return_always for all calls so that we dont have to set it up for each test_function
    will_return_always(__wrap_gtimer_prodfw_get_frequency, 1000000);
}

// Helper function to set up mock PSTATE data with throttling
static void setup_mock_pstate_data_with_throttling(pstate_telem_t* mock_pstate_data,
                                                   uint8_t core_index,
                                                   uint8_t pstate,
                                                   uint8_t plimit,
                                                   uint8_t throttle_status,
                                                   uint8_t vm_throttle_pri,
                                                   uint64_t timestamp)
{
    mock_pstate_data->timestamp = timestamp;
    mock_pstate_data->data.pstate = pstate;
    mock_pstate_data->data.plimit = plimit;
    mock_pstate_data->data.core = core_index;
    mock_pstate_data->data.cstate = 0; // CSTATE_C0 - active state
    mock_pstate_data->data.throttle_status = throttle_status;
    mock_pstate_data->data.vm_throttle_pri = vm_throttle_pri;

    // Set up mock PSTATE polling calls - follow functional mock pattern
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);  // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false); // more_entries
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, mock_pstate_data);
}

// Helper function to set up mock current data
static void setup_mock_current_data(core_current_t* mock_current_data,
                                    uint8_t core_index,
                                    uint64_t timestamp,
                                    int32_t avg,
                                    int32_t min,
                                    int32_t max,
                                    int32_t volt,
                                    int32_t pwr,
                                    int32_t pstate)
{
    mock_current_data->timestamp = timestamp;
    mock_current_data->data.avg = avg;
    mock_current_data->data.min = min;
    mock_current_data->data.max = max;
    mock_current_data->data.volt = volt;
    mock_current_data->data.pwr = pwr;
    mock_current_data->data.pstate = pstate;
    mock_current_data->data.reserved_45 = 0;
    mock_current_data->data.realm = 0;
    mock_current_data->data.change = 0;
    mock_current_data->data.mpam_id_low = 0;
    mock_current_data->data.mpam_id_high = 0;
    mock_current_data->data.cstate = 0;

    // Set up mock current polling calls - follow functional mock pattern
    will_return(__wrap_sensor_fifo_svc_poll_core_current, core_index);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, true);  // curr_data_is_valid
    will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries
    will_return(__wrap_sensor_fifo_svc_poll_core_current, mock_current_data);
}

// Helper to set up the next finalize timestamp for residency calculation.
// Usage: Call set_next_finalize_timestamp(desired_timestamp) before running the code that finalizes residency.
// This sets time_t0 so that when the mock timestamp function is called (inside data_smpl_finalize_pwr_pkg_metrics),
// it returns the exact timestamp needed for residency calculation.
// Subtracting MOCK_TIMESTAMP_INCREMENT (1000us) because the mock function increments time_t0 by 1000us before returning,
// and this adjustment ensures the returned value matches the desired timestamp exactly.
// Example: set_next_finalize_timestamp(7500000); // Ensures residency is calculated based on 7500000us
// After finalization, 'time_t0' is used as the actual finalize timestamp for assertions.
void set_next_finalize_timestamp(uint64_t desired_timestamp)
{
    time_t0 = desired_timestamp - MOCK_TIMESTAMP_INCREMENT;
}

// Test single current throttling cycle: NO_THROTTLING → CURRENT_THROTTLING_START → CURRENT_THROTTLING_END
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START (throttling begins)
// T3: CURRENT_THROTTLING_END (throttling ends, cycle complete)
// --- FINAL CHECK: entry_count=1, active=false (Functional Test Complete) ---
TEST_FUNCTION(test_core_current_throttling_functional, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_current_throttling_functional ---\n");
    }
    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_finalize_us = 4000000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Use microsecond values directly for timestamps
    pstate_telem_t mock_pstate_data_initial = {0};
    core_current_t mock_current_data = {0};

    // Step 1: NO_THROTTLING
    setup_mock_pstate_data_with_throttling(&mock_pstate_data_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&mock_current_data, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: CURRENT_THROTTLING_START
    pstate_telem_t mock_pstate_data_start = {0};
    core_current_t mock_current_data_start = {0};

    setup_mock_pstate_data_with_throttling(&mock_pstate_data_start, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&mock_current_data_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: CURRENT_THROTTLING_END
    pstate_telem_t mock_pstate_data_end = {0};
    core_current_t mock_current_data_end = {0};

    setup_mock_pstate_data_with_throttling(&mock_pstate_data_end, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&mock_current_data_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    // setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Processing step 3: CURRENT_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Set a finalize timestamp after the END event and call finalize.
    // This simulates package boundary logic, but since the throttle was already closed by the END event,
    // the residency calculation is complete and uses the END event timestamp as the final value.
    // Calling finalize with a later timestamp should NOT affect residency for closed cycles.
    set_next_finalize_timestamp(T4_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used (for debug only)
    if (g_print_logs)
        printf("Finalize called at T=%" PRIu32 " us (should not affect closed cycle residency calculation)\n",
               T_finalize_pkg_us);

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array = &throttle_record.throttle_collection[core_index].throttle_element[0];

    // Expected values for current throttling (THROTTLE_SOURCE_CURRENT = 0)
    uint32_t expected_entry_count = 1; // One throttling start event
    // expected_residency_mS is in milliseconds (mS), calculated by converting microsecond timestamps to milliseconds.
    uint32_t expected_residency_mS =
        (T3_us - T2_us) / 1000; // Throttling started at T2_us, ended at T3_us, so residency = T3_us-T2_us = 1000 mS
    bool expected_throttling_status = false; // Throttling should be inactive after END event

    if (g_print_logs)
    {
        printf("\n=== Results ===\n");
        printf("Current Throttling entry_count: %d (expected=%d)\n", throttle_array[0].entry_count, expected_entry_count);
        printf("Current Throttling residency: %d ms (expected=%d ms)\n", throttle_array[0].residency_mS, expected_residency_mS);
        printf("Current throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
    }

    // Verify results
    assert_int_equal(expected_entry_count, throttle_array[0].entry_count);
    assert_int_equal(expected_residency_mS, throttle_array[0].residency_mS);
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(false, core_rt[core_index].throttle_source_tracker[0]); // Current throttling tracker should be 0

    if (g_print_logs)
    {
        printf("--- END test_core_current_throttling_functional ---\n");
        printf("***\n");
    }
}

// Test multiple current throttling cycles: 3 start events, 2 completed cycles with start -> end, ends with 3rd cycle active (START status)
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START #1 (cycle 1 begins)
// T2.5: CURRENT_THROTTLING_START #1 DUPLICATE (ignored)
// T3: CURRENT_THROTTLING_END #1 (cycle 1 complete)
// T4: CURRENT_THROTTLING_START #2 (cycle 2 begins)
// T5: CURRENT_THROTTLING_END #2 (cycle 2 complete)
// T6: CURRENT_THROTTLING_START #3 (cycle 3 begins, remains active)
// T7_finalize_pkg_us: Finalize timestamp for residency calculation, set using __wrap_exec_tlm_cmpnt_get_timestamp_microseconds()
// --- FINAL CHECK: entry_count=3, active=true (Multiple Cycles Test with Duplicate Handling) ---
TEST_FUNCTION(test_core_multiple_current_throttling_cycles, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_current_throttling_cycles ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_us = 5000000,
             T6_us = 6000000, T7_finalize_pkg_us = 7500000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Throttling START #1
    pstate_telem_t pstate_start1 = {0};
    core_current_t current_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start1, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    // setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START #1
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 2.5: Duplicate Throttling START #1 (should be ignored - tests duplicate start event handling)
    pstate_telem_t pstate_start1_duplicate = {0};
    core_current_t current_start1_duplicate = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start1_duplicate, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us + 500000);
    setup_mock_current_data(&current_start1_duplicate, core_index, T2_us + 500000, 105, 85, 125, 105, 155, 10);
    if (g_print_logs)
        printf("Processing step 2.5: CURRENT_THROTTLING_START #1 DUPLICATE (should be ignored)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 3: Throttling END #1
    pstate_telem_t pstate_end1 = {0};
    core_current_t current_end1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end1, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_end1, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 3: CURRENT_THROTTLING_END #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Throttling START #2
    pstate_telem_t pstate_start2 = {0};
    core_current_t current_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start2, core_index, 12, 22, CURRENT_THROTTLING_START, 0, T4_us);
    setup_mock_current_data(&current_start2, core_index, T4_us, 110, 85, 125, 105, 210, 12);
    if (g_print_logs)
        printf("Processing step 4: CURRENT_THROTTLING_START #2\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Throttling END #2
    pstate_telem_t pstate_end2 = {0};
    core_current_t current_end2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end2, core_index, 13, 23, CURRENT_THROTTLING_END, 0, T5_us);
    setup_mock_current_data(&current_end2, core_index, T5_us, 115, 90, 130, 110, 220, 13);
    if (g_print_logs)
        printf("Processing step 5: CURRENT_THROTTLING_END #2\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 6: Throttling START #3 (active, not ended)
    pstate_telem_t pstate_start3 = {0};
    core_current_t current_start3 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start3, core_index, 14, 24, CURRENT_THROTTLING_START, 0, T6_us);
    setup_mock_current_data(&current_start3, core_index, T6_us, 120, 95, 135, 115, 230, 14);
    if (g_print_logs)
        printf("Processing step 6: CURRENT_THROTTLING_START #3 (remains active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // --- Residency accumulation test for finalize ---
    // Set next finalize timestamp for residency calculation
    set_next_finalize_timestamp(T7_finalize_pkg_us);
    // Calling data_smpl_finalize_pwr_pkg_metrics() before package creation to finalize residency for any throttling source still active (START sent, no END).
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used
    if (g_print_logs)
        printf("Finalize timestamp used for residency calculation: %u us\n", T_finalize_pkg_us);

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array = &throttle_record.throttle_collection[core_index].throttle_element[0];

    // Expected values for current throttling (THROTTLE_SOURCE_CURRENT = 0)
    uint32_t expected_entry_count = 3; // Three throttling start events (T2_us, T4_us, T6_us)
    // Residency calculation:
    // Cycle 1: T2_us to T3_us = (3000000 - 2000000) = 1000000 us
    // Cycle 2: T4_us to T5_us = (5000000 - 4000000) = 1000000 us
    // Cycle 3: T6_us to T_finalize_pkg_us = (7500000 - 6000000) = 1500000 us
    // Total residency = 1000000 + 1000000 + 1500000 = 3500000 us = 3500 mS
    // Converting to milliseconds: 3500000 us / 1000 = 3500 mS
    uint32_t expected_residency_mS = ((T3_us - T2_us) + (T5_us - T4_us) + (T_finalize_pkg_us - T6_us)) / 1000;
    bool expected_throttling_status = true; // Throttling should be active because the 3rd cycle is still running
    bool expected_tracker_status = true;    // Tracker should be active for 3rd cycle

    if (g_print_logs)
    {
        printf("\n=== Results (Multiple Cycles with Duplicate Start Test) ===\n");
        printf("Current Throttling entry_count: %d (expected=%d)\n", throttle_array[0].entry_count, expected_entry_count);
        printf("Current Throttling residency: %d ms (expected=%d ms)\n", throttle_array[0].residency_mS, expected_residency_mS);
        printf("Current throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[0] > 0 ? 1 : 0,
               expected_tracker_status);
        printf("NOTE: Duplicate START event at T2+500ms should be ignored (entry_count remains 3, not 4)\n");
    }

    // Assert that throttling status is active after the last START event (cycle 3 is still running)
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Verify results
    assert_int_equal(expected_entry_count, throttle_array[0].entry_count);
    assert_int_equal(expected_residency_mS, throttle_array[0].residency_mS);
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(expected_tracker_status, core_rt[core_index].throttle_source_tracker[0] > 0); // Current throttling tracker should be active

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_current_throttling_cycles ---\n");
        printf("***\n");
    }
}

// Test multiple current throttling cycles: 3 start events, 2 completed cycles with start -> end, ends with 3rd cycle active (START status)
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: TEMP_THROTTLING_START #1 (cycle 1 begins)
// T3: TEMP_THROTTLING_START continues (residency accumulation)
// T4: TEMP_THROTTLING_END #1 (cycle 1 complete)
// T5: TEMP_THROTTLING_START #2 (cycle 2 begins, remains active)
// T6_finalize_pkg_us: Finalize timestamp for residency calculation, set using __wrap_exec_tlm_cmpnt_get_timestamp_microseconds()
// --- FINAL CHECK: entry_count=2, active=true (Multiple Cycles Test)
TEST_FUNCTION(test_core_multiple_temperature_throttling_cycles, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_temperature_throttling_cycles ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_us = 5000000,
             T6_finalize_pkg_us = 7500000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Temperature Throttling START #1
    pstate_telem_t pstate_temp_start1 = {0};
    core_current_t current_temp_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_start1, core_index, 10, 20, TEMP_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_temp_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: TEMP_THROTTLING_START #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after TEMP_THROTTLING_START #1
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Temperature Throttling continues (same START status) - to test residency accumulation
    pstate_telem_t pstate_temp_continue1 = {0};
    core_current_t current_temp_continue1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_continue1, core_index, 11, 21, TEMP_THROTTLING_START, 0, T3_us);
    setup_mock_current_data(&current_temp_continue1, core_index, T3_us, 105, 85, 125, 105, 160, 11);
    if (g_print_logs)
        printf("Processing step 3: TEMP_THROTTLING_START continues (residency accumulation test)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Temperature Throttling END #1
    pstate_telem_t pstate_temp_end1 = {0};
    core_current_t current_temp_end1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_end1, core_index, 11, 25, TEMP_THROTTLING_END, 0, T4_us);
    setup_mock_current_data(&current_temp_end1, core_index, T4_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 4: TEMP_THROTTLING_END #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Temperature Throttling START #2 (active, not ended)
    pstate_telem_t pstate_temp_start2 = {0};
    core_current_t current_temp_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_start2, core_index, 12, 22, TEMP_THROTTLING_START, 0, T5_us);
    setup_mock_current_data(&current_temp_start2, core_index, T5_us, 110, 85, 125, 105, 210, 12);
    if (g_print_logs)
        printf("Processing step 5: TEMP_THROTTLING_START #2 (remains active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // --- Residency accumulation test for finalize ---
    // Set next finalize timestamp for residency calculation
    set_next_finalize_timestamp(T6_finalize_pkg_us);
    // Calling data_smpl_finalize_pwr_pkg_metrics() before package creation to finalize residency for any throttling source still active (START sent, no END).
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_TEMPERATURE];

    // Expected values for temperature throttling (THROTTLE_SOURCE_TEMPERATURE = 1)
    uint32_t expected_entry_count = 2; // Two throttling start events
    // Expected residency calculation:
    // Cycle 1: T2_us to T4_us = (4000000 - 2000000) = 2000000 us
    // Cycle 2: T5_us to T_finalize_pkg_us = (7500000 - 5000000) = 2500000 us
    // Total residency = 2000000 + 2500000 = 4500000 us = 4500 mS
    // Converting to milliseconds: 4500000 us / 1000 = 4500 mS
    // Note: The residency calculation includes the finalize timestamp to ensure accuracy.
    uint32_t expected_residency_mS = ((T4_us - T2_us) + (T_finalize_pkg_us - T5_us)) / 1000;
    bool expected_throttling_status = true; // Throttling should be active (2nd cycle still running)
    bool expected_tracker_status = true;    // Tracker should be active for 2nd cycle

    if (g_print_logs)
    {
        printf("\n=== Results (Temperature Multiple Cycles) ===\n");
        printf("Temperature Throttling entry_count: %d (expected=%d)\n", throttle_array->entry_count, expected_entry_count);
        printf("Temperature Throttling residency: %d ms (expected=%d ms)\n", throttle_array->residency_mS, expected_residency_mS);
        printf("Temperature throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Temperature throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0 ? 1 : 0,
               expected_tracker_status);
    }

    // Verify results
    assert_int_equal(expected_entry_count, throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, throttle_array->residency_mS);
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(expected_tracker_status,
                     core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] >
                         0); // Temperature throttling tracker should be active

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_temperature_throttling_cycles ---\n");
        printf("***\n");
    }
}

// Test simultaneous current and temperature throttling: both throttling types active at the same time
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START (current throttling begins)
// T2: TEMP_THROTTLING_START (temperature throttling begins, both active)
// T3: CURRENT_THROTTLING_END (current throttling ends)
// T3: TEMP_THROTTLING_END (temperature throttling ends)
// --- FINAL CHECK: both entry_count=1, both active=false (Simultaneous Test Complete) ---
TEST_FUNCTION(test_core_simultaneous_current_and_temperature_throttling, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_simultaneous_current_and_temperature_throttling ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_finalize_us = 4000000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Current Throttling START
    pstate_telem_t pstate_current_start = {0};
    core_current_t current_current_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_start, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_current_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Temperature Throttling START (now both types are active)
    pstate_telem_t pstate_temp_start = {0};
    core_current_t current_temp_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_start, core_index, 10, 20, TEMP_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_temp_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 3: TEMP_THROTTLING_START (both types now active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Current Throttling END
    pstate_telem_t pstate_current_end = {0};
    core_current_t current_current_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_end, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_current_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 4: CURRENT_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Temperature Throttling END
    pstate_telem_t pstate_temp_end = {0};
    core_current_t current_temp_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_end, core_index, 11, 25, TEMP_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_temp_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 5: TEMP_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Calling finalize with a later timestamp should NOT affect residency for closed cycles.
    set_next_finalize_timestamp(T4_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used (for debug only)
    if (g_print_logs)
        printf("Finalize called at T=%" PRIu32 " us (should not affect closed cycle residency calculation)\n",
               T_finalize_pkg_us);

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* current_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_CURRENT]; // current
    pwr_core_element_throttle_t* temp_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_TEMPERATURE]; // temperature

    // Expected values for both throttling types
    uint32_t expected_entry_count = 1; // One start event for each type
    // expected_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    uint32_t expected_residency_mS = (T3_us - T2_us) / 1000; // 1000 mS
    bool expected_throttling_status = false;                 // Both should be inactive after END events
    bool expected_current_tracker = false;                   // Current tracker should be inactive
    bool expected_temp_tracker = false;                      // Temperature tracker should be inactive

    if (g_print_logs)
    {
        printf("\n=== Results (Simultaneous Current & Temperature) ===\n");
        printf("Current Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               current_throttle_array->entry_count,
               expected_entry_count,
               current_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Temperature Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               temp_throttle_array->entry_count,
               expected_entry_count,
               temp_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0 ? 1 : 0,
               expected_current_tracker);
        printf("Temperature throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0 ? 1 : 0,
               expected_temp_tracker);
        printf("Overall throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
    }

    // Verify results for both throttling types
    assert_int_equal(expected_entry_count, current_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, current_throttle_array->residency_mS);
    assert_int_equal(expected_current_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0);

    assert_int_equal(expected_entry_count, temp_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, temp_throttle_array->residency_mS);
    assert_int_equal(expected_temp_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0);

    // Verify overall throttling status (should be false since both throttling types ended)
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);

    if (g_print_logs)
    {
        printf("--- END test_core_simultaneous_current_and_temperature_throttling ---\n");
        printf("***\n");
    }
}

// Test simultaneous current and temperature throttling with NO_THROTTLING at the end instead of ending each throttling separately
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START (current throttling begins)
// T2: TEMP_THROTTLING_START (temperature throttling begins, both active)
// T3: NO_THROTTLING (both throttling types end simultaneously)
// --- FINAL CHECK: both entry_count=1, both active=false (NO_THROTTLING End Test Complete) ---
TEST_FUNCTION(test_core_simultaneous_current_and_temperature_throttling_with_no_throttling, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf(
            "--- START test_core_simultaneous_current_and_temperature_throttling_with_no_throttling ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_finalize_us = 4000000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Current Throttling START
    pstate_telem_t pstate_current_start = {0};
    core_current_t current_current_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_start, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_current_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Temperature Throttling START (now both types are active)
    pstate_telem_t pstate_temp_start = {0};
    core_current_t current_temp_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_start, core_index, 10, 20, TEMP_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_temp_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 3: TEMP_THROTTLING_START (both types now active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Both throttling types END together by sending NO_THROTTLING
    pstate_telem_t pstate_no_throttle = {0};
    core_current_t current_no_throttle = {0};
    setup_mock_pstate_data_with_throttling(&pstate_no_throttle, core_index, 11, 25, NO_THROTTLING, 0, T3_us);
    setup_mock_current_data(&current_no_throttle, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 4: NO_THROTTLING (both types end simultaneously)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Calling finalize with a later timestamp should NOT affect residency for closed cycles.
    set_next_finalize_timestamp(T4_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used (for debug only)
    if (g_print_logs)
        printf("Finalize called at T=%" PRIu32 " us (should not affect closed cycle residency calculation)\n",
               T_finalize_pkg_us);

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* current_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_CURRENT]; // current
    pwr_core_element_throttle_t* temp_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_TEMPERATURE]; // temperature

    // Expected values for both throttling types
    uint32_t expected_entry_count = 1; // One start event for each type
    // expected_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    uint32_t expected_residency_mS = (T3_us - T2_us) / 1000; // 1000 mS
    bool expected_throttling_status = false;                 // Both should be inactive after NO_THROTTLING
    bool expected_current_tracker = false;                   // Current tracker should be inactive
    bool expected_temp_tracker = false;                      // Temperature tracker should be inactive

    if (g_print_logs)
    {
        printf("\n=== Results (Simultaneous Current & Temperature, NO_THROTTLING End) ===\n");
        printf("Current Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               current_throttle_array->entry_count,
               expected_entry_count,
               current_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Temperature Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               temp_throttle_array->entry_count,
               expected_entry_count,
               temp_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0 ? 1 : 0,
               expected_current_tracker);
        printf("Temperature throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0 ? 1 : 0,
               expected_temp_tracker);
        printf("Overall throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
    }

    // Verify results for both throttling types
    assert_int_equal(expected_entry_count, current_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, current_throttle_array->residency_mS);
    assert_int_equal(expected_current_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0);

    assert_int_equal(expected_entry_count, temp_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, temp_throttle_array->residency_mS);
    assert_int_equal(expected_temp_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0);

    // Verify overall throttling status (should be false since NO_THROTTLING ended both types)
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);

    if (g_print_logs)
    {
        printf("--- END test_core_simultaneous_current_and_temperature_throttling_with_no_throttling ---\n");
        printf("***\n");
    }
}

// Test overlapping current and temperature throttling with current restart: complex lifecycle management
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START (current starts)
// T3: TEMP_THROTTLING_START (both active)
// T4: CURRENT_THROTTLING_END (current ends, temp continues)
// T5: CURRENT_THROTTLING_START (current starts again, both active again)
// T6: TEMP_THROTTLING_END (temp ends, current continues)
// T7: CURRENT_THROTTLING_END (current ends, both inactive)
// --- FINAL CHECK: Current entry_count=2, Temp entry_count=1, both active=false (Overlapping Restart Test Complete) ---
//
// Expected Results:
// - Current: 2 entry_count, residency = (T4-T2) + (T7-T5)
// - Temperature: 1 entry_count, residency = (T6-T3)
// - Tests cumulative residency, independent lifecycle management, and overlapping throttling with restarts
TEST_FUNCTION(test_core_overlapping_current_and_temperature_throttling_with_current_restart, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf(
            "--- START test_core_overlapping_current_and_temperature_throttling_with_current_restart ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000;
    uint64_t T5_us = 5000000, T6_us = 6000000, T7_us = 7000000, T8_finalize_us = 8000000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: T1 - NO_THROTTLING (baseline)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Current Throttling START
    pstate_telem_t pstate_current_start1 = {0};
    core_current_t current_current_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_start1, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_current_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: T2 - CURRENT_THROTTLING_START (current starts)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Temperature Throttling START (both active)
    pstate_telem_t pstate_temp_start = {0};
    core_current_t current_temp_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_start, core_index, 11, 21, TEMP_THROTTLING_START, 0, T3_us);
    setup_mock_current_data(&current_temp_start, core_index, T3_us, 105, 85, 125, 105, 160, 11);
    if (g_print_logs)
        printf("Processing step 3: T3 - TEMP_THROTTLING_START (both active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Current Throttling END (temp continues)
    pstate_telem_t pstate_current_end1 = {0};
    core_current_t current_current_end1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_end1, core_index, 12, 22, CURRENT_THROTTLING_END, 0, T4_us);
    setup_mock_current_data(&current_current_end1, core_index, T4_us, 110, 90, 130, 110, 170, 12);
    if (g_print_logs)
        printf("Processing step 4: T4 - CURRENT_THROTTLING_END (current ends, temp continues)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Current Throttling START again (both active again)
    pstate_telem_t pstate_current_start2 = {0};
    core_current_t current_current_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_start2, core_index, 13, 23, CURRENT_THROTTLING_START, 0, T5_us);
    setup_mock_current_data(&current_current_start2, core_index, T5_us, 115, 95, 135, 115, 180, 13);
    if (g_print_logs)
        printf("Processing step 5: T5 - CURRENT_THROTTLING_START (current starts again, both active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 6: Temperature Throttling END (current continues)
    pstate_telem_t pstate_temp_end = {0};
    core_current_t current_temp_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_temp_end, core_index, 14, 24, TEMP_THROTTLING_END, 0, T6_us);
    setup_mock_current_data(&current_temp_end, core_index, T6_us, 120, 100, 140, 120, 190, 14);
    if (g_print_logs)
        printf("Processing step 6: T6 - TEMP_THROTTLING_END (temp ends, current continues)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 7: Current Throttling END (both inactive)
    pstate_telem_t pstate_current_end2 = {0};
    core_current_t current_current_end2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_end2, core_index, 15, 25, CURRENT_THROTTLING_END, 0, T7_us);
    setup_mock_current_data(&current_current_end2, core_index, T7_us, 125, 105, 145, 125, 200, 15);
    if (g_print_logs)
        printf("Processing step 7: T7 - CURRENT_THROTTLING_END (current ends, both inactive)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Calling finalize with a later timestamp should NOT affect residency for closed cycles.
    set_next_finalize_timestamp(T8_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used (for debug only)
    if (g_print_logs)
        printf("Finalize called at T=%" PRIu32 " us (should not affect closed cycle residency calculation)\n",
               T_finalize_pkg_us);

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* current_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_CURRENT];
    pwr_core_element_throttle_t* temp_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_TEMPERATURE];

    // Expected values calculation
    uint32_t expected_current_entry_count = 2; // Two current throttling start events (T2_us, T5_us)
    uint32_t expected_temp_entry_count = 1;    // One temperature throttling start event (T3_us)

    // expected_current_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    // Current residency: (T4_us-T2_us) + (T7_us-T5_us) = (4000000-2000000) + (7000000-5000000) = 2000000 + 2000000 = 4000 mS
    uint32_t expected_current_residency_mS = ((T4_us - T2_us) + (T7_us - T5_us)) / 1000;

    // expected_temp_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    // Temperature residency: (T6_us-T3_us) = (6000000-3000000) = 3000 mS
    uint32_t expected_temp_residency_mS = (T6_us - T3_us) / 1000;

    bool expected_throttling_status = false; // Both should be inactive after all END events
    bool expected_current_tracker = false;   // Current tracker should be inactive
    bool expected_temp_tracker = false;      // Temperature tracker should be inactive

    if (g_print_logs)
    {
        printf("\n=== Results (Overlapping Current & Temperature with Current Restart) ===\n");
        printf("Current Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               current_throttle_array->entry_count,
               expected_current_entry_count,
               current_throttle_array->residency_mS,
               expected_current_residency_mS);
        printf("Temperature Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               temp_throttle_array->entry_count,
               expected_temp_entry_count,
               temp_throttle_array->residency_mS,
               expected_temp_residency_mS);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0 ? 1 : 0,
               expected_current_tracker);
        printf("Temperature throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0 ? 1 : 0,
               expected_temp_tracker);
        printf("Overall throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Timeline validation:\n");
        printf("  Current cycle 1: T2-T4 = %" PRIu64 " ms\n", (T4_us - T2_us) / 1000);
        printf("  Current cycle 2: T5-T7 = %" PRIu64 " ms\n", (T7_us - T5_us) / 1000);
        printf("  Temperature cycle: T3-T6 = %" PRIu64 " ms\n", (T6_us - T3_us) / 1000);
    }

    // Verify current throttling results
    assert_int_equal(expected_current_entry_count, current_throttle_array->entry_count);
    assert_int_equal(expected_current_residency_mS, current_throttle_array->residency_mS);
    assert_int_equal(expected_current_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0);

    // Verify temperature throttling results
    assert_int_equal(expected_temp_entry_count, temp_throttle_array->entry_count);
    assert_int_equal(expected_temp_residency_mS, temp_throttle_array->residency_mS);
    assert_int_equal(expected_temp_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] > 0);

    // Verify overall throttling status (should be false since all throttling ended)
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);

    if (g_print_logs)
    {
        printf("--- END test_core_overlapping_current_and_temperature_throttling_with_current_restart ---\n");
        printf("***\n");
    }
}

// Test multiple adaptive clocking throttling with restart: 2 start events (second ignored), end, then restart
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: ADPT_CLK_THROTTLING_START #1 (cycle 1 begins, entry_count=1)
// T3: ADPT_CLK_THROTTLING_START #2 (duplicate, ignored for entry_count but residency accumulates)
// T4: ADPT_CLK_THROTTLING_START #3 (duplicate, ignored for entry_count but residency accumulates)
// T5: ADPT_CLK_THROTTLING_END (cycle 1 complete)
// T6: ADPT_CLK_THROTTLING_START #4 (cycle 2 begins, entry_count=2, remains active)
// T6_finalize_pkg_us: Finalize timestamp for residency calculation (set via time_t0 before finalize)
// --- FINAL CHECK: entry_count=2, active=true (Duplicate START Handling + Multiple Cycles Test) ---
TEST_FUNCTION(test_core_multiple_adaptive_clocking_throttling_cycles, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_adaptive_clocking_throttling_cycles ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_us = 5000000,
             T6_finalize_pkg_us = 6500000;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Adaptive Clocking Throttling START #1
    pstate_telem_t pstate_ac_start1 = {0};
    core_current_t current_ac_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_ac_start1, core_index, 10, 20, ADPT_CLK_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_ac_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: ADPT_CLK_THROTTLING_START #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after ADPT_CLK_THROTTLING_START #1
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Adaptive Clocking Throttling START #2 (Tests duplicate start handling and residency accumulation)
    pstate_telem_t pstate_ac_start2 = {0};
    core_current_t current_ac_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_ac_start2, core_index, 11, 21, ADPT_CLK_THROTTLING_START, 0, T3_us);
    setup_mock_current_data(&current_ac_start2, core_index, T3_us, 105, 85, 125, 105, 160, 11);
    if (g_print_logs)
        printf("Processing step 3: ADPT_CLK_THROTTLING_START #2 (should be ignored - duplicate)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Continue with throttling active (same START status to accumulate residency)
    pstate_telem_t pstate_ac_continue = {0};
    core_current_t current_ac_continue = {0};
    setup_mock_pstate_data_with_throttling(&pstate_ac_continue, core_index, 12, 22, ADPT_CLK_THROTTLING_START, 0, T4_us);
    setup_mock_current_data(&current_ac_continue, core_index, T4_us, 110, 90, 130, 110, 170, 12);
    if (g_print_logs)
        printf("Processing step 4: ADPT_CLK_THROTTLING_START continues (residency accumulation)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Adaptive Clocking Throttling END
    pstate_telem_t pstate_ac_end = {0};
    core_current_t current_ac_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_ac_end, core_index, 13, 23, ADPT_CLK_THROTTLING_END, 0, T5_us);
    setup_mock_current_data(&current_ac_end, core_index, T5_us, 115, 95, 135, 115, 180, 13);
    if (g_print_logs)
        printf("Processing step 5: ADPT_CLK_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Check tracker status after END and finalize - should be inactive
    bool expected_tracker_status_before_starting_new_one = false; // Should be inactive after END
    if (g_print_logs)
    {
        printf("\n=== Intermediate Check (After END and Finalize) ===\n");
        printf("Adaptive Clocking throttling tracker after END+finalize: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] > 0 ? 1 : 0,
               expected_tracker_status_before_starting_new_one);
    }
    // Verify tracker is inactive after END and finalize
    assert_int_equal(expected_tracker_status_before_starting_new_one,
                     core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] > 0);

    // Step 6: Adaptive Clocking Throttling START #3 (new cycle starts - should make tracker active again)
    uint64_t T6_us = 6000000;
    pstate_telem_t pstate_ac_start3 = {0};
    core_current_t current_ac_start3 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_ac_start3, core_index, 14, 24, ADPT_CLK_THROTTLING_START, 0, T6_us);
    setup_mock_current_data(&current_ac_start3, core_index, T6_us, 120, 100, 140, 120, 190, 14);
    if (g_print_logs)
        printf("Processing step 6: ADPT_CLK_THROTTLING_START #3 (new cycle starts - tracker should be "
               "active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // --- Residency accumulation test for finalize ---
    // Set next finalize timestamp for residency calculation
    set_next_finalize_timestamp(T6_finalize_pkg_us);
    // Calling data_smpl_finalize_pwr_pkg_metrics() before package creation to finalize residency for any throttling source still active (START sent, no END).
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_ADAPTIVE_CLK];

    // Expected values for adaptive clocking throttling (THROTTLE_SOURCE_ADAPTIVE_CLK = 4)
    uint32_t expected_entry_count = 2; // TWO valid start events (T2 and T6, duplicate at T3 ignored)
    // Completed cycle residency: (T5-T2) + (T_finalize_pkg_us - T6_us) / 1000
    uint32_t expected_residency_mS = ((T5_us - T2_us) + (T_finalize_pkg_us - T6_us)) / 1000;
    bool expected_throttling_status = true; // Throttling should be active after T6 START event
    bool expected_tracker_status = true;    // Tracker should be active after T6 START event

    if (g_print_logs)
    {
        printf("\n=== Results (Adaptive Clocking with Duplicate Start + Restart Test) ===\n");
        printf("Adaptive Clocking Throttling entry_count: %d (expected=%d)\n", throttle_array->entry_count, expected_entry_count);
        printf("Adaptive Clocking Throttling residency: %d ms (expected=%d ms)\n", throttle_array->residency_mS, expected_residency_mS);
        printf("Adaptive Clocking throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Adaptive Clocking throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] > 0 ? 1 : 0,
               expected_tracker_status);
        printf("Timeline: T2(START#1) -> T3(START#2 ignored) -> T4(continues) -> T5(END) -> T6(START#3 "
               "active)\n");
        printf("Completed cycle residency: T5-T2 = %" PRIu64 " ms\n", (T5_us - T2_us) / 1000);
        printf("NOTE: Duplicate START at T3 ignored, T6 START creates new active cycle\n");
    }

    // Verify results
    assert_int_equal(expected_entry_count, throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, throttle_array->residency_mS);
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(expected_tracker_status,
                     core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] >
                         0); // Adaptive clocking throttling tracker should be active after T6 START

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_adaptive_clocking_throttling_cycles ---\n");
        printf("***\n");
    }
}

// Test multiple rack throttling cycles: Tests both basic functional scenario and multiple cycles
// First: Complete cycle (START → END) validates basic functional test scenario
// Then: Start second cycle that remains active, validating multiple cycle behavior
// This rack_throttling_functional + multiple_rack_throttling_cycles scenario
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: RACK_THROTTLING_START #1 (priority=2)
// T3: RACK_THROTTLING_START continues (residency accumulation)
// T4: RACK_THROTTLING_END #1 (complete cycle)
// --- INTERMEDIATE CHECK: status=false, tracker=false (Basic Functional Test) ---
// T5: RACK_THROTTLING_START #2 (priority=3, new cycle begins)
// T_finalize_pkg_us: Finalize timestamp for residency calculation (set via time_t0 before finalize)
// --- FINAL CHECK: entry_count=2, active=true (Multiple Cycles Test) ---

TEST_FUNCTION(test_core_multiple_rack_throttling_cycles, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_rack_throttling_cycles ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_us = 5000000,
             T6_finalize_pkg_us = 6500000;
    uint8_t rack_priority1 = 2, rack_priority2 = 3; // Different rack priorities

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Rack Throttling START #1
    pstate_telem_t pstate_rack_start1 = {0};
    core_current_t current_rack_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_start1, core_index, 10, 20, RACK_THROTTLING_START, rack_priority1, T2_us);
    setup_mock_current_data(&current_rack_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: RACK_THROTTLING_START #1 (priority=%d)\n", rack_priority1);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after RACK_THROTTLING_START #1
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Rack Throttling continues (same START status) - to test residency accumulation
    pstate_telem_t pstate_rack_continue1 = {0};
    core_current_t current_rack_continue1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_continue1, core_index, 11, 21, RACK_THROTTLING_START, rack_priority1, T3_us);
    setup_mock_current_data(&current_rack_continue1, core_index, T3_us, 105, 85, 125, 105, 160, 11);
    if (g_print_logs)
        printf("Processing step 3: RACK_THROTTLING_START continues (residency accumulation test)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Rack Throttling END #1
    pstate_telem_t pstate_rack_end1 = {0};
    core_current_t current_rack_end1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_end1, core_index, 11, 25, RACK_THROTTLING_END, rack_priority1, T4_us);
    setup_mock_current_data(&current_rack_end1, core_index, T4_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 4: RACK_THROTTLING_END #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Check entry count after first complete cycle
    pwr_core_record_throttle_t throttle_record_after_first_cycle = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record_after_first_cycle);
    pwr_core_element_throttle_t* throttle_array_after_first_cycle =
        &throttle_record_after_first_cycle.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_RACK_LIMIT];

    uint32_t expected_entry_count_after_first_cycle = 1; // One complete cycle

    // Intermediate Check: Verify first complete cycle results (equivalent to basic functional test)
    bool expected_throttling_status_after_first_cycle = false; // Should be inactive after END event
    bool expected_tracker_status_after_first_cycle = false;    // Tracker should be inactive after END event

    if (g_print_logs)
    {
        printf("\n=== Intermediate Check (After First Complete Cycle) ===\n");
        printf("Entry count after first cycle: %d (expected=%d)\n",
               throttle_array_after_first_cycle->entry_count,
               expected_entry_count_after_first_cycle);
        printf("Rack throttling status after first cycle: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status_after_first_cycle);
        printf("Rack throttling tracker after first cycle: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] > 0 ? 1 : 0,
               expected_tracker_status_after_first_cycle);
    }

    // Verify intermediate state (validates basic functional test scenario)
    assert_int_equal(expected_entry_count_after_first_cycle, throttle_array_after_first_cycle->entry_count);
    assert_int_equal(expected_throttling_status_after_first_cycle, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(expected_tracker_status_after_first_cycle,
                     core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] > 0);

    // Step 5: Rack Throttling START #2 (active, not ended)
    pstate_telem_t pstate_rack_start2 = {0};
    core_current_t current_rack_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_start2, core_index, 12, 22, RACK_THROTTLING_START, rack_priority2, T5_us);
    setup_mock_current_data(&current_rack_start2, core_index, T5_us, 110, 85, 125, 105, 210, 12);
    if (g_print_logs)
        printf("Processing step 5: RACK_THROTTLING_START #2 (priority=%d, remains active)\n", rack_priority2);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // --- Residency accumulation test for finalize ---
    // Set next finalize timestamp for residency calculation
    set_next_finalize_timestamp(T6_finalize_pkg_us);
    // Calling data_smpl_finalize_pwr_pkg_metrics() before package creation to finalize residency for any throttling source still active (START sent, no END).
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used

    // Check entry count after second cycle START
    pwr_core_record_throttle_t throttle_record_after_second_start = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record_after_second_start);
    pwr_core_element_throttle_t* throttle_array_after_second_start =
        &throttle_record_after_second_start.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_RACK_LIMIT];

    uint32_t expected_entry_count_after_second_start = 2; // Two START events (both cycles)

    if (g_print_logs)
    {
        printf("\n=== Check After Second Cycle START ===\n");
        printf("Entry count after second START: %d (expected=%d)\n",
               throttle_array_after_second_start->entry_count,
               expected_entry_count_after_second_start);
    }

    // Verify entry count after second cycle
    assert_int_equal(expected_entry_count_after_second_start, throttle_array_after_second_start->entry_count);

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_RACK_LIMIT];

    // Expected values for rack throttling (THROTTLE_SOURCE_RACK_LIMIT = 2)
    uint32_t expected_entry_count = 2; // Two throttling start events
    // expected_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    // Cycles considered in residency calculation:
    // T4_us-T2_us = 2000 mS (includes intermediate step) + ongoing cycle: T_finalize_pkg_us - T5_us
    uint32_t expected_residency_mS = ((T4_us - T2_us) + (T_finalize_pkg_us - T5_us)) / 1000;
    bool expected_throttling_status = true; // Throttling should be active (2nd cycle still running)
    bool expected_tracker_status = true;    // Tracker should be active as 2nd cycle is still running

    if (g_print_logs)
    {
        printf("\n=== Final Results (Combined Basic + Multiple Cycles Test) ===\n");
        printf("Rack Throttling entry_count: %d (expected=%d)\n", throttle_array->entry_count, expected_entry_count);
        printf("Rack Throttling residency: %d ms (expected=%d ms)\n", throttle_array->residency_mS, expected_residency_mS);
        printf("Rack throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Rack throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] > 0 ? 1 : 0,
               expected_tracker_status);
        printf("Latest rack priority: %d\n", core_rt[core_index].latest_rack_throttle_priority);
        printf("NOTE: This test validates both basic functional and multiple cycles scenarios\n");
    }

    // Verify results
    assert_int_equal(expected_entry_count, throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, throttle_array->residency_mS);
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);
    assert_int_equal(expected_tracker_status,
                     core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] >
                         0); // Rack throttling tracker should be active

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_rack_throttling_cycles ---\n");
        printf("***\n");
    }
}

// Test simultaneous rack and current throttling: both throttling types active at the same time
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_THROTTLING_START (current throttling begins)
// T2: RACK_THROTTLING_START (rack throttling begins, both active, priority=2)
// T3: CURRENT_THROTTLING_END (current throttling ends)
// T3: RACK_THROTTLING_END (rack throttling ends)
// --- FINAL CHECK: both entry_count=1, both active=false (Simultaneous Rack+Current Test Complete) ---
TEST_FUNCTION(test_core_simultaneous_rack_and_current_throttling, test_setup, test_teardown)
{
    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_simultaneous_rack_and_current_throttling ---\n");
    }

    uint8_t core_index = 0;
    // Timestamps are in microseconds; residency is processed in milliseconds (conversion happens in package_create_pwr_core_throttle_record, compute_metrics.c).
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_finalize_us = 4000000;
    uint8_t rack_priority = 2;

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Current Throttling START
    pstate_telem_t pstate_current_start = {0};
    core_current_t current_current_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_start, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_current_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after CURRENT_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 3: Rack Throttling START (now both types are active)
    pstate_telem_t pstate_rack_start = {0};
    core_current_t current_rack_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_start, core_index, 10, 20, RACK_THROTTLING_START, rack_priority, T2_us);
    setup_mock_current_data(&current_rack_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    if (g_print_logs)
        printf("Processing step 3: RACK_THROTTLING_START (both types now active, priority=%d)\n", rack_priority);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Assert that throttling status is active after RACK_THROTTLING_START
    assert_true(core_rt[core_index].status_flags.throttle_is_active);

    // Step 4: Current Throttling END
    pstate_telem_t pstate_current_end = {0};
    core_current_t current_current_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_end, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_current_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 4: CURRENT_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Rack Throttling END
    pstate_telem_t pstate_rack_end = {0};
    core_current_t current_rack_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_rack_end, core_index, 11, 25, RACK_THROTTLING_END, rack_priority, T3_us);
    setup_mock_current_data(&current_rack_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    if (g_print_logs)
        printf("Processing step 5: RACK_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Calling finalize with a later timestamp should NOT affect residency for closed cycles.
    set_next_finalize_timestamp(T4_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used (for debug only)
    if (g_print_logs)
        printf("Finalize called at T=%" PRIu32 " us (should not affect closed cycle residency calculation)\n",
               T_finalize_pkg_us);

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* current_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_CURRENT]; // current
    pwr_core_element_throttle_t* rack_throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_RACK_LIMIT]; // rack

    // Expected values for both throttling types
    uint32_t expected_entry_count = 1; // One start event for each type
    // expected_residency_mS is in milliseconds (mS), calculated from microsecond timestamps.
    uint32_t expected_residency_mS = (T3_us - T2_us) / 1000; // 1000 mS
    bool expected_throttling_status = false;                 // Both should be inactive after END events
    bool expected_current_tracker = false;                   // Current tracker should be inactive
    bool expected_rack_tracker = false;                      // Rack tracker should be inactive

    if (g_print_logs)
    {
        printf("\n=== Results (Simultaneous Current & Rack) ===\n");
        printf("Current Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               current_throttle_array->entry_count,
               expected_entry_count,
               current_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Rack Throttling: entry_count=%d (expected=%d), residency=%d ms (expected=%d ms)\n",
               rack_throttle_array->entry_count,
               expected_entry_count,
               rack_throttle_array->residency_mS,
               expected_residency_mS);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0 ? 1 : 0,
               expected_current_tracker);
        printf("Rack throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] > 0 ? 1 : 0,
               expected_rack_tracker);
        printf("Overall throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
    }

    // Verify results for both throttling types
    assert_int_equal(expected_entry_count, current_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, current_throttle_array->residency_mS);
    assert_int_equal(expected_current_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] > 0);

    assert_int_equal(expected_entry_count, rack_throttle_array->entry_count);
    assert_int_equal(expected_residency_mS, rack_throttle_array->residency_mS);
    assert_int_equal(expected_rack_tracker, core_rt[core_index].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] > 0);

    // Verify overall throttling status (should be false since both throttling types ended)
    assert_int_equal(expected_throttling_status, core_rt[core_index].status_flags.throttle_is_active);

    if (g_print_logs)
    {
        printf("--- END test_core_simultaneous_rack_and_current_throttling ---\n");
        printf("***\n");
    }
}

// Test multiple CURRENT_OVERRUN cycles: 3 events, finalize, check overrun count
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: CURRENT_OVERRUN event #1
// T3: CURRENT_OVERRUN event #2
// T4: CURRENT_OVERRUN event #3
// --- FINAL CHECK: overrun_count=3 (Multiple Overrun Events, No Residency or Entry Count)
// All other throttle_element fields (entry_count, residency_mS, max_pstate, avg_pstate) are asserted to be zero.
TEST_FUNCTION(test_core_multiple_current_overrun_cycles, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_current_overrun_cycles ---\n");
    }
    uint8_t core_index = 0;
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_finalize_us = 5000000;
    setup_mock_gtimer_frequency();

    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return_always(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 2: CURRENT_OVERRUN event #1
    pstate_telem_t pstate_current_overrun1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_overrun1, core_index, 10, 20, CURRENT_THROTTLING_OVERRUN, 0, T2_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 3: CURRENT_OVERRUN event #2
    pstate_telem_t pstate_current_overrun2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_overrun2, core_index, 10, 20, CURRENT_THROTTLING_OVERRUN, 0, T3_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 4: CURRENT_OVERRUN event #3
    pstate_telem_t pstate_current_overrun3 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_current_overrun3, core_index, 10, 20, CURRENT_THROTTLING_OVERRUN, 0, T4_us);
    data_smpl_process_pstate_sensor_fifo();

    // Finalize timestamp for package boundary, Not being used for any calculation as we are only validating current_overrun_count
    set_next_finalize_timestamp(T5_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0;
    if (g_print_logs)
        printf("Finalize timestamp used for overrun count: %u us\n", T_finalize_pkg_us);

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_CURRENT_OVERRUN];

    uint16_t expected_current_overrun_count = 3;

    if (g_print_logs)
    {
        printf("\n=== Results (Multiple CURRENT_OVERRUN Cycles) ===\n");
        printf("CURRENT_OVERRUN overrun_count: %d (expected=%d)\n", throttle_array->overrun_count, expected_current_overrun_count);
        printf("CURRENT_OVERRUN entry_count: %d (expected=0)\n", throttle_array->entry_count);
        printf("CURRENT_OVERRUN residency_mS: %d (expected=0)\n", throttle_array->residency_mS);
        printf("CURRENT_OVERRUN max_pstate: %d (expected=0)\n", throttle_array->max_pstate);
        printf("CURRENT_OVERRUN avg_pstate: %d (expected=0)\n", throttle_array->avg_pstate);
    }

    // Verify overrun_count
    assert_int_equal(expected_current_overrun_count, throttle_array->overrun_count);
    // Verify all other fields including max_pstate and avg_pstate are zero especially for overrun events
    assert_int_equal(0, throttle_array->entry_count);
    assert_int_equal(0, throttle_array->residency_mS);
    assert_int_equal(0, throttle_array->max_pstate);
    assert_int_equal(0, throttle_array->avg_pstate);

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_current_overrun_cycles ---\n");
        printf("***\n");
    }
}

// Test multiple ADPT_CLK_THROTTLING_OVERRUN cycles: 3 events, check overrun count
//
// Test Flow Summary:
// ==================
// T1: NO_THROTTLING (baseline)
// T2: ADAPTIVE_CLK_OVERRUN event #1
// T3: ADPT_CLK_THROTTLING_OVERRUN event #2
// T4: ADPT_CLK_THROTTLING_OVERRUN event #3
// --- FINAL CHECK: overrun_count=3 (Multiple Overrun Events, No Residency or Entry Count)
// All other throttle_element fields (entry_count, residency_mS, max_pstate, avg_pstate) are asserted to be zero.
TEST_FUNCTION(test_core_multiple_adaptive_clocking_overrun_cycles, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_adaptive_clocking_overrun_cycles ---\n");
    }

    uint8_t core_index = 0;
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_finalize_us = 5000000;
    setup_mock_gtimer_frequency();

    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return_always(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 2: ADPT_CLK_THROTTLING_OVERRUN event #1
    pstate_telem_t pstate_adaptive_overrun1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_adaptive_overrun1, core_index, 10, 20, ADPT_CLK_THROTTLING_OVERRUN, 0, T2_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 3: ADPT_CLK_THROTTLING_OVERRUN event #2
    pstate_telem_t pstate_adaptive_overrun2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_adaptive_overrun2, core_index, 10, 20, ADPT_CLK_THROTTLING_OVERRUN, 0, T3_us);
    data_smpl_process_pstate_sensor_fifo();

    // Step 4: ADPT_CLK_THROTTLING_OVERRUN event #3
    pstate_telem_t pstate_adaptive_overrun3 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_adaptive_overrun3, core_index, 10, 20, ADPT_CLK_THROTTLING_OVERRUN, 0, T4_us);
    data_smpl_process_pstate_sensor_fifo();

    // Set finalize timestamp and finalize metrics before package creation
    set_next_finalize_timestamp(T5_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();

    // Get final results
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array =
        &throttle_record.throttle_collection[core_index].throttle_element[THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN];

    uint16_t expected_adaptive_overrun_count = 3;

    if (g_print_logs)
    {
        printf("\n=== Results (Multiple ADAPTIVE_CLK_OVERRUN Cycles) ===\n");
        printf("ADAPTIVE_CLK_OVERRUN overrun_count: %d (expected=%d)\n", throttle_array->overrun_count, expected_adaptive_overrun_count);
        printf("ADAPTIVE_CLK_OVERRUN entry_count: %d (expected=0)\n", throttle_array->entry_count);
        printf("ADAPTIVE_CLK_OVERRUN residency_mS: %d (expected=0)\n", throttle_array->residency_mS);
        printf("ADAPTIVE_CLK_OVERRUN max_pstate: %d (expected=0)\n", throttle_array->max_pstate);
        printf("ADAPTIVE_CLK_OVERRUN avg_pstate: %d (expected=0)\n", throttle_array->avg_pstate);
    }

    // Verify overrun_count
    assert_int_equal(expected_adaptive_overrun_count, throttle_array->overrun_count);
    // Verify all other fields including max_pstate and avg_pstate are zero especially for overrun events
    assert_int_equal(0, throttle_array->entry_count);
    assert_int_equal(0, throttle_array->residency_mS);
    assert_int_equal(0, throttle_array->max_pstate);
    assert_int_equal(0, throttle_array->avg_pstate);

    if (g_print_logs)
    {
        printf("--- END test_core_multiple_adaptive_clocking_overrun_cycles ---\n");
        printf("***\n");
    }
}
