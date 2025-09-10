//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_throttling_data.cpp
 * Test functionality and flow for power telemetry throttling data collection
 *
 * Test Flow:
 * ----------
 * test_core_current_throttling_functional()
 * Set up mock current data (power values)
 * Set up mock PSTATE data with throttling status
 * Mock sensor polling calls
 *  Call: data_proc_tlm_cmpnt_process_input_data()  (MAIN CALL)
 *  Call: package_create_pwr_core_throttle_record()
 *  Call: data_proc_tlm_cmpnt_get_pwr_core_throttle_data()
 * Verify results
 *
 * Test Functions:
 * ---------------
 * 1. test_core_current_throttling_functional: Tests current throttling start/end flow
 *
 * Notes:
 * - Throttling status comes from PSTATE telemetry packets
 * - Each throttling type has start/end events and residency tracking
 * - Entry count increments on throttling start events
 * - Residency accumulates during throttling periods but accumulated once the cycle is completed
 * - Multiple throttling types can be active simultaneously
 */

// @SSI_functional_Test
// @SSI_functional_Test:power_telemetry

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

#define NO_OF_ITERATIONS        (4)
#define NUMBER_OF_CORES_TO_TEST (4)
#define MAX_PSTATE_VALUE        (32)
#define RESIDENCY_TOLERANCE_MS  100

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
    int32_t avg;    // Average current
    int32_t min;    // Minimum current
    int32_t max;    // Maximum current
    int32_t volt;   // Voltage value in mV
    int32_t pwr;    // Power value
    int32_t pstate; // PSTATE from current packet
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
    // Set frequency to 1 MHz so 1 tick = 1 μs - only set up one call per processing
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
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

