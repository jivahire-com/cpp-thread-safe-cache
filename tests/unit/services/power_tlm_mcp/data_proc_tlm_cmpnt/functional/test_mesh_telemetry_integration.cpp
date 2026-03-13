//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh_telemetry_integration.cpp
 * Integration test for Log per Die Mesh Telemetry data collection and processing
 *
 * Objective:
 *   Validate Log per Die Mesh Telemetry collection, processing, and record creation.
 *   Test register-level mock interactions for mesh PMU counter APIs over 1 second intervals.
 *
 * Generic Test Flow:
 * ------------------
 * 1. Setup mock PMU counter read operations for mesh hardware APIs
 * 2. Configure M0/M1/M2 residency counters and M1/M2 entry counts with delivered performance counts
 * 3. Trigger data_smpl_update_metrics_for_per_die_mesh_counters() in 1 second intervals
 * 4. Create Die Mesh record via package_create_pwr_soc_die_mesh_record()
 * 5. Verify/Assert results for mesh telemetry data conversion and packaging
 *
 * Test Functions:
 * ---------------
 * 1. test_mesh_telemetry_single_interval: Tests single 1-second mesh telemetry collection
 *    - Validates M0, M1, M2 residency counter processing (PMU counters → metrics)
 *    - Verifies M1, M2 entry count and delivered performance count collection
 *    - Tests mesh clock telemetry re-initialization after collection
 *    - Confirms proper record creation with expected counter values
 *
 * 2. test_mesh_telemetry_multiple_intervals: Tests multiple 1-second interval collection
 *    - Validates accumulation of mesh telemetry data over multiple collection cycles
 *    - Tests proper re-initialization between collection intervals
 *    - Verifies final record contains accumulated metric data
 *
 * Notes:
 * ------
 * - Mesh telemetry collection runs every 1 second via data_smpl_update_metrics_for_per_die_mesh_counters()
 * - PMU counters return raw count values that get stored directly in computed metrics
 * - Each collection cycle re-initializes mesh telemetry for the next 1-second window
 * - Test uses unique counter values (1111, 2222, 3333, etc.) for easy debugging
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779236

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
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <telemetry_package_defs.h>
extern uint32_t time_t0;
}

// Global variable to control debug printing
static bool g_print_logs = true;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0, false, 0, true); // die_id=0, is_single_die=false, mpam_vm_mem_fixed_pwr_mW=0, all_zero_filtering_enable=true

    // Reset mock timestamp to initial value
    time_t0 = 1000;

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

// Helper function to setup mesh PMU counter mocks for a single collection cycle
static void setup_mesh_pmu_mocks(uint32_t m0_residency,
                                 uint32_t m1_residency,
                                 uint32_t m2_residency,
                                 uint32_t m1_entry_count,
                                 uint32_t m2_entry_count,
                                 uint32_t delivered_perf_count)
{
    if (g_print_logs)
    {
        printf("DEBUG: Setting up mesh PMU mocks\n");
        printf("  M0 residency: %u\n", m0_residency);
        printf("  M1 residency: %u, entry count: %u\n", m1_residency, m1_entry_count);
        printf("  M2 residency: %u, entry count: %u\n", m2_residency, m2_entry_count);
        printf("  Delivered perf count: %u\n", delivered_perf_count);
    }

    // Mock mesh hardware API returns for PMU counter reads
    // These APIs are called in order by data_smpl_update_metrics_for_per_die_mesh_counters()
    will_return(__wrap_mesh_get_m0_residency, m0_residency);
    will_return(__wrap_mesh_get_m1_residency, m1_residency);
    will_return(__wrap_mesh_get_m2_residency, m2_residency);
    will_return(__wrap_mesh_get_m1_entry_count, m1_entry_count);
    will_return(__wrap_mesh_get_m2_entry_count, m2_entry_count);
    will_return(__wrap_mesh_get_telemetry_delivered_perf_count, delivered_perf_count);

    // Mock mesh clock telemetry re-initialization call (happens at end of collection cycle)
    expect_value(__wrap_mesh_clock_telemetry, enable, true);
    expect_value(__wrap_mesh_clock_telemetry, interval_count, PER_DIE_MESH_PWR_TLM_INTERVAL);
    expect_function_call(__wrap_mesh_clock_telemetry);
}

