// Copyright (c) Microsoft Corporation. All rights reserved.

/**
 * @file test_droop_count.cpp
 * Functional test for core droop count telemetry metrics
 *
 * Test Flow:
 * ----------
 * test_droop_count_metrics()
 * - Mark all sensor FIFOs as empty to avoid unrelated polling
 * - Set up mock droop count values for each core
 * - Call: data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg()
 * - Verify droop count for each core matches expected mock value
 *
 * Notes:
 * - This test follows the same structure as other functional tests
 * - It validates that droop count values are correctly updated for all active cores
 */

// @SSI_functional_Test
// @SSI_functional_Test:power_telemetry
// TEST_CASE_ID:2779229

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <telemetry_package_defs.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <package_creation_i.h>
}

// === Constants and Macros ===
#define TEST_VOLTAGE_MV  900
#define MOCK_DROOP(core) (50 + (core) * 5)

// Global variable to control debug printing
static bool g_print_logs = true;

// === Setup/Teardown ===
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    memset(core_rt, 0, sizeof(core_rt));
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        core_is_active[core] = true;
        core_rt[core].latest_voltage_mV = TEST_VOLTAGE_MV;
        core_rt[core].latest_vcpu_voltage_mV = TEST_VOLTAGE_MV;
    }
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

// === Test Function ===
TEST_FUNCTION(test_droop_count_metrics, test_setup, test_teardown)
{
    // Mark all sensor FIFOs as empty to avoid polling in this test
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
    {
        test_snsr_fifo_is_empty[i] = true;
    }
    will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

    if (g_print_logs)
    {
        printf("\n=== Droop Count Telemetry Test (All Cores) ===\n");
        printf("NUMBER_OF_CORES_PER_DIE = %u\n", (unsigned int)NUMBER_OF_CORES_PER_DIE);
        for (unsigned int i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
            printf("core_is_active[%u] = %d\n", i, core_is_active[i]);
    }

    uint64_t mock_droop_counts[NUMBER_OF_CORES_PER_DIE];
    // Initialize mock droop counts with a unique pattern for each core
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
        mock_droop_counts[core] = core; // Strong pattern: each core gets its index
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, mock_droop_counts);

    data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg();

    // Also create the droop count record
    pwr_core_record_droop_count_t record;
    memset(&record, 0, sizeof(record));
    package_create_pwr_core_droop_count_record(&record);

    for (unsigned int i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        uint64_t record_droop = record.droop_count_collection[i].droop_count_element.droop_count;
        if (g_print_logs)
        {
            printf("Core %u: expected_droop=%u, record_droop=%llu, core_is_active=%d\n",
                   i,
                   i,
                   (unsigned long long)record_droop,
                   core_is_active[i]);
            if (record_droop != i)
            {
                printf("ASSERT FAIL: Core %u RECORD DROOP expected=%u, actual=%llu\n", i, i, (unsigned long long)record_droop);
            }
        }
        assert_int_equal(record_droop, i);
    }

    if (g_print_logs)
    {
        printf("TEST END: test_droop_count_metrics\n");
    }
}
