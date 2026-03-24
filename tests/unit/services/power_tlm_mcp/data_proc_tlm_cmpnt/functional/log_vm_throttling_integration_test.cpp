//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file log_vm_throttling_integration_test.cpp
 *
 * Test MPAM throttling data collection and package creation
 *
 * Test Objectives:
 * ================
 * - Validate MPAM throttling data is correctly collected from sensor FIFO
 * - Verify core current data processing assigns cores to correct MPAMs
 * - Confirm throttling state detection for MPAMs
 * - Validate package creation for MPAM throttle records
 *
 * Test Flow:
 * ==========
 * 1. Create dataset of core current inputs with MPAM IDs
 * 2. Call data_smpl_process_core_current_sensor_fifo() to process sensor data
 * 3. Call data_smpl_finalize_pwr_pkg_metrics() to finalize metrics
 * 4. Call package_create_pwr_mpam_throttle_record() to create package
 * 5. Verify throttle records are correctly populated
 *
 * Test Functions:
 * ===============
 * 1. test_mpam_die0_only_cores: Test MPAM throttling with cores on die 0
 * 2. test_mpam_die1_only_cores: Assign cores only from die 1 by mocking and verifying output, Not doing it because die 1 doesn't track transition
 * 3. test_mpam_both_dies_cores: Assign cores from both dies, verify data is totalled correctly
 * 4. test_mpam_throttle_start_and_end: Cover complete throttle cycle including start and end
 * 5. test_mpam_throttle_in_progress: Cover throttle start, still in progress when package is generated and calculating duration based on finalize timestamp
 * 6. test_mpam_throttle_edge_cases: Test INVALID_PSTATE (0xFF) handling when throttling occurs before pstate packet arrival
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2889381

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
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <fpfw_status.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
extern uint32_t time_t0;
extern bool in_band_publishing_active;
extern int g_enable_mock_die_id;
}

// Global variable to control debug printing
static bool g_print_logs = true;

// Test MPAM IDs
#define TEST_MPAM_ID_1 (5)  // MPAM 5
#define TEST_MPAM_ID_2 (10) // MPAM 10

// Helper Functions

// Setup mock core current data with MPAM ID
static void setup_mock_core_current_with_mpam(core_current_t* mock_current_data, // Pointer to core_current_t structure to fill
                                              uint8_t core_index, // Which core (0-67)
                                              uint8_t mpam_id, // Which MPAM/VM this core belongs to (0-127)
                                              uint64_t timestamp, // Time in microseconds
                                              uint32_t avg_mA,    // Average current in milliamps
                                              uint32_t min_mA,    // Minimum current in milliamps
                                              uint32_t max_mA,    // Maximum current in milliamps
                                              uint32_t volt_mV,   // Voltage in millivolts
                                              uint32_t pwr_mW,    // Power in milliwatts
                                              uint32_t pstate)    // Performance state
{
    // Convert from common units to raw sensor FIFO values (This is inverse of firmware conversion)
    // In Firmware: current_mA = raw_value * 32.2F
    // So Inverse applied in below values to get raw values: raw_value = current_mA / 32.2F
    uint32_t raw_avg = (uint32_t)(avg_mA / 32.2F);
    uint32_t raw_min = (uint32_t)(min_mA / 32.2F);
    uint32_t raw_max = (uint32_t)(max_mA / 32.2F);

    // In Firmware: power_mW = raw_value * 32
    // So Inverse: raw_value = power_mW / 32
    uint32_t raw_pwr = pwr_mW / 32U;

    // Fill in the core current data with raw sensor values
    mock_current_data->timestamp = timestamp;
    mock_current_data->data.avg = raw_avg;
    mock_current_data->data.min = raw_min;
    mock_current_data->data.max = raw_max;
    mock_current_data->data.volt = volt_mV; // Voltage is not converted, just pass through
    mock_current_data->data.pwr = raw_pwr;
    mock_current_data->data.pstate = pstate;
    mock_current_data->data.mpam_id_low = mpam_id; // Core's MPAM ID
    mock_current_data->data.mpam_id_high = 0;
    mock_current_data->data.cstate = 0; // C0 = active
    mock_current_data->data.reserved_45 = 0;
    mock_current_data->data.realm = 0;
    mock_current_data->data.change = 0;

    // Setup CMocka mocks for sensor FIFO polling
    will_return(__wrap_sensor_fifo_svc_poll_core_current, core_index);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, true);  // data is valid
    will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // no more entries
    will_return(__wrap_sensor_fifo_svc_poll_core_current, mock_current_data);

    if (g_print_logs)
    {
        printf("  Core %d: MPAM=%d, power=%d mW, pstate=%d\n", core_index, mpam_id, pwr_mW, pstate);
    }
}

//  Set the timestamp for the next finalize call
//  Adjusts time_t0 so that the next call to data_smpl_finalize_pwr_pkg_metrics()
//  will use the specified timestamp for residency calculations.
//  @param desired_timestamp Target timestamp in microseconds
static void set_next_finalize_timestamp(uint64_t desired_timestamp)
{
    time_t0 = desired_timestamp - MOCK_TIMESTAMP_INCREMENT;
}

// Setup function - called before each test
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);

    // Reset all telemetry data to clean state
    reset_pwr_tlm_data();

    // Initialize as primary die (die 0)
    die_2_die_exch_init(PRIMARY_DIE_ID);

    // Initialize all cores to inactive state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_is_active[core_id] = false;
        core_rt[core_id].latest_mpam_id = 0;
        core_rt[core_id].latest_power_mW = 0;
        core_rt[core_id].latest_pstate = 0;
        core_rt[core_id].status_flags.throttle_is_active = false;
    }

    // Initialize all MPAMs to inactive state
    for (uint8_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        mpam_staging[mpam_id].active = false;
        mpam_staging[mpam_id].throttling = false;
        mpam_staging[mpam_id].latest_total_pwr_mW = 0;
        mpam_staging[mpam_id].latest_pstate = 0;

        mpam_rt[mpam_id].status_flags.active = false;
        mpam_rt[mpam_id].status_flags.throttling = false;
        mpam_rt[mpam_id].residency_timestamp_uS = 0;
        mpam_rt[mpam_id].nominal_pstate = 20; // pstate ID (0-31), not frequency
    }

    // Initialize computed metrics
    comp_metrics_reset_local_2_min_metrics();

    // Initialize mock timestamp
    time_t0 = 100;

    // Enable telemetry publishing
    in_band_publishing_active = true;

    // Enable die ID mocking
    g_enable_mock_die_id = 1;

    return 0;
}

