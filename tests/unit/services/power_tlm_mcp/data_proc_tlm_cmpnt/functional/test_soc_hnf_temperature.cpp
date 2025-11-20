//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file test_soc_hnf_temperature.cpp
 * Functional test for SOC HNF temperature telemetry metrics
 *
 * Test Flow:
 * ----------
 * 1. Calculate expected min, max, and average values for each HNF channel
 *    using a fixed set of mock temperature samples (see hnf_temps).
 * 2. For each entry in the mock data:
 *    - Prepare a tile_temp_t structure with valid temperature data for two HNF channels.
 *    - Set up CMocka will_return calls to mock the sensor FIFO polling.
 * 3. Call data_smpl_process_tile_temperature_sensor_fifo() to process all mock FIFO entries.
 *    (Note: This function is used instead of data_smpl_parse_tile_temperature_entry because it processes
 *    the entire FIFO and simulates the real pipeline behavior, ensuring all entries are handled as in production.)
 * 4. Call package_create_pwr_soc_hnf_record() to package the computed HNF metrics.
 * 5. For each HNF channel:
 *    - Compare the actual min, max, and average values in the record to the expected values,
 *      using assert_int_equal for strict checking.
 *    - (Optional) Print debug information if g_print_debug is enabled.
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779233

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tlm_fuses.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <fpfw_status.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

// === Constants and Macros ===
#define NUM_HNF_CHANNELS 2
#define NUM_ENTRIES      3

#define CELSIUS_TO_DECI_CELSIUS(temp)          ((temp) * 10) // Convert Celsius to deciCelsius
#define ROUND_UP_CELSIUS_TO_DECI_CELSIUS(temp) ((uint16_t)((((temp) * 10.0) + 0.5)))
#ifndef TEST_TEMP_CEL_2_DOUT
    #define TEST_TEMP_CEL_2_DOUT(temp) \
        (TLM_FUSE_TEMP_CEL_2_DOUT((temp), DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL))
#endif

// === Test Data ===

// Debug flag for controlling prints
static bool g_print_debug = true;
static const int hnf_temps[NUM_ENTRIES][NUM_HNF_CHANNELS] = {{70, 75}, {80, 65}, {60, 85}};

// === Helper Functions ===
static void calc_min_max_sum(const int temps[][NUM_HNF_CHANNELS], int num_entries, int* min, int* max, int* sum)
{
    for (int ch = 0; ch < NUM_HNF_CHANNELS; ++ch)
    {
        min[ch] = INT_MAX;
        max[ch] = INT_MIN;
        sum[ch] = 0;
    }
    for (int entry = 0; entry < num_entries; ++entry)
    {
        for (int ch = 0; ch < NUM_HNF_CHANNELS; ++ch)
        {
            int val = temps[entry][ch];
            if (val < min[ch])
                min[ch] = val;
            if (val > max[ch])
                max[ch] = val;
            sum[ch] += val;
        }
    }
}

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0, false, 0); // die_id=0, is_single_die=false, mpam_vm_mem_fixed_pwr_mW=0
    //  enable all cores for these tests
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        core_is_active[core] = true;
    }
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

TEST_FUNCTION(test_soc_hnf_temperature_telemetry, test_setup, test_teardown)
{
    // === Test Start ===
    printf("\n=== SOC HNF Temperature Telemetry Test (Multi-Channel, Multi-Entry) ===\n");

    // === Calculate expected min, max, sum ===
    int expected_min[NUM_HNF_CHANNELS], expected_max[NUM_HNF_CHANNELS], sum[NUM_HNF_CHANNELS];
    calc_min_max_sum(hnf_temps, NUM_ENTRIES, expected_min, expected_max, sum);

    // === Prepare and inject mock FIFO data ===
    tile_temp_t test_fifo[NUM_ENTRIES];
    memset(test_fifo, 0, sizeof(test_fifo));
    for (int entry = 0; entry < NUM_ENTRIES; ++entry)
    {
        test_fifo[entry].temp0.temp_valid = 1;
        test_fifo[entry].temp2.temp6 = TEST_TEMP_CEL_2_DOUT(hnf_temps[entry][0]);
        test_fifo[entry].temp2.temp7 = TEST_TEMP_CEL_2_DOUT(hnf_temps[entry][1]);

        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);    // tile_index
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, (entry < NUM_ENTRIES - 1)); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &test_fifo[entry]); // pointer to data
    }

    // === Execute test logic ===
    // Use the full FIFO processing function to accurately simulate production pipeline behavior;
    // this ensures all FIFO entries are handled as in real operation (single-entry parsing is insufficient).
    data_smpl_process_tile_temperature_sensor_fifo();
    pwr_soc_record_hnf_t hnf_record;
    package_create_pwr_soc_hnf_record(&hnf_record);

    // === Verification and Logging ===
    for (int ch = 0; ch < NUM_HNF_CHANNELS; ++ch)
    {
        uint16_t actual_min = hnf_record.hnf_collection[ch].hnf_element.min_dC;
        uint16_t actual_max = hnf_record.hnf_collection[ch].hnf_element.max_dC;
        uint16_t actual_avg = hnf_record.hnf_collection[ch].hnf_element.average_dC;

        if (g_print_debug)
        {
            printf("HNF Channel %d: expected_min=%d, actual_min=%u\n",
                   ch,
                   ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_min[ch]),
                   actual_min);
            printf("HNF Channel %d: expected_max=%d, actual_max=%u\n",
                   ch,
                   ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_max[ch]),
                   actual_max);
            printf("HNF Channel %d: expected_avg=%d, actual_avg=%u\n",
                   ch,
                   ROUND_UP_CELSIUS_TO_DECI_CELSIUS(sum[ch] / NUM_ENTRIES),
                   actual_avg);
        }

        if (g_print_debug)
        {
            if (actual_avg != ROUND_UP_CELSIUS_TO_DECI_CELSIUS(sum[ch] / NUM_ENTRIES))
            {
                printf("[CHECK] HNF Channel %d: expected_avg=%d, actual_avg=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(sum[ch] / NUM_ENTRIES),
                       actual_avg);
                printf("ASSERT FAIL: HNF Channel %d AVG expected=%d, actual=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(sum[ch] / NUM_ENTRIES),
                       actual_avg);
            }
            if (actual_min != ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_min[ch]))
            {
                printf("[CHECK] HNF Channel %d: expected_min=%d, actual_min=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_min[ch]),
                       actual_min);
                printf("ASSERT FAIL: HNF Channel %d MIN expected=%d, actual=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_min[ch]),
                       actual_min);
            }
            if (actual_max != ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_max[ch]))
            {
                printf("[CHECK] HNF Channel %d: expected_max=%d, actual_max=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_max[ch]),
                       actual_max);
                printf("ASSERT FAIL: HNF Channel %d MAX expected=%d, actual=%u\n",
                       ch,
                       ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_max[ch]),
                       actual_max);
            }
        }

        assert_int_equal(actual_min, ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_min[ch]));
        assert_int_equal(actual_max, ROUND_UP_CELSIUS_TO_DECI_CELSIUS(expected_max[ch]));
        assert_int_equal(actual_avg, ROUND_UP_CELSIUS_TO_DECI_CELSIUS(sum[ch] / NUM_ENTRIES));
    }

    // === Test End ===
    printf("TEST END: test_soc_hnf_temperature_telemetry\n");
}
