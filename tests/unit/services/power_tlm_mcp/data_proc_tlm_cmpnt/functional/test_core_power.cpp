//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_power.cpp
 * Functional test for core power telemetry metrics
 *
 * Test Flow:
 * ----------
 * test_multi_entry_multi_core_power()
 * - Set up mock core current FIFO data for multiple cores and entries
 * - Simulate sensor polling responses
 * - Call: data_smpl_process_core_current_sensor_fifo()
 * - Call: package_create_pwr_core_power_record()
 * - Verify latest power values for each core
 *
 * Notes:
 * - This test follows the same structure as other functional tests
 * - It validates that power values are correctly computed and packaged
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779224

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

// Constants and macros for mock setup (multi-core)
#define NUM_CORES             2
#define NUM_ENTRIES           2
#define MOCK_PWR(core, entry) (50 + (core) * 10 + (entry) * 5)

// Global variable to control debug printing
static bool g_print_logs = true;

// Setup function
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();                // Clears telemetry state
    memset(core_rt, 0, sizeof(core_rt)); // Clears runtime core data
    //  enable all cores for these tests
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        core_is_active[core] = true;
    }
    in_band_publishing_active = true;

    return 0;
}

// Teardown function
static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data(); // Clean up telemetry state
    return 0;
}

// Helper to set up mock current data and will_return calls
static void setup_mock_current_data(core_current_t* mock_data,
                                    int core_index,
                                    int avg,
                                    int min,
                                    int max,
                                    int volt,
                                    int pwr,
                                    int pstate,
                                    uint64_t timestamp,
                                    int poll_repeat)
{
    mock_data->timestamp = timestamp;
    mock_data->data.avg = avg;
    mock_data->data.min = min;
    mock_data->data.max = max;
    mock_data->data.volt = volt;
    mock_data->data.pwr = pwr;
    mock_data->data.pstate = pstate;
    mock_data->data.mpam_id_low = 1;
    mock_data->data.cstate = 0;
    for (int i = 0; i < poll_repeat; ++i)
    {
        will_return(__wrap_sensor_fifo_svc_poll_core_current, core_index);
        will_return(__wrap_sensor_fifo_svc_poll_core_current, true);
        will_return(__wrap_sensor_fifo_svc_poll_core_current, (i == poll_repeat - 1 ? 0 : 1));
        will_return(__wrap_sensor_fifo_svc_poll_core_current, mock_data);
    }
}

// Test configuration for two cores and two entries
typedef struct
{
    int avg;
    int min;
    int max;
    int volt;
    int pwr;
    int pstate;
} test_config_t;

static const test_config_t test_configs[NUM_CORES][NUM_ENTRIES] = {
    {{10, 8, 12, 100, MOCK_PWR(0, 0), 2}, {11, 9, 13, 100, MOCK_PWR(0, 1), 2}},  // Core 0
    {{20, 18, 22, 100, MOCK_PWR(1, 0), 3}, {21, 19, 23, 100, MOCK_PWR(1, 1), 3}} // Core 1
};