// Teardown function called after each test
static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);

    // Reset all telemetry data
    reset_pwr_tlm_data();

    // Disable die ID mocking
    g_enable_mock_die_id = 0;

    return 0;
}

// Test: MPAM throttling data collection and package creation
// ============================================================
// This test validates the complete flow of MPAM throttling data:
// Step 1: Create dataset of core current inputs with MPAM IDs
// Step 2: Process sensor FIFO data (data_smpl_process_core_current_sensor_fifo)
// Step 3: Finalize metrics (data_smpl_finalize_pwr_pkg_metrics)
// Step 4: Create package (package_create_pwr_mpam_throttle_record)
// Step 5: Verify throttle records are correctly populated
//
// Test Data:
// - MPAM 5: 3 cores (0, 1, 2) with throttling active
// - MPAM 10: 2 cores (3, 4) with no throttling
//
TEST_FUNCTION(test_mpam_die0_only_cores, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mpam_die0_only_cores ---\n");
    }

    // Mock die ID to return 0 (primary die / die 0) - called multiple times during processing
    // Using will_return_always since die ID is checked in many places throughout the test flow
    will_return_always(__wrap_die_2_die_exch_get_this_die_id, 0);

    // Mock droop counts (required by comp_metrics_for_cores_droop_counts() called during data_smpl_finalize_pwr_pkg_metrics())
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Define timestamps in microseconds
    // T1_start_us: Used for core data timestamps in the sensor FIFO packets
    // T2_finalize_pkg_us: Target finalize time for residency calculation
    // Note: The MPAM residency timestamp is set automatically by data_smpl_calculate_mpam_data_from_cores()
    // when it detects throttling transition. It uses exec_tlm_cmpnt_get_timestamp_microseconds() which
    // returns time_t0. Since time_t0 starts at 100 (see test_setup()) and increments by MOCK_TIMESTAMP_INCREMENT (1000)
    // for each call, when the first throttling core (core 0) is processed, time_t0 will be at 1100.
    uint64_t T1_start_us = 1000000;        // Start time for core data: 1 second
    uint64_t T2_finalize_pkg_us = 6001100; // Choosing like this to avoid taking care of rounding for assertion

    // Calculate expected MPAM start timestamp based on mock behavior
    // Initial time_t0 = 100 (see test_setup()), incremented to 1100 when first core is processed
    uint32_t expected_mpam5_start_us = 100 + MOCK_TIMESTAMP_INCREMENT; // 100 + 1000 = 1100

    // STEP 1: Create dataset of core current inputs
    if (g_print_logs)
        printf("\n=== STEP 1: Creating core current dataset ===\n");

    // Declare core current structures (one per core)
    core_current_t current_core0 = {0};
    core_current_t current_core1 = {0};
    core_current_t current_core2 = {0};
    core_current_t current_core3 = {0};
    core_current_t current_core4 = {0};

    // STEP 2: Process core current sensor FIFO data
    if (g_print_logs)
        printf("\n=== STEP 2: Processing sensor FIFO data ===\n");

    // Mark cores 0, 1, 2 as active and throttling for MPAM 5
    core_is_active[0] = true;
    core_is_active[1] = true;
    core_is_active[2] = true;
    core_rt[0].status_flags.throttle_is_active = true;
    core_rt[1].status_flags.throttle_is_active = true;
    core_rt[2].status_flags.throttle_is_active = true;

    // Mark cores 3, 4 as active (but not throttling) for MPAM 10
    core_is_active[3] = true;
    core_is_active[4] = true;

    // Process 3 cores for MPAM 5 (with throttling)
    // The first core processed will trigger MPAM throttling detection and set residency_timestamp_uS automatically
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 500, 15);
    data_smpl_process_core_current_sensor_fifo();

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 110, 90, 130, 1000, 600, 15);
    data_smpl_process_core_current_sensor_fifo();

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 120, 100, 140, 1000, 700, 15);
    data_smpl_process_core_current_sensor_fifo();

    // Note: MPAM 5 throttling state (active=true, throttling=true) is automatically set by
    // data_smpl_calculate_mpam_data_from_cores() which ORs the throttle_is_active flags from all cores.
    // The residency_timestamp_uS is also set automatically when throttling transition is detected.

    // Process 2 cores for MPAM 10 (no throttling)
    // Note: core_rt[3/4].status_flags.throttle_is_active is default set to false in test_setup
    // so MPAM 10 throttling state will be automatically calculated as false
    setup_mock_core_current_with_mpam(&current_core3, 3, TEST_MPAM_ID_2, T1_start_us, 130, 110, 150, 1000, 800, 18);
    data_smpl_process_core_current_sensor_fifo();

    setup_mock_core_current_with_mpam(&current_core4, 4, TEST_MPAM_ID_2, T1_start_us, 140, 120, 160, 1000, 900, 18);
    data_smpl_process_core_current_sensor_fifo();

    // STEP 3: Finalize package metrics
    if (g_print_logs)
        printf("\n=== STEP 3: Finalizing metrics ===\n");

    // Set timestamp for finalization
    set_next_finalize_timestamp(T2_finalize_pkg_us);

    // Finalize the metrics (computes residencies, averages, etc.)
    data_smpl_finalize_pwr_pkg_metrics();

    // Capture the actual finalize timestamp used (time_t0 after finalize increments it)
    uint32_t T_finalize_pkg_us = time_t0;

    if (g_print_logs)
    {
        printf("Finalize timestamp used: %u us\n", T_finalize_pkg_us);
    }

    // STEP 4: Create MPAM throttle package and verify
    if (g_print_logs)
        printf("\n=== STEP 4: Creating throttle package ===\n");

    // Create the throttle record package
    pwr_soc_record_mpam_throttle_t mpam_throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&mpam_throttle_record);

    // Get pointers to MPAM elements
    pwr_soc_element_mpam_throttle_t* mpam5_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_1].mpam_throttle_element;
    pwr_soc_element_mpam_throttle_t* mpam10_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_2].mpam_throttle_element;

    // Expected values for MPAM 5 (throttling active)
    // Note: The residency timestamp was set automatically when the first throttling core was processed.
    // The finalize timestamp is captured as T_finalize_pkg_us after finalize completes.
    // Duration calculation: ROUND_USEC_TO_MSEC(6001100 - 1100) = (6000000 + 500) / 1000 = 6000 ms
    uint8_t expected_mpam5_id = TEST_MPAM_ID_1;
    uint32_t expected_mpam5_duration_mS = ROUND_USEC_TO_MSEC(T_finalize_pkg_us - expected_mpam5_start_us);

    // Expected values for MPAM 10 (no throttling)
    uint8_t expected_mpam10_id = TEST_MPAM_ID_2;
    uint32_t expected_mpam10_duration_mS = 0; // As mpam10 process detects no throttling.

    // Note: The package also includes pstate frequency fields (nominal_pstate_frequency_Mhz, max_pstate_frequency_Mhz,
    // avg_pstate_frequency_Mhz, throttle_extent_centipct) which are populated from pstate tracking during core current
    // processing. Since this test focuses on validating MPAM throttling data from core current sensors, we print these
    // fields for visibility but do not assert them. Pstate frequency validation should be covered by separate pstate tests.

    // throttle_extent_centipct is only printed as per actual and not asserted as expected calculations will depend on DVFS table.

    // Print actual vs expected values
    if (g_print_logs)
    {
        printf("\n=== Verification Results ===\n");
        printf("MPAM 5 (throttling):\n");
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam5_element->mpam_id, expected_mpam5_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam5_element->throttle_duration_mS, expected_mpam5_duration_mS);
        printf("  throttle_extent_centipct: %u\n", mpam5_element->throttle_extent_centipct);
        printf("\nMPAM 10 (no throttling):\n");
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam10_element->mpam_id, expected_mpam10_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam10_element->throttle_duration_mS, expected_mpam10_duration_mS);
        printf("\nRecord size: actual=%u, expected=%zu\n", record_size, sizeof(pwr_soc_record_mpam_throttle_t));
    }

    // Verify MPAM 5 (throttling)
    assert_int_equal(expected_mpam5_id, mpam5_element->mpam_id);
    assert_int_equal(expected_mpam5_duration_mS, mpam5_element->throttle_duration_mS);

    // Verify MPAM 10 (no throttling) - only mpam_id and duration are asserted
    assert_int_equal(expected_mpam10_id, mpam10_element->mpam_id);
    assert_int_equal(expected_mpam10_duration_mS, mpam10_element->throttle_duration_mS);

    // Verify record size
    assert_int_equal(sizeof(pwr_soc_record_mpam_throttle_t), record_size);

    if (g_print_logs)
    {
        printf("--- END test_mpam_die0_only_cores ---\n");
        printf("***\n");
    }
}