// Test function to verify single 1-second mesh telemetry collection interval
// Validates the complete mesh telemetry flow from PMU counter mocking to record creation
//
// Test Flow Summary:
// ==================
// 1. Setup Input: Mock mesh PMU counters with raw tick count values (2 GHz clock)
// 2. Process Data: Call data_smpl_update_metrics_for_per_die_mesh_counters() (1 second interval)
// 3. Create Record: Generate Die Mesh telemetry record (real data retrieval, no mock)
// 4. Verify Results: Assert record values match MESH_COUNTER_TO_MS(input) for residency/perf
//    and raw values for entry counts
//   - M0 Residency: 2714000000 ticks → MESH_COUNTER_TO_MS → 1357 mS
//   - M1 Residency: 4936000000 ticks → MESH_COUNTER_TO_MS → 2468 mS, Entry Count: 17 → 17
//   - M2 Residency: 7158000000 ticks → MESH_COUNTER_TO_MS → 3579 mS, Entry Count: 29 → 29
//   - Delivered Performance: 9362000000 ticks → MESH_COUNTER_TO_MS → 4681 mS
TEST_FUNCTION(test_mesh_telemetry_single_interval, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mesh_telemetry_single_interval ---\n");
        printf("=== Mesh Telemetry Test: Single 1-Second Collection ===\n");
    }

    // ===== EXPLICIT EXPECTED VALUES =====
    // These are the EXACT values we expect to see in the final Die Mesh record
    // Defined explicitly and independently to prevent false positives from calculation bugs
    // Residency/perf values are the result of MESH_COUNTER_TO_MS(raw_ticks)
    //   MESH_COUNTER_TO_MS(counter) = (counter / 2,000,000,000) * 1000
    // Entry counts are copied raw (no conversion)
    // Note: Input ticks must fit in uint32_t (max ~4.29e9 → ~2147 mS max)
    uint64_t expected_m0_residency_mS = 500;    // Expected M0 residency: 500 milliseconds
    uint64_t expected_m1_residency_mS = 750;    // Expected M1 residency: 750 milliseconds
    uint64_t expected_m2_residency_mS = 1000;   // Expected M2 residency: 1000 milliseconds
    uint64_t expected_m1_entry_count = 17;      // Expected M1 entry count: 17 entries (raw)
    uint64_t expected_m2_entry_count = 29;      // Expected M2 entry count: 29 entries (raw)
    uint64_t expected_delivered_perf_mS = 1250; // Expected delivered performance: 1250 milliseconds

    if (g_print_logs)
    {
        printf("=== EXPLICIT EXPECTED VALUES ===\n");
        printf("  M0 residency mS: %llu\n", expected_m0_residency_mS);
        printf("  M1 residency mS: %llu\n", expected_m1_residency_mS);
        printf("  M2 residency mS: %llu\n", expected_m2_residency_mS);
        printf("  M1 entry count: %llu\n", expected_m1_entry_count);
        printf("  M2 entry count: %llu\n", expected_m2_entry_count);
        printf("  Delivered perf mS: %llu\n", expected_delivered_perf_mS);
    }

    // ===== INPUT VALUES - Raw PMU tick counts (2 GHz clock) =====
    // These input values are raw hardware counter ticks fed to mesh_get_* mocks
    // The real data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data() applies MESH_COUNTER_TO_MS()
    // for residency/perf counters: (ticks / 2e9) * 1000 = mS
    // Entry counts are passed through raw (no conversion)
    // NEW E2E values: raw ticks = desired_mS * DIE_MESH_FREQ_HZ / 1000
    //   DIE_MESH_FREQ_HZ = 2,000,000,000 (2 GHz)
    //   Max uint32_t = 4,294,967,295 → max ~2147 mS
    uint32_t input_m0_residency_ticks = 1000000000UL; // 500 * 2e6 = 1,000,000,000 ticks → 500 mS
    uint32_t input_m1_residency_ticks = 1500000000UL; // 750 * 2e6 = 1,500,000,000 ticks → 750 mS
    uint32_t input_m2_residency_ticks = 2000000000UL; // 1000 * 2e6 = 2,000,000,000 ticks → 1000 mS
    uint32_t input_m1_entry_count = (uint32_t)expected_m1_entry_count; // 17 entries (raw, no conversion)
    uint32_t input_m2_entry_count = (uint32_t)expected_m2_entry_count; // 29 entries (raw, no conversion)
    uint32_t input_delivered_perf_ticks = 2500000000UL; // 1250 * 2e6 = 2,500,000,000 ticks → 1250 mS

    if (g_print_logs)
    {
        printf("=== INPUT MOCK VALUES (raw ticks → MESH_COUNTER_TO_MS → expected mS) ===\n");
        printf("  M0 Residency ticks: %u → Expected: %llu mS\n", input_m0_residency_ticks, expected_m0_residency_mS);
        printf("  M1 Residency ticks: %u → Expected: %llu mS\n", input_m1_residency_ticks, expected_m1_residency_mS);
        printf("  M2 Residency ticks: %u → Expected: %llu mS\n", input_m2_residency_ticks, expected_m2_residency_mS);
        printf("  M1 Entry count:     %u → Expected: %llu (raw)\n", input_m1_entry_count, expected_m1_entry_count);
        printf("  M2 Entry count:     %u → Expected: %llu (raw)\n", input_m2_entry_count, expected_m2_entry_count);
        printf("  Delivered Perf ticks: %u → Expected: %llu mS\n", input_delivered_perf_ticks, expected_delivered_perf_mS);
        printf("\n");
    }

    // Step 1: Setup mesh PMU counter mocks (will be called during mesh telemetry collection)
    if (g_print_logs)
        printf("=== SETUP: Mesh PMU mocks for 1-second collection ===\n");

    setup_mesh_pmu_mocks(input_m0_residency_ticks,
                         input_m1_residency_ticks,
                         input_m2_residency_ticks,
                         input_m1_entry_count,
                         input_m2_entry_count,
                         input_delivered_perf_ticks);

    // Step 2: Trigger 1-second mesh telemetry collection
    if (g_print_logs)
        printf("DEBUG: Calling data_smpl_update_metrics_for_per_die_mesh_counters() for mesh collection\n");

    data_smpl_update_metrics_for_per_die_mesh_counters();

    if (g_print_logs)
        printf("DEBUG: Mesh telemetry collection complete\n");

    // Step 3: Create Die Mesh record and verify contents
    pwr_soc_record_die_mesh_t die_mesh_record;
    memset(&die_mesh_record, 0, sizeof(die_mesh_record));

    if (g_print_logs)
        printf("DEBUG: Creating die mesh record\n");

    // Now using real data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data() which reads from
    // computed_metrics_2_mins populated by data_smpl_update_metrics_for_per_die_mesh_counters()

    uint32_t record_size = package_create_pwr_soc_die_mesh_record(&die_mesh_record);

    // Validate record creation succeeded
    assert_int_not_equal(record_size, 0);

    if (g_print_logs)
    {
        printf("DEBUG: Record created successfully, size: %u bytes\n", record_size);
        printf("DEBUG: Verifying die mesh telemetry data\n");
    }

    // --- Extract actual values with clear unit naming for readability ---

    // Validate record structure before accessing fields
    if (record_size > sizeof(telemetry_record_hdr_t))
    {
        uint64_t actual_m0_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m0_residency_mS;
        uint64_t actual_m1_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m1_residency_mS;
        uint64_t actual_m2_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m2_residency_mS;
        uint64_t actual_m1_entry_count = die_mesh_record.die_mesh_collection.die_mesh_element.m1_entry_count;
        uint64_t actual_m2_entry_count = die_mesh_record.die_mesh_collection.die_mesh_element.m2_entry_count;
        uint64_t actual_delivery_perf_mS = die_mesh_record.die_mesh_collection.die_mesh_element.delivery_perf_mS;

        // Verify die mesh telemetry data (using clear unit-named variables)
        assert_int_equal(actual_m0_residency_mS, expected_m0_residency_mS);
        assert_int_equal(actual_m1_residency_mS, expected_m1_residency_mS);
        assert_int_equal(actual_m2_residency_mS, expected_m2_residency_mS);
        assert_int_equal(actual_m1_entry_count, expected_m1_entry_count);
        assert_int_equal(actual_m2_entry_count, expected_m2_entry_count);
        assert_int_equal(actual_delivery_perf_mS, expected_delivered_perf_mS);

        if (g_print_logs)
        {
            printf("\n=== VERIFICATION RESULTS (Input \u2192 Actual == Expected) ===\n");
            printf("M0 residency: %u ticks \u2192 %llu mS == %llu mS\n", input_m0_residency_ticks, actual_m0_residency_mS, expected_m0_residency_mS);
            printf("M1 residency: %u ticks \u2192 %llu mS == %llu mS\n", input_m1_residency_ticks, actual_m1_residency_mS, expected_m1_residency_mS);
            printf("M2 residency: %u ticks \u2192 %llu mS == %llu mS\n", input_m2_residency_ticks, actual_m2_residency_mS, expected_m2_residency_mS);
            printf("M1 entry count: %u \u2192 %llu == %llu\n", input_m1_entry_count, actual_m1_entry_count, expected_m1_entry_count);
            printf("M2 entry count: %u \u2192 %llu == %llu\n", input_m2_entry_count, actual_m2_entry_count, expected_m2_entry_count);
            printf("Delivery perf: %u ticks \u2192 %llu mS == %llu mS\n",
                   input_delivered_perf_ticks,
                   actual_delivery_perf_mS,
                   expected_delivered_perf_mS);
            printf("\n=== ALL VERIFICATIONS PASSED ===\n");
            printf("Mesh Telemetry Test: Single interval assertions successful\n");
        }
    }
    else
    {
        printf("ERROR: Die mesh record is empty or invalid.\n");
        assert_true(record_size > sizeof(telemetry_record_hdr_t));
    }

    if (g_print_logs)
    {
        printf("DEBUG: All die mesh telemetry verifications passed\n");
        printf("--- END test_mesh_telemetry_single_interval ---\n");
        printf("***\n");
    }
}

