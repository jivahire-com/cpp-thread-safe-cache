//
// Copyright (c) Microsoft Corporation. All rights restatic int32_t test_setup(void** state)
//
/**
 * @file test_core_vm_throttling_priorities.cpp
 * Test functionality and flow for VM throttling priorities (rack throttling only)
 *
 * Objective:
 *   Validate data for throttling priorities.
 *   Note: VM throttling priorities are only for rack throttling.
 *
 * Test Flow:
 * ----------
 * - Set up mock PSTATE data with rack throttling priority
 * - Mock sensor polling calls
 * - Call: data_smpl_process_pstate_sensor_fifo()
 * - Call: package_create_pwr_core_rack_priority_record()
 * - Verify/Assert results for rack throttling priorities
 *
 * Test Functions:
 * ---------------
 * 1. test_single_priority_no_interruption: Tests single rack throttling priority without interruptions
 * 2. test_single_priority_with_finalize: Tests single rack throttling priority with finalize timestamp
 * 3. test_sequential_priorities_with_finalize: Tests sequential rack throttling priorities without overlaps
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779222

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
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
extern bool in_band_publishing_active;
extern uint8_t g_enable_mock_pstate;
}

// Global variable to control debug printing
static bool g_print_logs = true;

#define NUMBER_OF_CORES_TO_TEST (4)

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();

    // Initialize core active status - needed for throttling metrics
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_TO_TEST; core_id++)
    {
        core_is_active[core_id] = true; // Mark cores as active for compute metrics

        // Initialize core runtime state
        core_rt[core_id].pstate_from_pstate_pkt = 0x00;
        core_rt[core_id].latest_plimit = 0;
        core_rt[core_id].pstate_res_timestamp_uS = 0;
        core_rt[core_id].status_flags.throttle_is_active = false;
        core_rt[core_id].status_flags.rack_throttle_is_active = false;
        core_rt[core_id].latest_rack_throttle_priority = 0;

        // Initialize rack priority data for each core in computed_metrics_2_mins
        for (uint8_t priority = 0; priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; priority++)
        {
            core_rt[core_id].rack_pri_res_timestamp_uS[priority] = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].residency_uS = 0;
        }

        // Initialize throttling data for each core in computed_metrics_2_mins
        for (uint8_t throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
        {
            core_rt[core_id].throttle_source_tracker[throttle_source] = false;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS = 0;
        }
    }

    // Reset mock timestamp to initial value
    time_t0 = 100;
    // Enable PSTATE mock for all tests
    g_enable_mock_pstate = 1;
    // Enable in-band publishing for rack priority tracking
    in_band_publishing_active = true;

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    // Disable PSTATE mock after test completion
    g_enable_mock_pstate = 0;
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

    // Set up mock PSTATE polling calls
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

    // Set up mock current polling calls
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
// Currently not used in single priority test, but used for multi-priority tests
static void set_next_finalize_timestamp(uint64_t desired_timestamp)
{
    time_t0 = desired_timestamp - MOCK_TIMESTAMP_INCREMENT;
}

// Test single priority rack throttling with continuous operation and clean END event termination
// Validates basic priority residency calculation and entry counting without priority switching
//
// Test Flow Summary:
// ==================
// T0: NO_THROTTLING (baseline timestamp reference)
// T1: RACK_THROTTLING_START (priority 1) - Begin throttling cycle
// T2: RACK_THROTTLING_CONTINUE (priority 1) - Residency accumulation
// T3: RACK_THROTTLING_END (priority 1) - Complete the cycle
// Validates single priority residency accumulation and entry counting accuracy.
TEST_FUNCTION(test_single_priority_no_interruption, test_setup, test_teardown)
{
    const uint8_t core_index = 0;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_single_priority_no_interruption ---\n");
        printf("=== SINGLE PRIORITY TEST with residency accumulation ===\n");
    }

    // Reset state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_rack_throttle_priority = 0;
        for (uint8_t priority = 0; priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; priority++)
        {
            core_rt[core_id].rack_pri_res_timestamp_uS[priority] = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].residency_uS = 0;
        }
    }

    // Define timestamps for single priority test with residency accumulation
    uint64_t T0_us = 1000000; // NO_THROTTLING baseline
    uint64_t T1_us = 2000000; // Priority 1 START
    uint64_t T2_us = 3000000; // Priority 1 CONTINUES (residency accumulation)
    uint64_t T3_us = 4000000; // Priority 1 END

    // No droop_count mock being called because it doesn't call data_smpl_finalize_pwr_pkg_metrics() function.
    // And data_smpl_finalize_pwr_pkg_metrics() function calls droop_count mock once.

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Initial time_t0 value: %" PRIu32 " us\n", time_t0);

    // === STEP 0: NO_THROTTLING baseline ===
    pstate_telem_t pstate_baseline = {0};
    core_current_t current_baseline = {0};
    setup_mock_pstate_data_with_throttling(&pstate_baseline, core_index, 8, 15, NO_THROTTLING, 0, T0_us);
    setup_mock_current_data(&current_baseline, core_index, T0_us, 90, 70, 110, 85, 90, 8);

    if (g_print_logs)
        printf("T0: NO_THROTTLING baseline at %" PRIu64 " us\n", T0_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 1: START Priority 1 (index 0) ===
    pstate_telem_t pstate_start = {0};
    core_current_t current_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start, core_index, 10, 20, RACK_THROTTLING_START, 0, T1_us);
    setup_mock_current_data(&current_start, core_index, T1_us, 100, 80, 120, 90, 100, 10);

    if (g_print_logs)
        printf("T1: RACK_THROTTLING_START (priority=1 -> index 0) at %" PRIu64 " us\n", T1_us);

    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 1.5: CONTINUE Priority 1 (residency accumulation) ===
    pstate_telem_t pstate_continue = {0};
    core_current_t current_continue = {0};
    setup_mock_pstate_data_with_throttling(&pstate_continue, core_index, 10, 20, RACK_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_continue, core_index, T2_us, 105, 85, 125, 95, 105, 10);

    if (g_print_logs)
        printf("T2: RACK_THROTTLING_START continues (residency accumulation) at %" PRIu64 " us\n", T2_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 2: END Priority 1 (index 0) ===
    pstate_telem_t pstate_end = {0};
    core_current_t current_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end, core_index, 10, 20, RACK_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_end, core_index, T3_us, 100, 80, 120, 90, 100, 10);

    if (g_print_logs)
        printf("T3: RACK_THROTTLING_END (priority=1 -> index 0) at %" PRIu64 " us\n", T3_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === Create rack priority package ===
    pwr_core_record_rack_priorities_t rack_priority_record = {{0}};
    package_create_pwr_core_rack_priority_record(&rack_priority_record);

    // Get results from created rack priority package
    pwr_core_element_rack_priorities_t* rack_priority_array =
        &rack_priority_record.rack_priority_collection[core_index].rack_priority_element[0];

    // === Calculate expected values ===
    // Priority 1 (index 0) - Single cycle with intermediate accumulation:
    // Total residency: T1->T2->T3 = (T3_us - T1_us) = 4000000 - 2000000 = 2000000us -> 2000ms
    uint32_t expected_residency_ms = (T3_us - T1_us) / 1000;
    uint32_t expected_entry_count = 1; // One RACK_THROTTLING_START event

    if (g_print_logs)
    {
        printf("\n=== RESULTS ===\n");
        printf("Expected entry_count: %d, Actual entry_count: %d\n",
               expected_entry_count,
               rack_priority_array[0].entry_count);
        printf("Expected residency: %d ms, Actual residency: %d ms\n",
               expected_residency_ms,
               rack_priority_array[0].residency_mS);
    }

    // Verify results from package output
    assert_int_equal(expected_entry_count, rack_priority_array[0].entry_count);
    assert_int_equal(expected_residency_ms, rack_priority_array[0].residency_mS);

    if (g_print_logs)
    {
        printf("--- END test_single_priority_no_interruption ---\n");
        printf("***\n");
    }
}

// Test single priority with finalize timestamp - validates residency calculation when throttling is active at
// package boundary This test confirms proper finalize timestamp handling for active throttling events
//
// Test Flow Summary:
// ==================
// T0: NO_THROTTLING (baseline timestamp reference)
// T1: RACK_THROTTLING_START (priority 1) - Single priority cycle
// T2: RACK_THROTTLING_CONTINUE (priority 1) - Residency accumulation
// T3: RACK_THROTTLING_END (priority 1) - Complete the cycle
// T4: RACK_THROTTLING_START (priority 1) - Second cycle (will be active at boundary)
// T10: data_smpl_finalize_pwr_pkg_metrics() - finalizes active Single priority cycle
// --- FINAL CHECK: Single priority with 2 entries and residency calculated using finalize timestamp ---
// Validates finalize timestamp usage for active throttling events at package boundary.
TEST_FUNCTION(test_single_priority_with_finalize, test_setup, test_teardown)
{
    const uint8_t core_index = 0;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_single_priority_with_finalize ---\n");
        printf("=== SINGLE PRIORITY TEST with finalize timestamp ===\n");
        printf("Testing residency calculation when throttling is active at package boundary\n");
    }

    // Reset state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_rack_throttle_priority = 0;
        for (uint8_t priority = 0; priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; priority++)
        {
            core_rt[core_id].rack_pri_res_timestamp_uS[priority] = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].residency_uS = 0;
        }
    }

    // Timestamps for test with finalize timestamp usage
    uint64_t T0_us = 1000000;           // 1000ms: NO_THROTTLING baseline
    uint64_t T1_us = 2000000;           // 2000ms: Priority 1 START (cycle 1)
    uint64_t T2_us = 3000000;           // 3000ms: Priority 1 CONTINUES (residency accumulation)
    uint64_t T3_us = 4000000;           // 4000ms: Priority 1 END (cycle 1 completes - 2000ms duration)
    uint64_t T4_us = 5000000;           // 5000ms: Priority 1 START (cycle 2 - will be active at boundary)
    uint64_t T10_finalize_us = 7000000; // 7000ms: Finalize (cycle 2 active - 2000ms duration)

    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Initial time_t0 value: %" PRIu32 " us\n", time_t0);

    // === STEP 0: NO_THROTTLING baseline ===
    pstate_telem_t pstate_baseline = {0};
    core_current_t current_baseline = {0};
    setup_mock_pstate_data_with_throttling(&pstate_baseline, core_index, 8, 15, NO_THROTTLING, 0, T0_us);
    setup_mock_current_data(&current_baseline, core_index, T0_us, 90, 70, 110, 85, 90, 8);

    if (g_print_logs)
        printf("T0: NO_THROTTLING baseline at %" PRIu64 " us\n", T0_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 1: START Priority 1 (index 0) - Cycle 1 ===
    pstate_telem_t pstate_start_cycle1 = {0};
    core_current_t current_start_cycle1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start_cycle1, core_index, 10, 20, RACK_THROTTLING_START, 0, T1_us);
    setup_mock_current_data(&current_start_cycle1, core_index, T1_us, 100, 80, 120, 90, 100, 10);

    if (g_print_logs)
        printf("T1: RACK_THROTTLING_START (priority=1 -> index 0) cycle 1 at %" PRIu64 " us\n", T1_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 2: CONTINUE Priority 1 (residency accumulation test) ===
    pstate_telem_t pstate_continue = {0};
    core_current_t current_continue = {0};
    setup_mock_pstate_data_with_throttling(&pstate_continue, core_index, 10, 20, RACK_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_continue, core_index, T2_us, 105, 85, 125, 95, 105, 10);

    if (g_print_logs)
        printf("T2: RACK_THROTTLING_START continues (residency accumulation test) at %" PRIu64 " us\n", T2_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 3: END Priority 1 (index 0) - Complete Cycle 1 ===
    pstate_telem_t pstate_end_cycle1 = {0};
    core_current_t current_end_cycle1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end_cycle1, core_index, 10, 20, RACK_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_end_cycle1, core_index, T3_us, 100, 80, 120, 90, 100, 10);

    if (g_print_logs)
        printf("T3: RACK_THROTTLING_END (priority=1 -> index 0) cycle 1 complete at %" PRIu64 " us\n", T3_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 4: START Priority 1 (index 0) - Cycle 2 (will be active at boundary) ===
    pstate_telem_t pstate_start_cycle2 = {0};
    core_current_t current_start_cycle2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start_cycle2, core_index, 12, 22, RACK_THROTTLING_START, 0, T4_us);
    setup_mock_current_data(&current_start_cycle2, core_index, T4_us, 110, 90, 130, 95, 110, 12);

    if (g_print_logs)
        printf("T4: RACK_THROTTLING_START (priority=1 -> index 0) cycle 2 at %" PRIu64
               " us (will be active at boundary)\n",
               T4_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === FINALIZE: Complete active throttling at package boundary ===
    set_next_finalize_timestamp(T10_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used

    if (g_print_logs)
        printf("T10: data_smpl_finalize_pwr_pkg_metrics() called with finalize timestamp: %u us\n", T_finalize_pkg_us);

    // === Create rack priority package ===
    pwr_core_record_rack_priorities_t rack_priority_record = {{0}};
    package_create_pwr_core_rack_priority_record(&rack_priority_record);

    // Get results from rack priority package
    pwr_core_element_rack_priorities_t* rack_priority_array =
        &rack_priority_record.rack_priority_collection[core_index].rack_priority_element[0];

    // === Calculate expected values ===
    // Priority 1 (index 0) - Two cycles:
    //   Cycle 1: T1->T3 = 4000000μs - 2000000μs = 2000000μs = 2000ms (complete cycle)
    //   Cycle 2: T4->T_finalize_pkg_us = T_finalize_pkg_us - 5000000μs (active at boundary, uses finalize
    //   timestamp) Total: 2000ms + (T_finalize_pkg_us - T4_us) / 1000 ms
    uint32_t expected_residency_ms = ((T3_us - T1_us) + (T_finalize_pkg_us - T4_us)) / 1000;
    uint32_t expected_entry_count = 2; // Two RACK_THROTTLING_START events

    if (g_print_logs)
    {
        printf("\n=== RESULTS ===\n");
        printf("Expected entry_count: %d, Actual entry_count: %d\n",
               expected_entry_count,
               rack_priority_array[0].entry_count);
        printf("Expected residency: %d ms, Actual residency: %d ms\n",
               expected_residency_ms,
               rack_priority_array[0].residency_mS);
        printf("Finalize timestamp used for cycle 2: %u us\n", T_finalize_pkg_us);
        printf("Cycle 1 duration: %d ms, Cycle 2 duration (using finalize): %d ms\n",
               (int)((T3_us - T1_us) / 1000),
               (int)((T_finalize_pkg_us - T4_us) / 1000));
    }

    // Verify results from package output
    assert_int_equal(expected_entry_count, rack_priority_array[0].entry_count);
    assert_int_equal(expected_residency_ms, rack_priority_array[0].residency_mS);

    if (g_print_logs)
    {
        printf("--- END test_single_priority_with_finalize ---\n");
        printf("***\n");
    }
}

// Test multiple priorities with overlapping cycles - validates residency calculation for different priority ending scenarios
// This test confirms proper residency calculation when priorities have different ending patterns (END vs finalize)
//
// Test Flow Summary:
// ==================
// T0: NO_THROTTLING (baseline timestamp reference)
// T1: RACK_THROTTLING_START (priority 1) - Start priority 1
// T2: RACK_THROTTLING_START (priority 2) - Start priority 2, automatically ends priority 1
// T3: RACK_THROTTLING_END (priority 2) - Complete the cycle for priority 2
// T4_finalize_us: data_smpl_finalize_pwr_pkg_metrics() - finalizes (no active priorities, not used in calculations)
// --- FINAL CHECK: Sequential priorities with automatic priority switching ---
// Priority 1 residency: T2 - T1 (ended when priority 2 started)
// Priority 2 residency: T3 - T2 (ended with explicit END event)
TEST_FUNCTION(test_sequential_priorities_with_finalize, test_setup, test_teardown)
{
    const uint8_t core_index = 0;

    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_sequential_priorities_with_finalize ---\n");
        printf("=== SEQUENTIAL PRIORITIES TEST with END events ===\n");
        printf("Testing sequential priorities without overlaps\n");
    }

    // Reset state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_rack_throttle_priority = 0;
        for (uint8_t priority = 0; priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; priority++)
        {
            core_rt[core_id].rack_pri_res_timestamp_uS[priority] = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].entry_count = 0;
            computed_metrics_2_mins.cores[core_id].rack_priorities[priority].residency_uS = 0;
        }
    }

    // Timestamps for sequential priorities test (no overlaps and automatic switching of priority)
    uint64_t T0_us = 1000000;          // 1000ms: NO_THROTTLING baseline
    uint64_t T1_us = 2000000;          // 2000ms: Priority 1 START
    uint64_t T2_us = 5000000;          // 5000ms: Priority 2 START (automatically ends priority 1)
    uint64_t T3_us = 8000000;          // 8000ms: Priority 2 END (3000ms duration)
    uint64_t T4_finalize_us = 9000000; // 9000ms: Finalize (no active priorities, so not used)

    // Add mock setup for droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Set up gtimer frequency mock for all calls in this test
    setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Initial time_t0 value: %" PRIu32 " us\n", time_t0);

    // === STEP 0: NO_THROTTLING baseline ===
    pstate_telem_t pstate_baseline = {0};
    core_current_t current_baseline = {0};
    setup_mock_pstate_data_with_throttling(&pstate_baseline, core_index, 8, 15, NO_THROTTLING, 0, T0_us);
    setup_mock_current_data(&current_baseline, core_index, T0_us, 90, 70, 110, 85, 90, 8);

    if (g_print_logs)
        printf("T0: NO_THROTTLING baseline at %" PRIu64 " us\n", T0_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === STEP 1: START Priority 1 (index 0) ===
    pstate_telem_t pstate_pri1_start = {0};
    core_current_t current_pri1_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_pri1_start, core_index, 10, 20, RACK_THROTTLING_START, 0, T1_us);
    setup_mock_current_data(&current_pri1_start, core_index, T1_us, 100, 80, 120, 90, 100, 10);

    if (g_print_logs)
        printf("T1: RACK_THROTTLING_START (priority=1 -> index 0) at %" PRIu64 " us\n", T1_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Not ending Priority 1 as it will end automatically when Priority 2 starts

    // === STEP 2: START Priority 2 (index 1) - automatically ends priority 1 ===
    pstate_telem_t pstate_pri2_start = {0};
    core_current_t current_pri2_start = {0};
    setup_mock_pstate_data_with_throttling(&pstate_pri2_start, core_index, 12, 22, RACK_THROTTLING_START, 1, T2_us);
    setup_mock_current_data(&current_pri2_start, core_index, T2_us, 110, 90, 130, 95, 110, 12);

    if (g_print_logs)
        printf("T2: RACK_THROTTLING_START (priority=2 -> index 1) sequential at %" PRIu64 " us\n", T2_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Validate automatic priority switching behavior after processing priority 2 start
    if (g_print_logs)
        printf("Automatic priority switching: Priority 1 should now be ended by Priority 2 start\n");

    // === STEP 3: END Priority 2 (index 1) - Complete the cycle ===
    pstate_telem_t pstate_pri2_end = {0};
    core_current_t current_pri2_end = {0};
    setup_mock_pstate_data_with_throttling(&pstate_pri2_end, core_index, 12, 22, RACK_THROTTLING_END, 1, T3_us);
    setup_mock_current_data(&current_pri2_end, core_index, T3_us, 110, 90, 130, 95, 110, 12);

    if (g_print_logs)
        printf("T3: RACK_THROTTLING_END (priority=2 -> index 1) complete at %" PRIu64 " us\n", T3_us);
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // === FINALIZE: No active throttling at package boundary ===
    set_next_finalize_timestamp(T4_finalize_us);
    data_smpl_finalize_pwr_pkg_metrics();
    uint32_t T_finalize_pkg_us = time_t0; // Capture the finalize timestamp used

    if (g_print_logs)
        printf("T4: data_smpl_finalize_pwr_pkg_metrics() called (no active priorities) with finalize "
               "timestamp: %u us\n",
               T_finalize_pkg_us);

    // === Create rack priority package ===
    pwr_core_record_rack_priorities_t rack_priority_record = {{0}};
    package_create_pwr_core_rack_priority_record(&rack_priority_record);

    // Get results from rack priority package
    pwr_core_element_rack_priorities_t* rack_priority_array =
        &rack_priority_record.rack_priority_collection[core_index].rack_priority_element[0];

    // === Calculate expected values ===
    // Priority 1 (index 0) - Single cycle completed with END event:
    //   Cycle: T2 - T1 = 4000000μs - 2000000μs = 2000000μs = 2000ms
    uint32_t expected_residency_priority_1_ms = (T2_us - T1_us) / 1000; // Priority 1 ends when Priority 2 starts

    // Priority 2 (index 1) - Single cycle completed with END event:
    //   Cycle: T3 - T2 = 8000000μs - 5000000μs = 3000000μs = 3000ms
    uint32_t expected_residency_priority_2_ms = (T3_us - T2_us) / 1000; // 3000ms

    uint32_t expected_entry_count_1 = 1; // Single RACK_THROTTLING_START event for priority 1
    uint32_t expected_entry_count_2 = 1; // Single RACK_THROTTLING_START event for priority 2

    if (g_print_logs)
    {
        printf("\n=== RESULTS ===\n");
        printf("Priority 1 (index 0):\n");
        printf("  Expected entry_count: %d, Actual entry_count: %d\n",
               expected_entry_count_1,
               rack_priority_array[0].entry_count);
        printf("  Expected residency: %d ms, Actual residency: %d ms\n",
               expected_residency_priority_1_ms,
               rack_priority_array[0].residency_mS);

        printf("Priority 2 (index 1):\n");
        printf("  Expected entry_count: %d, Actual entry_count: %d\n",
               expected_entry_count_2,
               rack_priority_array[1].entry_count);
        printf("  Expected residency: %d ms, Actual residency: %d ms\n",
               expected_residency_priority_2_ms,
               rack_priority_array[1].residency_mS);

        printf("Finalize timestamp: %u us (not used for residency - all cycles ended with END events)\n", T_finalize_pkg_us);
        printf("Sequential execution: Priority 1 (T1->T2), then Priority 2 (T2->T3) - no overlaps\n");
    }

    // Verify results from package output
    assert_int_equal(expected_entry_count_1, rack_priority_array[0].entry_count);
    assert_int_equal(expected_residency_priority_1_ms, rack_priority_array[0].residency_mS);
    assert_int_equal(expected_entry_count_2, rack_priority_array[1].entry_count);
    assert_int_equal(expected_residency_priority_2_ms, rack_priority_array[1].residency_mS);

    if (g_print_logs)
    {
        printf("--- END test_sequential_priorities_with_finalize ---\n");
        printf("***\n");
    }
}