// Test: MPAM throttling data collection from both dies
// ======================================================
// This test validates cross-die MPAM throttling aggregation where cores from both dies
// belong to the same MPAM. The primary die reads die 1 data from the exchange and aggregates it.
//
// Test Flow:
// 1. Simulate die 1 processing: Process some cores on die 1, write to exchange
// 2. Simulate die 0 processing: Process some cores on die 0, read die 1 data, aggregate
// 3. Finalize and verify the aggregated throttling data
//
// Test Data:
// - MPAM 5: 2 cores on die 0 (cores 0,1) + 2 cores on die 1 (cores 2,3) with throttling
// - MPAM 10: 1 core on die 0 (core 4) + 1 core on die 1 (core 5) without throttling
//
TEST_FUNCTION(test_mpam_both_dies_cores, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mpam_both_dies_cores ---\n");
    }

    // Define timestamps
    uint64_t T1_start_us = 1000000;
    uint64_t T2_finalize_pkg_us = 6004100; // Chosen to give exact ms result: (6004100 - 4100) = 6000000 us = 6000 ms

    // ========== STEP 1: Create dataset (die 1 processing) ==========
    if (g_print_logs)
        printf("\n=== STEP 1: Creating dataset (die 1 processing) ===\n");

    // Mock die ID to return 1 for die 1 processing
    // data_smpl_calculate_mpam_data_from_cores() calls die_2_die_exch_get_this_die_id() 2 times per core:
    //   1. Once check at function entry
    //   2. Once while checking for primary/secondary die logic
    // Processing 3 cores: 3 cores × 2 calls/core = 6 total calls
    will_return_count(__wrap_die_2_die_exch_get_this_die_id, 1, 6);

    // Reset time_t0 for die 1 processing
    time_t0 = 100;

    // Declare core current structures for die 1
    core_current_t die1_current_core2 = {0};
    core_current_t die1_current_core3 = {0};
    core_current_t die1_current_core5 = {0};

    // Mark cores 2, 3 as active and throttling for MPAM 5 on die 1
    core_is_active[2] = true;
    core_is_active[3] = true;
    core_rt[2].status_flags.throttle_is_active = true;
    core_rt[3].status_flags.throttle_is_active = true;

    // Mark core 5 as active (no throttling) for MPAM 10 on die 1
    core_is_active[5] = true;

    // Process die 1 cores for MPAM 5 (with throttling)
    setup_mock_core_current_with_mpam(&die1_current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 120, 100, 140, 1000, 700, 15);
    data_smpl_process_core_current_sensor_fifo();

    setup_mock_core_current_with_mpam(&die1_current_core3, 3, TEST_MPAM_ID_1, T1_start_us, 130, 110, 150, 1000, 800, 15);
    data_smpl_process_core_current_sensor_fifo();

    // Process die 1 core for MPAM 10 (no throttling)
    setup_mock_core_current_with_mpam(&die1_current_core5, 5, TEST_MPAM_ID_2, T1_start_us, 150, 130, 170, 1000, 1000, 18);
    data_smpl_process_core_current_sensor_fifo();

    // Die 1 has accumulated its core data in mpam_staging
    // For testing purpose keeping this data in mpam_staging, this is simulating the exchange behaviour
    // In real firmware: die 1 writes to exchange, die 0 reads and aggregates
    bool die1_mpam5_was_throttling = mpam_staging[TEST_MPAM_ID_1].throttling;
    bool die1_mpam10_was_throttling = mpam_staging[TEST_MPAM_ID_2].throttling;
    uint16_t die1_mpam5_power = mpam_staging[TEST_MPAM_ID_1].latest_total_pwr_mW;
    uint16_t die1_mpam10_power = mpam_staging[TEST_MPAM_ID_2].latest_total_pwr_mW;

    if (g_print_logs)
    {
        printf("Die 1 MPAM 5: throttling=%d, power=%d mW\n", die1_mpam5_was_throttling, die1_mpam5_power);
        printf("Die 1 MPAM 10: throttling=%d, power=%d mW\n", die1_mpam10_was_throttling, die1_mpam10_power);
    }

    // ========== STEP 2: Process sensor data (die 0 processing and aggregation) ==========
    if (g_print_logs)
        printf("\n=== STEP 2: Processing sensor data (die 0 processing and aggregation) ===\n");

    // Note: We don't reset mpam_staging here because we want die 1's data to remain
    // This simulates the exchange read that would normally populate mpam_staging
    comp_metrics_reset_local_2_min_metrics();

    // Mock die ID to return 0 for die 0 processing
    // Using will_return_always since die ID is checked in many places throughout data processing
    will_return_always(__wrap_die_2_die_exch_get_this_die_id, 0);

    // Mock droop counts for finalize
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Declare core current structures for die 0
    core_current_t die0_current_core0 = {0};
    core_current_t die0_current_core1 = {0};
    core_current_t die0_current_core4 = {0};

    // Mark cores 0, 1 as active and throttling for MPAM 5 on die 0
    core_is_active[0] = true;
    core_is_active[1] = true;
    core_rt[0].status_flags.throttle_is_active = true;
    core_rt[1].status_flags.throttle_is_active = true;

    // Mark core 4 as active (no throttling) for MPAM 10 on die 0
    core_is_active[4] = true;

    // Process die 0 cores for MPAM 5 (with throttling)
    setup_mock_core_current_with_mpam(&die0_current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 500, 15);
    data_smpl_process_core_current_sensor_fifo();

    setup_mock_core_current_with_mpam(&die0_current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 110, 90, 130, 1000, 600, 15);
    data_smpl_process_core_current_sensor_fifo();

    // Process die 0 core for MPAM 10 (no throttling)
    setup_mock_core_current_with_mpam(&die0_current_core4, 4, TEST_MPAM_ID_2, T1_start_us, 140, 120, 160, 1000, 900, 18);
    data_smpl_process_core_current_sensor_fifo();

    // At this point, die 0 has:
    // - Read die 1 data from exchange into mpam_staging
    // - Added die 0 core data to mpam_staging
    // - Called comp_metrics_for_mpam_data() to track throttling

    // ========== STEP 3: Finalize metrics ==========
    if (g_print_logs)
        printf("\n=== STEP 3: Finalizing metrics ===\n");

    // Set timestamp for finalization
    set_next_finalize_timestamp(T2_finalize_pkg_us);

    // Finalize the metrics
    data_smpl_finalize_pwr_pkg_metrics();

    // Capture finalize timestamp
    uint32_t T_finalize_pkg_us = time_t0;

    if (g_print_logs)
    {
        printf("Finalize timestamp used: %u us\n", T_finalize_pkg_us);
        printf("MPAM 5 start timestamp from mpam_rt: %llu us\n", mpam_rt[TEST_MPAM_ID_1].residency_timestamp_uS);
        printf("MPAM 5 residency_uS from computed_metrics: %u us\n",
               computed_metrics_2_mins.mpam[TEST_MPAM_ID_1].residency_uS);
    }

    // ========== STEP 4: Create package ==========
    if (g_print_logs)
        printf("\n=== STEP 4: Creating package ===\n");

    // Create the throttle record package
    pwr_soc_record_mpam_throttle_t mpam_throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&mpam_throttle_record);

    // Get pointers to MPAM elements
    pwr_soc_element_mpam_throttle_t* mpam5_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_1].mpam_throttle_element;
    pwr_soc_element_mpam_throttle_t* mpam10_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_2].mpam_throttle_element;

    // ========== STEP 5: Verify throttle records ==========

    // Expected values - MPAM 5 aggregated from both dies (4 cores total with throttling)
    // Throttling starts when first die 0 core is processed: time_t0 = 4100 us
    // Finalize timestamp: T2_finalize_pkg_us = 6004100 us
    // Duration: (6004100 - 4100) = 6000000 us = 6000 ms
    uint8_t expected_mpam5_id = TEST_MPAM_ID_1;
    uint32_t expected_mpam5_start_us = 100 + (4 * MOCK_TIMESTAMP_INCREMENT); // 100 + 4000 = 4100 us
    uint32_t expected_mpam5_residency_us = T2_finalize_pkg_us - expected_mpam5_start_us; // 6004100 - 4100 = 6000000 us
    uint32_t expected_mpam5_duration_mS = ROUND_USEC_TO_MSEC(expected_mpam5_residency_us); // 6000 ms

    // Expected values - MPAM 10 aggregated from both dies (2 cores total, no throttling)
    uint8_t expected_mpam10_id = TEST_MPAM_ID_2;
    uint32_t expected_mpam10_duration_mS = 0;

    if (g_print_logs)
    {
        printf("Expected calculation: (%u - %u) = %u us → %u ms\n",
               T_finalize_pkg_us,
               expected_mpam5_start_us,
               expected_mpam5_residency_us,
               expected_mpam5_duration_mS);
    }

    // Print results
    if (g_print_logs)
    {
        printf("\n=== Verification Results (both dies aggregated) ===\n");
        printf("MPAM 5 (throttling - 4 cores across both dies):\n");
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam5_element->mpam_id, expected_mpam5_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam5_element->throttle_duration_mS, expected_mpam5_duration_mS);
        printf("  throttle_extent_centipct: %u\n", mpam5_element->throttle_extent_centipct);
        printf("\nMPAM 10 (no throttling - 2 cores across both dies):\n");
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam10_element->mpam_id, expected_mpam10_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam10_element->throttle_duration_mS, expected_mpam10_duration_mS);
        printf("\nRecord size: actual=%u, expected=%zu\n", record_size, sizeof(pwr_soc_record_mpam_throttle_t));
    }

    // Verify aggregated MPAM 5 (throttling from both dies)
    assert_int_equal(expected_mpam5_id, mpam5_element->mpam_id);
    assert_int_equal(expected_mpam5_duration_mS, mpam5_element->throttle_duration_mS);

    // Verify aggregated MPAM 10 (no throttling from both dies)
    assert_int_equal(expected_mpam10_id, mpam10_element->mpam_id);
    assert_int_equal(expected_mpam10_duration_mS, mpam10_element->throttle_duration_mS);

    // Verify record size
    assert_int_equal(sizeof(pwr_soc_record_mpam_throttle_t), record_size);

    if (g_print_logs)
    {
        printf("--- END test_mpam_both_dies_cores ---\n");
        printf("***\n");
    }
}