// Test function
TEST_FUNCTION(test_multi_entry_multi_core_power, test_setup, test_teardown)
{

    // Print test start
    printf("\n=== Core Power Telemetry Test (Multi-Core, Multi-Entry) ===\n");

    // Arrays to track expected min, max, and sum for each core
    int expected_min[NUM_CORES], expected_max[NUM_CORES], sum[NUM_CORES];
    for (int core = 0; core < NUM_CORES; ++core)
    {
        expected_min[core] = INT_MAX;
        expected_max[core] = INT_MIN;
        sum[core] = 0;
    }

    // Simulate NUM_ENTRIES samples for each core
    for (int entry = 0; entry < NUM_ENTRIES; ++entry)
    {
        for (int core = 0; core < NUM_CORES; ++core)
        {
            // Set voltage for P=V×I power calculation before processing
            // To achieve expected_power = pwr * 32, with current = avg * 32.2
            // We need voltage such that: (avg * 32.2 * voltage) / 1000 = pwr * 32
            // For avg=10, pwr=50: voltage ≈ (50 * 32 * 1000) / (10 * 32.2) ≈ 4969 mV
            // For avg=20, pwr=60: voltage ≈ (60 * 32 * 1000) / (20 * 32.2) ≈ 2981 mV
            // Using a constant voltage of ~3100 mV gives reasonable approximation
            uint32_t expected_power_mW = test_configs[core][entry].pwr * CORE_POWER_MW_PER_BIT;
            uint32_t current_mA = (uint32_t)(test_configs[core][entry].avg * CORE_CURRENT_CONVERSION_FACTOR);
            // Add rounding compensation: add (current_mA - 1) to numerator before division to round up
            core_rt[core].latest_voltage_mV = ((expected_power_mW * 1000) + current_mA - 1) / current_mA;

            // Prepare and mock core current data for this core/entry
            core_current_t mock_data = {0};
            uint64_t timestamp = 1000 + 100 * entry + 1000 * core;
            setup_mock_current_data(&mock_data,
                                    core,
                                    test_configs[core][entry].avg,
                                    test_configs[core][entry].min,
                                    test_configs[core][entry].max,
                                    test_configs[core][entry].volt,
                                    test_configs[core][entry].pwr,
                                    test_configs[core][entry].pstate,
                                    timestamp,
                                    2); // 2 polling cycles per entry

            // Process the mocked data (simulate hardware polling)
            data_smpl_process_core_current_sensor_fifo();

            // Update expected min/max/avg for this core
            int entry_power = test_configs[core][entry].pwr * CORE_POWER_MW_PER_BIT;
            if (entry_power < expected_min[core])
                expected_min[core] = entry_power;
            if (entry_power > expected_max[core])
                expected_max[core] = entry_power;
            sum[core] += entry_power;
        }
    }

    // Calculate expected power and average for each core
    int expected_power[NUM_CORES], expected_avg[NUM_CORES];
    for (int core = 0; core < NUM_CORES; ++core)
    {
        expected_power[core] = test_configs[core][NUM_ENTRIES - 1].pwr * CORE_POWER_MW_PER_BIT;
        expected_avg[core] = sum[core] / NUM_ENTRIES;
    }

    // Package and verify the results for each core
    pwr_core_record_power_t record;
    package_create_pwr_core_power_record(&record);

    for (int core = 0; core < NUM_CORES; ++core)
    {
        // Extract actual min, max, avg from the packaged record
        uint16_t actual_min = record.power_collection[core].power_element.min_mW;
        uint16_t actual_max = record.power_collection[core].power_element.max_mW;
        uint16_t actual_avg = record.power_collection[core].power_element.average_mW;

        if (g_print_logs)
        {
            printf("Core %d: expected_power=%d, actual_power=%d\n", core, expected_power[core], core_rt[core].latest_power_mW);
            printf("MOCK_PWR=%d, CORE_POWER_MW_PER_BIT=%d, expected=%d, actual=%d\n",
                   test_configs[core][NUM_ENTRIES - 1].pwr,
                   CORE_POWER_MW_PER_BIT,
                   expected_power[core],
                   core_rt[core].latest_power_mW);

            printf("Core %d: record min_mW=%d, max_mW=%d, average_mW=%d\n", core, actual_min, actual_max, actual_avg);

            if (actual_avg != expected_avg[core])
            {
                printf("[CHECK] Core %d: expected_avg=%d, actual_avg=%d\n", core, expected_avg[core], actual_avg);
                printf("ASSERT FAIL: Core %d AVG expected=%d, actual=%d\n", core, expected_avg[core], actual_avg);
            }
            if (actual_min != expected_min[core])
            {
                printf("[CHECK] Core %d: expected_min=%d, actual_min=%d\n", core, expected_min[core], actual_min);
                printf("ASSERT FAIL: Core %d MIN expected=%d, actual=%d\n", core, expected_min[core], actual_min);
            }
            if (actual_max != expected_max[core])
            {
                printf("[CHECK] Core %d: expected_max=%d, actual_max=%d\n", core, expected_max[core], actual_max);
                printf("ASSERT FAIL: Core %d MAX expected=%d, actual=%d\n", core, expected_max[core], actual_max);
            }
        }

        // Real asserts (always run, not gated by print_logs)
        // Allow minimal rounding tolerance for average (expected or expected+1)
        assert_in_range(actual_avg, expected_avg[core], expected_avg[core] + 1);
        assert_int_equal(expected_min[core], actual_min);
        assert_int_equal(expected_max[core], actual_max);
    }

    // Print test end
    {
        printf("TEST END: test_multi_entry_multi_core_power\n");
    }
}
