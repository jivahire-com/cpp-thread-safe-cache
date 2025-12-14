//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_d2d_link_state_integration.cpp
 * Integration test for D2D Link State telemetry data collection and processing
 *
 * Objective:
 *   Validate D2D Link State telemetry collection, processing, and record creation.
 *   Test register-level mock interactions for D2DSS PMU counter APIs.
 *
 * Generic Test Flow:
 * ------------------
 * 1. Setup mock PMU counter read operations for D2DSS interfaces
 * 2. Configure L0/L0s/L1 link state counter values for residency and bandwidth
 * 3. Trigger data_proc_tlm_cmpnt_process_one_second_input_data() (twice for difference calculation)
 * 4. Create D2D link state record via package_create_pwr_soc_d2d_link_record()
 * 5. Verify/Assert results for D2D link state data conversion and packaging
 *
 * Test Functions:
 * ---------------
 * 1. test_d2d_link_state_basic_flow: Tests basic D2D link state telemetry flow
 *    - Validates L0, L0s, L1 residency time conversion (counters → milliseconds)
 *    - Verifies L0 bandwidth calculation (flit count → bytes)
 *    - Tests counter difference calculation between successive reads
 *    - Confirms proper record creation for interface 0 with expected values
 *
 * Notes:
 * ------
 * - D2D counters use difference calculation: second_read - first_read
 * - Conversion formula: milliseconds = (counter_difference / 2GHz) * 1000
 * - Bandwidth formula: bytes = flit_count * 64 (bytes per flit)
 * - Mock setup order must match actual PMU read order: RX first, then TX
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779237

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <data_utilities_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// Global variable to control debug printing
static bool g_print_debug = true;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0, false, 0, true); // die_id=0, is_single_die=false, mpam_vm_mem_fixed_pwr_mW=0, all_zero_filtering_enable=true
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// Helper function to setup D2D PMU counter mocks for a specific interface and link state
static void setup_d2d_pmu_mocks_for_interface(uint8_t interface_id,
                                              uint64_t l0_tx_res,
                                              uint64_t l0_rx_res,
                                              uint64_t l0_bw_tx,
                                              uint64_t l0_bw_rx,
                                              uint64_t l0s_tx_res,
                                              uint64_t l0s_rx_res,
                                              uint64_t l1_tx_res,
                                              uint64_t l1_rx_res)
{
    if (g_print_debug)
    {
        // Only print debug info for interface 0 (actual test interface) to reduce noise
        // Other interfaces have zero values and are just for completeness
        if (interface_id == 0)
        {
            printf("DEBUG: Setting up PMU mocks for interface %d\n", interface_id);
            printf("  L0: tx_res=%llu, rx_res=%llu, bw_tx=%llu, bw_rx=%llu\n", l0_tx_res, l0_rx_res, l0_bw_tx, l0_bw_rx);
            printf("  L0s: tx_res=%llu, rx_res=%llu\n", l0s_tx_res, l0s_rx_res);
            printf("  L1: tx_res=%llu, rx_res=%llu\n", l1_tx_res, l1_rx_res);
        }
        else if (interface_id == 1)
        {
            printf("DEBUG: Setting up PMU mocks for interfaces 1-7 (all zeros)\n");
        }
    }

    // Mock L0 state counters (4 counters: rx_res, tx_res, bw_tx, bw_rx)
    // IMPORTANT: The actual read order in data_smpl_read_d2dss_l0_state is RX first, then TX
    // Each counter requires 2 calls (low, high) + success return

    // L0 RX Residency Counter (read FIRST in actual implementation)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_rx_res & 0xFFFFFFFF)); // counter_low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_rx_res >> 32));        // counter_high
    will_return(__wrap_d2dss_pmu_read, 0);                                  // result - success

    // L0 TX Residency Counter (read SECOND in actual implementation)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_tx_res & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_tx_res >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // L0 RX Bandwidth Flit Counter (read FIRST in actual implementation)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_bw_rx & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_bw_rx >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // L0 TX Bandwidth Flit Counter (read SECOND in actual implementation)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_bw_tx & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0_bw_tx >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // Mock L0s state counters (2 counters: rx_res first, then tx_res)
    // L0s RX Residency Counter (read FIRST)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0s_rx_res & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0s_rx_res >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // L0s TX Residency Counter (read SECOND)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0s_tx_res & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l0s_tx_res >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // Mock L1 state counters (2 counters: rx_res first, then tx_res)
    // L1 RX Residency Counter (read FIRST)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l1_rx_res & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l1_rx_res >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);

    // L1 TX Residency Counter (read SECOND)
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l1_tx_res & 0xFFFFFFFF));
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(l1_tx_res >> 32));
    will_return(__wrap_d2dss_pmu_read, 0);
}