// Test: MPAM throttling complete cycle (start and end)
// ======================================================
// This test validates a complete throttle cycle where throttling starts, persists,
// and then ends before package finalization.
//
// Test Flow:
// 1. Process cores with throttling active - establishes throttling start
// 2. Process cores with throttling inactive - establishes throttling end
// 3. Finalize and verify throttle duration = end_time - start_time
//
// Test Data:
// - MPAM 5: Cores 0,1,2 start throttling, then stop throttling
// - Timeline: Start throttling → Continue → Stop throttling → Finalize
//
TEST_FUNCTION(test_mpam_throttle_start_and_end, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mpam_throttle_start_and_end ---\n");
    }

    // Mock die ID to return 0 (primary die)
    // data_smpl_calculate_mpam_data_from_cores() calls die_2_die_exch_get_this_die_id() 2 times per core
    // The function is called extensively throughout the data processing pipeline.
    // Use will_return_always to handle all calls without counting.
    will_return_always(__wrap_die_2_die_exch_get_this_die_id, 0);
    // Mock droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Define timestamps
    uint64_t T1_start_us = 1000000;

    // Declare core current structures
    core_current_t current_core0 = {0};
    core_current_t current_core1 = {0};
    core_current_t current_core2 = {0};

    // Note: The following phases are part of STEP 2 (Process sensor data)
    // ========== Phase 1: Start throttling ==========
    if (g_print_logs)
        printf("\n=== Phase 1: Starting throttling ===\n");

    // Reset time for throttling start
    time_t0 = 100;

    // Mark cores as active and throttling
    core_is_active[0] = true;
    core_is_active[1] = true;
    core_is_active[2] = true;
    core_rt[0].status_flags.throttle_is_active = true;
    core_rt[1].status_flags.throttle_is_active = true;
    core_rt[2].status_flags.throttle_is_active = true;

    // Process cores with throttling active
    // First core will trigger transition: !was_throttling → is_throttling
    // This sets mpam_rt[5].residency_timestamp_uS to current time_t0
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 500, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 1100, mpam_rt[5].residency_timestamp_uS = 1100

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 110, 90, 130, 1000, 600, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 2100

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 120, 100, 140, 1000, 700, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 3100

    uint64_t throttle_start_time = mpam_rt[TEST_MPAM_ID_1].residency_timestamp_uS;

    if (g_print_logs)
    {
        printf("Throttle start timestamp: %llu us\n", throttle_start_time);
        printf("Current time_t0: %u us\n", time_t0);
    }

    // ========== STEP 2 continued: Process sensor data (Phase 2: Continue throttling) ==========
    if (g_print_logs)
        printf("\n=== STEP 2 continued: Processing sensor data - Phase 2: Continuing throttling ===\n");

    // Process cores again with throttling still active
    // This accumulates residency time
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 105, 85, 125, 1000, 520, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 4100

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 115, 95, 135, 1000, 620, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 5100

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 125, 105, 145, 1000, 720, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 6100

    if (g_print_logs)
    {
        printf("After continuing throttling, time_t0: %u us\n", time_t0);
    }

    // ========== STEP 2 continued: Process sensor data (Phase 3: End throttling) ==========
    if (g_print_logs)
        printf("\n=== STEP 2 continued: Processing sensor data - Phase 3: Ending throttling ===\n");

    // Now turn off throttling for all cores
    // This will trigger transition: was_throttling → !is_throttling
    core_rt[0].status_flags.throttle_is_active = false;
    core_rt[1].status_flags.throttle_is_active = false;
    core_rt[2].status_flags.throttle_is_active = false;

    // Process cores with throttling inactive
    // First core will trigger end transition and accumulate final residency
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 90, 70, 110, 1000, 450, 20);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 7100
    // Transition detected: was_throttling → !is_throttling
    // Residency accumulated: (7100 - 1100) = 6000 us added to computed_metrics

    uint64_t throttle_end_time = time_t0; // Capture the end time

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 95, 75, 115, 1000, 470, 20);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 8100

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 490, 20);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 9100

    if (g_print_logs)
    {
        printf("Throttle end timestamp: %llu us\n", throttle_end_time);
        printf("After ending throttling, time_t0: %u us\n", time_t0);
        printf("Accumulated residency so far: %u us\n", computed_metrics_2_mins.mpam[TEST_MPAM_ID_1].residency_uS);
    }

    // ========== STEP 3: Finalize metrics (after throttling has ended) ==========
    if (g_print_logs)
        printf("\n=== STEP 3: Finalizing metrics ===\n");

    // Set finalize timestamp to some time after throttling ended
    uint64_t T_finalize_pkg_us = 15001100;
    set_next_finalize_timestamp(T_finalize_pkg_us);

    // Finalize the metrics
    data_smpl_finalize_pwr_pkg_metrics();

    // Capture actual finalize timestamp
    uint32_t actual_finalize_time = time_t0;

    if (g_print_logs)
    {
        printf("Finalize timestamp: %u us\n", actual_finalize_time);
        printf("Final accumulated residency: %u us\n", computed_metrics_2_mins.mpam[TEST_MPAM_ID_1].residency_uS);
    }

    // ========== STEP 4: Create package ==========
    if (g_print_logs)
        printf("\n=== STEP 4: Creating package ===\n");

    // Create the throttle record package
    pwr_soc_record_mpam_throttle_t mpam_throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&mpam_throttle_record);

    // Get pointer to MPAM 5 element
    pwr_soc_element_mpam_throttle_t* mpam5_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_1].mpam_throttle_element;

    // ========== STEP 5: Verify throttle records ==========

    // Expected values: Duration from throttle start to end (NOT to finalize time)
    // Throttle start: time_t0 = 1100 us (Phase 1, Core 0)
    // Throttle end:   time_t0 = 7100 us (Phase 3, Core 0)
    // Duration: (7100 - 1100) = 6000 us = 6 ms
    uint8_t expected_mpam5_id = TEST_MPAM_ID_1;
    uint32_t expected_mpam5_start_us = 100 + (1 * MOCK_TIMESTAMP_INCREMENT);                // 1100 us
    uint32_t expected_mpam5_end_us = 100 + (7 * MOCK_TIMESTAMP_INCREMENT);                  // 7100 us
    uint32_t expected_mpam5_residency_us = expected_mpam5_end_us - expected_mpam5_start_us; // 6000 us
    uint32_t expected_mpam5_duration_mS = ROUND_USEC_TO_MSEC(expected_mpam5_residency_us);  // 6 ms

    // Verify duration end_time - start_time, shouldn't consider time between throttle_end and finalize
    uint32_t time_from_start_to_end_ms = (throttle_end_time - throttle_start_time) / 1000;
    uint32_t time_from_start_to_finalize_ms = (actual_finalize_time - throttle_start_time) / 1000;

    // Print all actual vs expected values
    if (g_print_logs)
    {
        printf("\n=== Verification Results ===\n");
        printf("MPAM 5 (complete throttle cycle - start to end):\n");
        printf("  Throttle start time: %llu us\n", throttle_start_time);
        printf("  Throttle end time:   %llu us\n", throttle_end_time);
        printf("  Finalize time:       %u us\n", actual_finalize_time);
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam5_element->mpam_id, expected_mpam5_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam5_element->throttle_duration_mS, expected_mpam5_duration_mS);
        printf("  throttle_extent_centipct: %u\n", mpam5_element->throttle_extent_centipct);
        printf("\nTiming verification:\n");
        printf("  Time from start to end:      %u ms\n", time_from_start_to_end_ms);
        printf("  Time from start to finalize: %u ms\n", time_from_start_to_finalize_ms);
        printf("  Duration should match start-to-end, not start-to-finalize\n");
        printf("\nRecord size: actual=%u, expected=%zu\n", record_size, sizeof(pwr_soc_record_mpam_throttle_t));
    }

    // Assert all expected values
    assert_int_equal(expected_mpam5_id, mpam5_element->mpam_id);
    assert_int_equal(expected_mpam5_duration_mS, mpam5_element->throttle_duration_mS);
    assert_int_equal(sizeof(pwr_soc_record_mpam_throttle_t), record_size);

    // Verify duration is from start to end (within rounding tolerance), not to finalize
    assert_true(mpam5_element->throttle_duration_mS <= time_from_start_to_end_ms + 1);
    assert_true(mpam5_element->throttle_duration_mS < time_from_start_to_finalize_ms - 1000);

    if (g_print_logs)
    {
        printf("--- END test_mpam_throttle_start_and_end ---\n");
        printf("***\n");
    }
}