// Test function to verify multiple 1-second mesh telemetry collection intervals
// Validates accumulation of mesh telemetry data over multiple collection cycles
//
// Test Flow Summary:
// ==================
// 1. Setup Input: Mock mesh PMU counters with raw tick counts for multiple 1-second intervals
// 2. Process Data: Call data_smpl_update_metrics_for_per_die_mesh_counters() multiple times
// 3. Create Record: Generate Die Mesh telemetry record (real data retrieval, no mock)
// 4. Verify Results: Assert record contains ACCUMULATED data from both intervals
//    comp_metrics_for_per_die_mesh_tlm() uses += so values accumulate across intervals
//   - Interval 1: M0=600M ticks, M1=800M ticks, M2=400M ticks, M1_entry=87, M2_entry=76, Perf=1.2B ticks
//   - Interval 2: M0=400M ticks, M1=200M ticks, M2=600M ticks, M1_entry=54, M2_entry=43, Perf=800M ticks
//   - Accumulated ticks: M0=1B, M1=1B, M2=1B, M1_entry=141, M2_entry=119, Perf=2B
//   - Expected mS (MESH_COUNTER_TO_MS): M0=500, M1=500, M2=500, Perf=1000
TEST_FUNCTION(test_mesh_telemetry_multiple_intervals, test_setup, test_teardown)
{
    if (g_print_logs)
    {
        printf("\n***\n");
        printf("--- START test_mesh_telemetry_multiple_intervals ---\n");
        printf("=== Mesh Telemetry Test: Multiple 1-Second Collections ===\n");
    }

    // ===== INTERVAL 1 VALUES (raw PMU tick counts, 2 GHz clock) =====
    uint32_t interval1_m0_residency_ticks = 600000000UL; // 600M ticks
    uint32_t interval1_m1_residency_ticks = 800000000UL; // 800M ticks
    uint32_t interval1_m2_residency_ticks = 400000000UL; // 400M ticks
    uint32_t interval1_m1_entry_count = 87;
    uint32_t interval1_m2_entry_count = 76;
    uint32_t interval1_delivered_perf_ticks = 1200000000UL; // 1.2B ticks

    // ===== INTERVAL 2 VALUES (raw PMU tick counts, 2 GHz clock) =====
    uint32_t interval2_m0_residency_ticks = 400000000UL; // 400M ticks
    uint32_t interval2_m1_residency_ticks = 200000000UL; // 200M ticks
    uint32_t interval2_m2_residency_ticks = 600000000UL; // 600M ticks
    uint32_t interval2_m1_entry_count = 54;
    uint32_t interval2_m2_entry_count = 43;
    uint32_t interval2_delivered_perf_ticks = 800000000UL; // 800M ticks

    // ===== EXPECTED VALUES (explicit, based on accumulated ticks + MESH_COUNTER_TO_MS) =====
    // comp_metrics_for_per_die_mesh_tlm() accumulates via +=, so final state = interval1 + interval2
    // Accumulated ticks: M0 = 600M+400M = 1B, M1 = 800M+200M = 1B, M2 = 400M+600M = 1B
    // MESH_COUNTER_TO_MS(1,000,000,000) = (1e9 / 2e9) * 1000 = 500 mS
    // Accumulated perf ticks: 1.2B+800M = 2B → MESH_COUNTER_TO_MS(2e9) = 1000 mS
    // Entry counts are raw accumulated: 87+54=141, 76+43=119

    uint64_t expected_m0_residency_mS = 500;    // MESH_COUNTER_TO_MS(1,000,000,000) = 500 mS
    uint64_t expected_m1_residency_mS = 500;    // MESH_COUNTER_TO_MS(1,000,000,000) = 500 mS
    uint64_t expected_m2_residency_mS = 500;    // MESH_COUNTER_TO_MS(1,000,000,000) = 500 mS
    uint64_t expected_m1_entry_count = 141;     // 87 + 54 = 141 (raw accumulated)
    uint64_t expected_m2_entry_count = 119;     // 76 + 43 = 119 (raw accumulated)
    uint64_t expected_delivered_perf_mS = 1000; // MESH_COUNTER_TO_MS(2,000,000,000) = 1000 mS

    if (g_print_logs)
    {
        printf("=== MULTIPLE INTERVAL TEST PLAN ===\n");
        printf("Interval 1 (ticks): M0=%u, M1=%u, M2=%u, M1_entry=%u, M2_entry=%u, Perf=%u\n",
               interval1_m0_residency_ticks,
               interval1_m1_residency_ticks,
               interval1_m2_residency_ticks,
               interval1_m1_entry_count,
               interval1_m2_entry_count,
               interval1_delivered_perf_ticks);
        printf("Interval 2 (ticks): M0=%u, M1=%u, M2=%u, M1_entry=%u, M2_entry=%u, Perf=%u\n",
               interval2_m0_residency_ticks,
               interval2_m1_residency_ticks,
               interval2_m2_residency_ticks,
               interval2_m1_entry_count,
               interval2_m2_entry_count,
               interval2_delivered_perf_ticks);
        printf("Expected (accumulated + MESH_COUNTER_TO_MS): M0=%llu, M1=%llu, M2=%llu, M1_entry=%llu, "
               "M2_entry=%llu, Perf=%llu\n",
               expected_m0_residency_mS,
               expected_m1_residency_mS,
               expected_m2_residency_mS,
               expected_m1_entry_count,
               expected_m2_entry_count,
               expected_delivered_perf_mS);
        printf("Expected final record: contains accumulated data from both intervals\n\n");
    }

    // Step 1: First 1-second collection interval
    if (g_print_logs)
        printf("=== INTERVAL 1: First 1-second mesh collection ===\n");

    setup_mesh_pmu_mocks(interval1_m0_residency_ticks,
                         interval1_m1_residency_ticks,
                         interval1_m2_residency_ticks,
                         interval1_m1_entry_count,
                         interval1_m2_entry_count,
                         interval1_delivered_perf_ticks);

    data_smpl_update_metrics_for_per_die_mesh_counters();

    if (g_print_logs)
        printf("DEBUG: Interval 1 collection complete\n");

    // Step 2: Second 1-second collection interval
    if (g_print_logs)
        printf("\n=== INTERVAL 2: Second 1-second mesh collection ===\n");

    setup_mesh_pmu_mocks(interval2_m0_residency_ticks,
                         interval2_m1_residency_ticks,
                         interval2_m2_residency_ticks,
                         interval2_m1_entry_count,
                         interval2_m2_entry_count,
                         interval2_delivered_perf_ticks);

    data_smpl_update_metrics_for_per_die_mesh_counters();

    if (g_print_logs)
        printf("DEBUG: Interval 2 collection complete\n");

    // Step 3: Create Die Mesh record and verify contents (no data preparation needed since we called collection directly)
    pwr_soc_record_die_mesh_t die_mesh_record;
    memset(&die_mesh_record, 0, sizeof(die_mesh_record));

    if (g_print_logs)
        printf("\nDEBUG: Creating die mesh record after multiple intervals\n");

    // Now using real data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data() which reads from
    // computed_metrics_2_mins populated by data_smpl_update_metrics_for_per_die_mesh_counters()

    uint32_t record_size = package_create_pwr_soc_die_mesh_record(&die_mesh_record);

    // Validate record creation succeeded
    assert_int_not_equal(record_size, 0);

    if (g_print_logs)
    {
        printf("DEBUG: Record created successfully, size: %u bytes\n", record_size);
        printf("DEBUG: Verifying die mesh telemetry data contains accumulated interval values\n");
    }

    // --- Extract actual values and verify they match the latest collection interval ---

    if (record_size > sizeof(telemetry_record_hdr_t))
    {
        uint64_t actual_m0_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m0_residency_mS;
        uint64_t actual_m1_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m1_residency_mS;
        uint64_t actual_m2_residency_mS = die_mesh_record.die_mesh_collection.die_mesh_element.m2_residency_mS;
        uint64_t actual_m1_entry_count = die_mesh_record.die_mesh_collection.die_mesh_element.m1_entry_count;
        uint64_t actual_m2_entry_count = die_mesh_record.die_mesh_collection.die_mesh_element.m2_entry_count;
        uint64_t actual_delivery_perf_mS = die_mesh_record.die_mesh_collection.die_mesh_element.delivery_perf_mS;

        // Verify die mesh telemetry data matches accumulated intervals (interval1 + interval2)
        assert_int_equal(actual_m0_residency_mS, expected_m0_residency_mS);
        assert_int_equal(actual_m1_residency_mS, expected_m1_residency_mS);
        assert_int_equal(actual_m2_residency_mS, expected_m2_residency_mS);
        assert_int_equal(actual_m1_entry_count, expected_m1_entry_count);
        assert_int_equal(actual_m2_entry_count, expected_m2_entry_count);
        assert_int_equal(actual_delivery_perf_mS, expected_delivered_perf_mS);

        if (g_print_logs)
        {
            printf("\n=== VERIFICATION RESULTS (Input \u2192 Actual == Expected, Accumulated) ===\n");
            printf("M0 residency: %u+%u ticks \u2192 %llu mS == %llu mS\n",
                   interval1_m0_residency_ticks,
                   interval2_m0_residency_ticks,
                   actual_m0_residency_mS,
                   expected_m0_residency_mS);
            printf("M1 residency: %u+%u ticks \u2192 %llu mS == %llu mS\n",
                   interval1_m1_residency_ticks,
                   interval2_m1_residency_ticks,
                   actual_m1_residency_mS,
                   expected_m1_residency_mS);
            printf("M2 residency: %u+%u ticks \u2192 %llu mS == %llu mS\n",
                   interval1_m2_residency_ticks,
                   interval2_m2_residency_ticks,
                   actual_m2_residency_mS,
                   expected_m2_residency_mS);
            printf("M1 entry count: %u+%u \u2192 %llu == %llu\n",
                   interval1_m1_entry_count,
                   interval2_m1_entry_count,
                   actual_m1_entry_count,
                   expected_m1_entry_count);
            printf("M2 entry count: %u+%u \u2192 %llu == %llu\n",
                   interval1_m2_entry_count,
                   interval2_m2_entry_count,
                   actual_m2_entry_count,
                   expected_m2_entry_count);
            printf("Delivery perf: %u+%u ticks \u2192 %llu mS == %llu mS\n",
                   interval1_delivered_perf_ticks,
                   interval2_delivered_perf_ticks,
                   actual_delivery_perf_mS,
                   expected_delivered_perf_mS);
            printf("\n=== ALL VERIFICATIONS PASSED ===\n");
            printf("Mesh Telemetry Test: Multiple interval assertions successful\n");
        }
    }
    else
    {
        printf("ERROR: Die mesh record is empty or invalid.\n");
        assert_true(record_size > sizeof(telemetry_record_hdr_t));
    }

    if (g_print_logs)
    {
        printf("DEBUG: All die mesh telemetry multiple interval verifications passed\n");
        printf("--- END test_mesh_telemetry_multiple_intervals ---\n");
        printf("***\n");
    }
}