// Test function to verify D2D link state telemetry integration
// Validates the complete D2D telemetry flow from PMU counter mocking to record creation
//
// Test Flow Summary:
// ==================
// 1. Setup Input: Mock D2DSS PMU counters for interface 0 with realistic values
// 2. Process Data: Call data processing twice (baseline + difference calculation)
// 3. Create Record: Generate D2D link state telemetry record
// 4. Verify Results: Assert record values match expected conversions
//   - L0 TX: 2.222B counters → 1111ms, L0 RX: 4.444B counters → 2222ms
//   - L0 BW: 524K flits → 32MB TX, 1048K flits → 64MB RX
//   - L0s TX: 6.666B counters → 3333ms, L0s RX: 8.888B counters → 4444ms
//   - L1 TX: 11.11B counters → 5555ms, L1 RX: 13.332B counters → 6666ms
TEST_FUNCTION(test_d2d_link_state_basic_flow, test_setup, test_teardown)
{
    if (g_print_debug)
    {
        printf("\n=== D2D Link State Test: PMU Counter Processing ===\n");
    }

    // ===== EXPLICIT EXPECTED VALUES =====
    // These are the EXACT values we expect to see in the final D2D record
    // Defined explicitly and independently to prevent false positives from calculation bugs
    // Using unique values to make test failures easier to diagnose
    uint64_t expected_l0_tx_residency_ms = 1111;        // Expected L0 TX residency: 1111 milliseconds
    uint64_t expected_l0_rx_residency_ms = 2222;        // Expected L0 RX residency: 2222 milliseconds
    uint64_t expected_l0_tx_bandwidth_bytes = 33554432; // Expected L0 TX bandwidth: 33554432 bytes (32MB)
    uint64_t expected_l0_rx_bandwidth_bytes = 67108864; // Expected L0 RX bandwidth: 67108864 bytes (64MB)
    uint64_t expected_l0s_tx_residency_ms = 3333;       // Expected L0s TX residency: 3333 milliseconds
    uint64_t expected_l0s_rx_residency_ms = 4444;       // Expected L0s RX residency: 4444 milliseconds
    uint64_t expected_l1_tx_residency_ms = 5555;        // Expected L1 TX residency: 5555 milliseconds
    uint64_t expected_l1_rx_residency_ms = 6666;        // Expected L1 RX residency: 6666 milliseconds

    if (g_print_debug)
    {
        printf("=== EXPLICIT EXPECTED VALUES ===\n");
        printf("  L0 TX residency: %llu ms\n", expected_l0_tx_residency_ms);
        printf("  L0 RX residency: %llu ms\n", expected_l0_rx_residency_ms);
        printf("  L0 TX bandwidth: %llu bytes\n", expected_l0_tx_bandwidth_bytes);
        printf("  L0 RX bandwidth: %llu bytes\n", expected_l0_rx_bandwidth_bytes);
        printf("  L0s TX residency: %llu ms\n", expected_l0s_tx_residency_ms);
        printf("  L0s RX residency: %llu ms\n", expected_l0s_rx_residency_ms);
        printf("  L1 TX residency: %llu ms\n", expected_l1_tx_residency_ms);
        printf("  L1 RX residency: %llu ms\n", expected_l1_rx_residency_ms);
    }

    // ===== INPUT VALUES - Set to produce the expected outputs above =====
    // These input values are specifically chosen to produce the explicit expected values
    // Conversion formulas used (for reference, but expected values are primary):
    // - Residency: milliseconds = (counter_difference / 2,000,000,000) * 1000
    // - Bandwidth: bytes = flit_count * 64
    uint64_t interface0_l0_tx_residency_pmu_ticks = 2222000000ULL; // Input: 2.222B ticks → Expected: 1111ms
    uint64_t interface0_l0_rx_residency_pmu_ticks = 4444000000ULL; // Input: 4.444B ticks → Expected: 2222ms
    uint64_t interface0_l0_bw_tx_flits = 524288;  // Input: 524288 flits → Expected: 33554432 bytes (32MB)
    uint64_t interface0_l0_bw_rx_flits = 1048576; // Input: 1048576 flits → Expected: 67108864 bytes (64MB)
    uint64_t interface0_l0s_tx_residency_pmu_ticks = 6666000000ULL; // Input: 6.666B ticks → Expected: 3333ms
    uint64_t interface0_l0s_rx_residency_pmu_ticks = 8888000000ULL; // Input: 8.888B ticks → Expected: 4444ms
    uint64_t interface0_l1_tx_residency_pmu_ticks = 11110000000ULL; // Input: 11.11B ticks → Expected: 5555ms
    uint64_t interface0_l1_rx_residency_pmu_ticks = 13332000000ULL; // Input: 13.332B ticks → Expected: 6666ms

    if (g_print_debug)
    {
        printf("=== INPUT MOCK VALUES (with corresponding expected outputs) ===\n");
        printf("  L0 TX Residency Counter:  %llu ticks → Expected: %llu ms\n",
               interface0_l0_tx_residency_pmu_ticks,
               expected_l0_tx_residency_ms);
        printf("  L0 RX Residency Counter:  %llu ticks → Expected: %llu ms\n",
               interface0_l0_rx_residency_pmu_ticks,
               expected_l0_rx_residency_ms);
        printf("  L0 TX Bandwidth Flits:    %llu flits → Expected: %llu bytes\n", interface0_l0_bw_tx_flits, expected_l0_tx_bandwidth_bytes);
        printf("  L0 RX Bandwidth Flits:    %llu flits → Expected: %llu bytes\n", interface0_l0_bw_rx_flits, expected_l0_rx_bandwidth_bytes);
        printf("  L0s TX Residency Counter: %llu ticks → Expected: %llu ms\n",
               interface0_l0s_tx_residency_pmu_ticks,
               expected_l0s_tx_residency_ms);
        printf("  L0s RX Residency Counter: %llu ticks → Expected: %llu ms\n",
               interface0_l0s_rx_residency_pmu_ticks,
               expected_l0s_rx_residency_ms);
        printf("  L1 TX Residency Counter:  %llu ticks → Expected: %llu ms\n",
               interface0_l1_tx_residency_pmu_ticks,
               expected_l1_tx_residency_ms);
        printf("  L1 RX Residency Counter:  %llu ticks → Expected: %llu ms\n",
               interface0_l1_rx_residency_pmu_ticks,
               expected_l1_rx_residency_ms);
        printf("\n");
    }

    // Since we call processing twice (first establishes baseline, second calculates diffs),
    // we need to setup mocks for two cycles. Second cycle uses doubled values so:
    // (2 * first) - first = first, giving us our intended counter values as differences.

    if (g_print_debug)
        printf("=== SETUP PHASE 1: Baseline mock values ===\n");

    // First cycle: setup mocks for interface 0
    setup_d2d_pmu_mocks_for_interface(0,
                                      interface0_l0_tx_residency_pmu_ticks,
                                      interface0_l0_rx_residency_pmu_ticks,
                                      interface0_l0_bw_tx_flits,
                                      interface0_l0_bw_rx_flits,
                                      interface0_l0s_tx_residency_pmu_ticks,
                                      interface0_l0s_rx_residency_pmu_ticks,
                                      interface0_l1_tx_residency_pmu_ticks,
                                      interface0_l1_rx_residency_pmu_ticks);

    // For other interfaces (1-7), setup zero values for first cycle
    for (uint8_t i = 1; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        setup_d2d_pmu_mocks_for_interface(i, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    if (g_print_debug)
        printf("=== SETUP PHASE 2: Difference calculation mock values ===\n");

    // Second cycle: setup mocks again for the second processing call
    // Interface 0 gets doubled values so differences = original values
    setup_d2d_pmu_mocks_for_interface(0,
                                      interface0_l0_tx_residency_pmu_ticks * 2,
                                      interface0_l0_rx_residency_pmu_ticks * 2,
                                      interface0_l0_bw_tx_flits * 2,
                                      interface0_l0_bw_rx_flits * 2,
                                      interface0_l0s_tx_residency_pmu_ticks * 2,
                                      interface0_l0s_rx_residency_pmu_ticks * 2,
                                      interface0_l1_tx_residency_pmu_ticks * 2,
                                      interface0_l1_rx_residency_pmu_ticks * 2);

    // For other interfaces (1-7), setup zero values for second cycle
    for (uint8_t i = 1; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        setup_d2d_pmu_mocks_for_interface(i, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    if (g_print_debug)
        printf("DEBUG: All PMU mocks setup complete\n");

    // Step 2: Trigger D2D link telemetry processing
    // Note: D2D counters work on difference calculation, so we need two calls:
    // First call establishes baseline, second call calculates differences
    if (g_print_debug)
        printf("DEBUG: First call to establish counter baselines\n");

    data_proc_tlm_cmpnt_process_one_second_input_data();

    if (g_print_debug)
        printf("DEBUG: Second call to calculate counter differences\n");

    data_proc_tlm_cmpnt_process_one_second_input_data();

    if (g_print_debug)
        printf("DEBUG: Telemetry processing complete\n");

    // Step 3: Create D2D link state record and verify contents
    pwr_soc_record_d2d_link_t d2d_record;
    memset(&d2d_record, 0, sizeof(d2d_record));

    if (g_print_debug)
        printf("DEBUG: Creating link state record\n");

    uint32_t record_size = package_create_pwr_soc_d2d_link_record(&d2d_record);

    // Validate record creation succeeded
    assert_int_not_equal(record_size, 0);

    if (g_print_debug)
    {
        printf("DEBUG: Record created successfully, size: %u bytes\n", record_size);
        printf("DEBUG: Verifying interface 0 link state data\n");
    }

    // --- Extract actual values with clear unit naming for readability ---
    // Note: The struct fields use _mS (capital S) but our expected values use _ms (lowercase s)
    // Both represent milliseconds, but we'll be consistent in variable naming

    // Validate record structure and collection before accessing array elements
    if (record_size > sizeof(telemetry_record_hdr_t))
    {
        uint64_t actual_l0_tx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0].tx_residency_mS;
        uint64_t actual_l0_rx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0].rx_residency_mS;
        uint64_t actual_l0_tx_bandwidth_bytes =
            d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0].bw_tx_flit_bytes;
        uint64_t actual_l0_rx_bandwidth_bytes =
            d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0].bw_rx_flit_bytes;
        uint64_t actual_l0s_tx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0S].tx_residency_mS;
        uint64_t actual_l0s_rx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L0S].rx_residency_mS;
        uint64_t actual_l1_tx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L1].tx_residency_mS;
        uint64_t actual_l1_rx_residency_ms = d2d_record.d2d_link_collection[0].d2d_link_element[D2D_LINK_L1].rx_residency_mS;

        // Verify interface 0 L0 state data (using clear unit-named variables)
        assert_int_equal(actual_l0_tx_residency_ms, expected_l0_tx_residency_ms);
        assert_int_equal(actual_l0_rx_residency_ms, expected_l0_rx_residency_ms);
        assert_int_equal(actual_l0_tx_bandwidth_bytes, expected_l0_tx_bandwidth_bytes);
        assert_int_equal(actual_l0_rx_bandwidth_bytes, expected_l0_rx_bandwidth_bytes);

        // Verify interface 0 L0s state data
        assert_int_equal(actual_l0s_tx_residency_ms, expected_l0s_tx_residency_ms);
        assert_int_equal(actual_l0s_rx_residency_ms, expected_l0s_rx_residency_ms);

        // Verify interface 0 L1 state data
        assert_int_equal(actual_l1_tx_residency_ms, expected_l1_tx_residency_ms);
        assert_int_equal(actual_l1_rx_residency_ms, expected_l1_rx_residency_ms);

        if (g_print_debug)
        {
            printf("\n=== VERIFICATION RESULTS ===\n");
            printf("✓ L0 TX residency: %llu ms == %llu ms\n", actual_l0_tx_residency_ms, expected_l0_tx_residency_ms);
            printf("✓ L0 RX residency: %llu ms == %llu ms\n", actual_l0_rx_residency_ms, expected_l0_rx_residency_ms);
            printf("✓ L0 TX bandwidth: %llu bytes == %llu bytes\n", actual_l0_tx_bandwidth_bytes, expected_l0_tx_bandwidth_bytes);
            printf("✓ L0 RX bandwidth: %llu bytes == %llu bytes\n", actual_l0_rx_bandwidth_bytes, expected_l0_rx_bandwidth_bytes);
            printf("✓ L0s TX residency: %llu ms == %llu ms\n", actual_l0s_tx_residency_ms, expected_l0s_tx_residency_ms);
            printf("✓ L0s RX residency: %llu ms == %llu ms\n", actual_l0s_rx_residency_ms, expected_l0s_rx_residency_ms);
            printf("✓ L1 TX residency: %llu ms == %llu ms\n", actual_l1_tx_residency_ms, expected_l1_tx_residency_ms);
            printf("✓ L1 RX residency: %llu ms == %llu ms\n", actual_l1_rx_residency_ms, expected_l1_rx_residency_ms);
            printf("\n=== ALL VERIFICATIONS PASSED ===\n");
            printf("D2D Link State Test: All assertions successful\n");
        }
    }
    else
    {
        printf("ERROR: D2D link collection is empty or invalid.\n");
        assert_true(record_size > sizeof(telemetry_record_hdr_t));
    }

    if (g_print_debug)
    {
        printf("DEBUG: All D2D link state verifications passed\n");
    }
}