// Test: MPAM throttling in progress (active at finalize)
// ========================================================
// This test validates the scenario where throttling starts and is still active
// when the package is finalized.
//
// Test Flow:
// 1. Process cores with throttling active - establishes throttling start
// 2. Continue processing cores with throttling still active
// 3. Finalize while throttling is active and verify throttle duration = finalize_time - start_time
//
// Test Data:
// - MPAM 5: Cores 0,1,2 start throttling and remain throttling through finalize
// - Timeline: Start throttling → Continue → Finalize (throttle still active)
//
TEST_FUNCTION(test_mpam_throttle_in_progress, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mpam_throttle_in_progress ---\n");
    }

    // Mock die ID to return 0 (primary die)
    // data_smpl_calculate_mpam_data_from_cores() calls die_2_die_exch_get_this_die_id() 2 times per core
    // The function is called extensively throughout the data processing pipeline.
    // Use will_return_always to handle all calls without counting.
    will_return_always(__wrap_die_2_die_exch_get_this_die_id, 0);
    // Mock droop counts
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    // Define timestamps
    uint64_t T1_start_us = 1000000;
    uint64_t T2_finalize_pkg_us = 10005100; // Chosen to give exact ms result: (10005100 - 1100) = 10004000 us = 10004 ms

    // Declare core current structures
    core_current_t current_core0 = {0};
    core_current_t current_core1 = {0};
    core_current_t current_core2 = {0};

    // ========== PHASE 1: Start throttling ==========
    if (g_print_logs)
        printf("\n=== Phase 1: Starting throttling ===\n");

    // Reset time for throttling start
    time_t0 = 100;

    // Mark cores as active and throttling
    core_is_active[0] = true;
    core_is_active[1] = true;
    core_is_active[2] = true;
    core_rt[0].status_flags.throttle_is_active = true;
    core_rt[1].status_flags.throttle_is_active = true;
    core_rt[2].status_flags.throttle_is_active = true;

    // Process cores with throttling active
    // First core will trigger transition: !was_throttling → is_throttling
    // This sets mpam_rt[5].residency_timestamp_uS to current time_t0
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 500, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 1100, mpam_rt[5].residency_timestamp_uS = 1100

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 110, 90, 130, 1000, 600, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 2100

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 120, 100, 140, 1000, 700, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 3100

    uint64_t throttle_start_time = mpam_rt[TEST_MPAM_ID_1].residency_timestamp_uS;

    if (g_print_logs)
    {
        printf("Throttle start timestamp: %llu us\n", throttle_start_time);
        printf("Current time_t0: %u us\n", time_t0);
    }

    // ========== STEP 2 continued: Process sensor data (Phase 2: Continue throttling, remains active) ==========
    if (g_print_logs)
        printf("\n=== STEP 2 continued: Processing sensor data - Phase 2: Continuing throttling ===\n");

    // Process cores again with throttling STILL active
    // Throttling never ends - it remains active through finalization
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 105, 85, 125, 1000, 520, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 4100

    setup_mock_core_current_with_mpam(&current_core1, 1, TEST_MPAM_ID_1, T1_start_us, 115, 95, 135, 1000, 620, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 5100

    setup_mock_core_current_with_mpam(&current_core2, 2, TEST_MPAM_ID_1, T1_start_us, 125, 105, 145, 1000, 720, 10);
    data_smpl_process_core_current_sensor_fifo();
    // After this: time_t0 = 6100

    if (g_print_logs)
    {
        printf("After continuing throttling, time_t0: %u us\n", time_t0);
        printf("Throttling still active: %d\n", mpam_rt[TEST_MPAM_ID_1].status_flags.throttling);
    }

    // ========== STEP 3: Finalize metrics (throttling still in progress) ==========
    if (g_print_logs)
        printf("\n=== STEP 3: Finalizing metrics (throttling still active) ===\n");

    // Set finalize timestamp
    set_next_finalize_timestamp(T2_finalize_pkg_us);

    // Finalize the metrics while throttling is still active
    // This should calculate residency from start to finalize
    data_smpl_finalize_pwr_pkg_metrics();

    // Capture actual finalize timestamp
    uint32_t actual_finalize_time = time_t0;

    if (g_print_logs)
    {
        printf("Finalize timestamp: %u us\n", actual_finalize_time);
        printf("Throttling still active after finalize: %d\n", mpam_rt[TEST_MPAM_ID_1].status_flags.throttling);
        printf("Accumulated residency: %u us\n", computed_metrics_2_mins.mpam[TEST_MPAM_ID_1].residency_uS);
    }

    // ========== STEP 4: Create package ==========
    if (g_print_logs)
        printf("\n=== STEP 4: Creating package ===\n");

    // Create the throttle record package
    pwr_soc_record_mpam_throttle_t mpam_throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&mpam_throttle_record);

    // Get pointer to MPAM 5 element
    pwr_soc_element_mpam_throttle_t* mpam5_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_1].mpam_throttle_element;

    // ========== STEP 5: Verify throttle records ==========

    // Expected values: Duration from throttle start to finalize as throttle never ends
    // Throttle start: time_t0 = 1100 us (Phase 1, Core 0)
    // Finalize:       time_t0 = 10005100 us (throttle still active)
    // Duration: (10005100 - 1100) = 10004000 us = 10004 ms
    uint8_t expected_mpam5_id = TEST_MPAM_ID_1;
    uint32_t expected_mpam5_start_us = 100 + (1 * MOCK_TIMESTAMP_INCREMENT);               // 1100 us
    uint32_t expected_mpam5_residency_us = T2_finalize_pkg_us - expected_mpam5_start_us;   // 10004000 us
    uint32_t expected_mpam5_duration_mS = ROUND_USEC_TO_MSEC(expected_mpam5_residency_us); // 10004 ms

    // Print all actual vs expected values
    if (g_print_logs)
    {
        printf("\n=== Verification Results ===\n");
        printf("MPAM 5 (throttling in progress - start to finalize):\n");
        printf("  Throttle start time: %llu us\n", throttle_start_time);
        printf("  Finalize time:       %u us\n", actual_finalize_time);
        printf("  Throttling status:   STILL ACTIVE\n");
        printf("  mpam_id:              actual=%u, expected=%u\n", mpam5_element->mpam_id, expected_mpam5_id);
        printf("  throttle_duration_mS: actual=%u, expected=%u\n", mpam5_element->throttle_duration_mS, expected_mpam5_duration_mS);
        printf("  throttle_extent_centipct: %u\n", mpam5_element->throttle_extent_centipct);
        printf("\nExpected calculation: (%u - %u) = %u us → %u ms\n",
               actual_finalize_time,
               expected_mpam5_start_us,
               expected_mpam5_residency_us,
               expected_mpam5_duration_mS);
        printf("\nRecord size: actual=%u, expected=%zu\n", record_size, sizeof(pwr_soc_record_mpam_throttle_t));
    }

    // Assert all expected values
    assert_int_equal(expected_mpam5_id, mpam5_element->mpam_id);
    assert_int_equal(expected_mpam5_duration_mS, mpam5_element->throttle_duration_mS);
    assert_int_equal(sizeof(pwr_soc_record_mpam_throttle_t), record_size);

    // Verify throttling is still active
    assert_true(mpam_rt[TEST_MPAM_ID_1].status_flags.throttling);

    if (g_print_logs)
    {
        printf("--- END test_mpam_throttle_in_progress ---\n");
        printf("***\n");
    }
}