// Test single current throttling cycle: NO_THROTTLING → CURRENT_THROTTLING_START → CURRENT_THROTTLING_END
TEST_FUNCTION(test_core_current_throttling_functional, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_current_throttling_functional ---\n");
    }
    uint8_t core_index = 0;
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000;

    // Use microsecond values directly for timestamps
    pstate_telem_t mock_pstate_data_initial = {0};
    core_current_t mock_current_data = {0};

    // Step 1: NO_THROTTLING
    setup_mock_pstate_data_with_throttling(&mock_pstate_data_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&mock_current_data, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: CURRENT_THROTTLING_START
    pstate_telem_t mock_pstate_data_start = {0};
    core_current_t mock_current_data_start = {0};

    setup_mock_pstate_data_with_throttling(&mock_pstate_data_start, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&mock_current_data_start, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 3: CURRENT_THROTTLING_END
    pstate_telem_t mock_pstate_data_end = {0};
    core_current_t mock_current_data_end = {0};

    setup_mock_pstate_data_with_throttling(&mock_pstate_data_end, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&mock_current_data_end, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Processing step 3: CURRENT_THROTTLING_END\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Finalize metrics to update residency values
    data_smpl_finalize_pwr_pkg_metrics();

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array = &throttle_record.throttle_collection[core_index].throttle_element[0];

    // Expected values for current throttling (THROTTLE_SOURCE_CURRENT = 0)
    uint32_t expected_entry_count = 1; // One throttling start event
    uint32_t expected_residency_mS =
        (T3_us - T2_us) / 1000; // Throttling started at T2, ended at T3, so residency = T3-T2 = 1000ms
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
TEST_FUNCTION(test_core_multiple_current_throttling_cycles, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("***\n");
        printf("--- START test_core_multiple_current_throttling_cycles ---\n");
    }

    uint8_t core_index = 0;
    uint64_t T1_us = 1000000, T2_us = 2000000, T3_us = 3000000, T4_us = 4000000, T5_us = 5000000, T6_us = 6000000;

    // Step 1: Initial NO_THROTTLING state
    pstate_telem_t pstate_initial = {0};
    core_current_t current_initial = {0};
    setup_mock_pstate_data_with_throttling(&pstate_initial, core_index, 10, 20, NO_THROTTLING, 0, T1_us);
    setup_mock_current_data(&current_initial, core_index, T1_us, 100, 80, 120, 100, 100, 10);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 1: NO_THROTTLING\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 2: Throttling START #1
    pstate_telem_t pstate_start1 = {0};
    core_current_t current_start1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start1, core_index, 10, 20, CURRENT_THROTTLING_START, 0, T2_us);
    setup_mock_current_data(&current_start1, core_index, T2_us, 100, 80, 120, 100, 150, 10);
    setup_mock_gtimer_frequency();

    if (g_print_logs)
        printf("Processing step 2: CURRENT_THROTTLING_START #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 3: Throttling END #1
    pstate_telem_t pstate_end1 = {0};
    core_current_t current_end1 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end1, core_index, 11, 25, CURRENT_THROTTLING_END, 0, T3_us);
    setup_mock_current_data(&current_end1, core_index, T3_us, 100, 80, 120, 100, 200, 11);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 3: CURRENT_THROTTLING_END #1\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 4: Throttling START #2
    pstate_telem_t pstate_start2 = {0};
    core_current_t current_start2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start2, core_index, 12, 22, CURRENT_THROTTLING_START, 0, T4_us);
    setup_mock_current_data(&current_start2, core_index, T4_us, 110, 85, 125, 105, 210, 12);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 4: CURRENT_THROTTLING_START #2\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 5: Throttling END #2
    pstate_telem_t pstate_end2 = {0};
    core_current_t current_end2 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_end2, core_index, 13, 23, CURRENT_THROTTLING_END, 0, T5_us);
    setup_mock_current_data(&current_end2, core_index, T5_us, 115, 90, 130, 110, 220, 13);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 5: CURRENT_THROTTLING_END #2\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Step 6: Throttling START #3 (active, not ended)
    pstate_telem_t pstate_start3 = {0};
    core_current_t current_start3 = {0};
    setup_mock_pstate_data_with_throttling(&pstate_start3, core_index, 14, 24, CURRENT_THROTTLING_START, 0, T6_us);
    setup_mock_current_data(&current_start3, core_index, T6_us, 120, 95, 135, 115, 230, 14);
    setup_mock_gtimer_frequency();
    if (g_print_logs)
        printf("Processing step 6: CURRENT_THROTTLING_START #3 (remains active)\n");
    data_smpl_process_pstate_sensor_fifo();
    data_smpl_process_core_current_sensor_fifo();

    // Finalize metrics to update residency values
    data_smpl_finalize_pwr_pkg_metrics();

    // Get final results by creating record for the same
    pwr_core_record_throttle_t throttle_record = {{0}};
    package_create_pwr_core_throttle_record(&throttle_record);
    pwr_core_element_throttle_t* throttle_array = &throttle_record.throttle_collection[core_index].throttle_element[0];

    // Expected values for current throttling (THROTTLE_SOURCE_CURRENT = 0)
    uint32_t expected_entry_count = 3; // Because there are three throttling start events
    uint32_t expected_residency_mS = ((T3_us - T2_us) + (T5_us - T4_us)) / 1000; // Cycle1: T3-T2, Cycle2: T5-T4 = 2000ms total
    bool expected_throttling_status = true; // Throttling should be active because the 3rd cycle is still running
    bool expected_tracker_status = true;    // Tracker should be active for 3rd cycle

    if (g_print_logs)
    {
        printf("\n=== Results (Multiple Cycles) ===\n");
        printf("Current Throttling entry_count: %d (expected=%d)\n", throttle_array[0].entry_count, expected_entry_count);
        printf("Current Throttling residency: %d ms (expected=%d ms)\n", throttle_array[0].residency_mS, expected_residency_mS);
        printf("Current throttling status: %d (expected=%d)\n",
               core_rt[core_index].status_flags.throttle_is_active,
               expected_throttling_status);
        printf("Current throttling tracker: %d (expected=%d)\n",
               core_rt[core_index].throttle_source_tracker[0] > 0 ? 1 : 0,
               expected_tracker_status);
    }

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