// Test: MPAM throttling edge case - INVALID_PSTATE (0xFF) handling
// =================================================================
// This test validates firmware behavior when throttling occurs before any pstate packet arrives.
//
// Scenario:
// ---------
// if throttling starts before any pstate packet arrives, nominal_pstate may be INVALID_PSTATE (0xFF).
// Note: valid pstate range: 0-31.
//
// Test Flow:
// ----------
// 1. Configure core with nominal_pstate = INVALID_PSTATE (0xFF), assuming pstate_frequency will be 0, but dvfs table should take care of it by sending a minimum value.
// 2. Process core current packet with throttling active
// 3. Finalize metrics
// 4. Create package and verify throttle data
//
// Verification:
// -------------
// - nominal_pstate_frequency_Mhz field is populated
// - Package creation completes successfully without
// - Throttle extent calculation handles it correctly without any crash
TEST_FUNCTION(test_mpam_throttle_edge_cases, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mpam_throttle_edge_cases ---\n");
        printf("Testing edge case: nominal_pstate = INVALID_PSTATE (0xFF)\n");
        printf("Scenario: Throttling starts before pstate packet arrives\n");
    }

    // Mock die ID for primary die processing
    // Using will_return_always since die ID is checked in many places throughout data processing
    will_return_always(__wrap_die_2_die_exch_get_this_die_id, 0);

    // Mock droop counts - needed for finalize call
    static uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE] = {0};
    will_return_count(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts, 1);

    // Define timestamps
    uint64_t T1_start_us = 1000000;
    uint64_t T2_finalize_pkg_us = 6001100;

    // Declare core current structure
    core_current_t current_core0 = {0};

    // ========== STEP 1 & 2: Create dataset and process sensor data ==========
    if (g_print_logs)
    {
        printf("\n=== STEP 1 & 2: Creating dataset and processing sensor data ===\n");
        printf("Scenario: Throttling starts with no pstate packet received\n");
        printf("Setting core nominal_pstate to INVALID_PSTATE (0xFF)\n");
    }

    // Reset time
    time_t0 = 100;

    // Set core nominal_pstate to INVALID_PSTATE (0xFF) to simulate no pstate packet received
    // This is the firmware's sentinel value for invalid/uninitialized pstate (defined in compute_metrics_i.h)
    core_rt[0].nominal_pstate = INVALID_PSTATE; // 0xFF

    // Mark core 0 as active and throttling for MPAM 5
    core_is_active[0] = true;
    core_rt[0].status_flags.throttle_is_active = true;

    // Process core with throttling active
    setup_mock_core_current_with_mpam(&current_core0, 0, TEST_MPAM_ID_1, T1_start_us, 100, 80, 120, 1000, 500, 15);
    data_smpl_process_core_current_sensor_fifo();

    if (g_print_logs)
    {
        printf("\nAfter processing current packet:\n");
        printf("  Core 0 nominal_pstate: 0x%02X (INVALID_PSTATE)\n", core_rt[0].nominal_pstate);
        printf("  MPAM %d nominal_pstate: 0x%02X (copied from core)\n",
               TEST_MPAM_ID_1,
               mpam_rt[TEST_MPAM_ID_1].nominal_pstate);
    }

    // ========== STEP 3: Finalize metrics ==========
    if (g_print_logs)
        printf("\n=== STEP 3: Finalizing metrics ===\n");

    set_next_finalize_timestamp(T2_finalize_pkg_us);
    data_smpl_finalize_pwr_pkg_metrics();

    // ========== STEP 4: Create package ==========
    if (g_print_logs)
        printf("\n=== STEP 4: Creating package ===\n");

    // Create package
    // dvfs_get_freq_from_plimit(0xFF) will be called to convert pstate to frequency
    pwr_soc_record_mpam_throttle_t mpam_throttle_record = {{0}};
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&mpam_throttle_record);

    pwr_soc_element_mpam_throttle_t* mpam5_element =
        &mpam_throttle_record.mpam_throttle_collection[TEST_MPAM_ID_1].mpam_throttle_element;

    // ========== STEP 5: Verify throttle records ==========
    if (g_print_logs)
    {
        printf("\n=== STEP 5: Verification Results ===\n");
        printf("Package created successfully\n");
        printf("\nPackage contents:\n");
        printf("  MPAM ID: %u\n", mpam5_element->mpam_id);
        printf("  nominal_pstate_frequency_Mhz: %u MHz\n", mpam5_element->nominal_pstate_frequency_Mhz);
        printf("  max_pstate_frequency_Mhz: %u MHz\n", mpam5_element->max_pstate_frequency_Mhz);
        printf("  throttle_extent_centipct: %u%%\n", mpam5_element->throttle_extent_centipct);
        printf("  throttle_duration_mS: %u ms\n", mpam5_element->throttle_duration_mS);
    }

    // Verify package was created
    assert_int_equal(sizeof(pwr_soc_record_mpam_throttle_t), record_size);
    assert_int_equal(TEST_MPAM_ID_1, mpam5_element->mpam_id);

    // Verify nominal_pstate_frequency_Mhz is populated
    assert_true(mpam5_element->nominal_pstate_frequency_Mhz > 0);

    if (g_print_logs)
    {
        printf("\n--- END test_mpam_throttle_edge_cases ---\n");
        printf("***\n");
    }
}
