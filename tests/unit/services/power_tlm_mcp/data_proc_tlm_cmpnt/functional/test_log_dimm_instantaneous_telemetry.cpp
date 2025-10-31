// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file test_log_dimm_instantaneous_telemetry.cpp
 * @brief Functional test for DIMM instantaneous telemetry metrics.
 *
 * SSI_Unit_Test: Telemetry_Dimm_Instantaneous
 *
 * This test validates that the DIMM instantaneous telemetry record always reports the last read sensor data for each DIMM, with no computation or aggregation.
 *
 * Test Flow:
 * 1. Clear Data
 * 2. For each DIMM (up to 6):
 *    - Feed unique sensor data (s0>s1 and s1>s0 cases)
 *    - (Optionally) Call data_smpl_parse_dimm_entry() per DIMM (covered by process function)
 * 3. Call package_create_inst_soc_dimm_runtime_record() to create the record
 * 4. Validate that all record fields match the last data read for each DIMM ("last value wins")
 *
 * This test is self-contained, with clear header dependencies and maintainable structure for robust telemetry validation and traceability.
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779240

#include "telemetry_functional.h" // Test harness and common test utilities

#include <CMockaWrapper.h>       // CMocka test macros
#include <data_sampling_i.h>     // Telemetry data sampling interfaces
#include <sensor_fifo_service.h> // DIMM sensor data structures
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tlm_fuses.h>
extern "C" {
#include <FpFwCMocka.h>        // CMocka FpFw hooks
#include <compute_metrics_i.h> // Telemetry compute metrics (if needed)
}

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0, false); // die_id=0, is_single_die=false
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// This test emphasizes that DIMM instantaneous telemetry records report the last read sensor data for each
// DIMM. No computation or aggregation is performed; the record simply reflects the most recent values
// provided. The test validates that the packaged record matches exactly the last data read for each DIMM.
TEST_FUNCTION(test_log_dimm_instantaneous_telemetry, test_setup, test_teardown)
{
    // --- Case 1: s0 > s1 ---
    // Feed unique data for each DIMM and verify the record matches the last read values exactly.
    bool print = false;
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
        sensor_fifo_is_empty_mock[i] = true;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_DIMM_TEMP_FW] = false;
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);

    sensor_ram_dimm_info_t dimm_infos[NUMBER_OF_DIMMS];
    memset(dimm_infos, 0, sizeof(dimm_infos));
    for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
    {
        dimm_infos[i].dimm_id = i;
        dimm_infos[i].dimm_temp_s0_dC = 70 + i;
        dimm_infos[i].dimm_temp_s1_dC = 60 + i;
        dimm_infos[i].dimm_power_mW = 1000 + 10 * i;
        dimm_infos[i].dimm_memory_frequency_id = i;
        dimm_infos[i].dimm_throttling = i % 2;
        // Only one entry per poll, so mock poller for each dimm
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, (i < NUMBER_OF_DIMMS - 1) ? true : false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_infos[i]);
    }

    data_proc_tlm_cmpnt_process_input_data();

    inst_soc_record_dimm_runtime_t record = {{0}};
    package_create_inst_soc_dimm_runtime_record(&record);
    // Optionally print record for debug
    if (print)
    {
        printf("DIMM Record (Case 1):\n");
        for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
        {
            printf("  DIMM %d: temp=%d, power=%d, freq=%d, throttle=%d\n",
                   i,
                   record.dimm_collection[i].dimm_rt_element.temperature_dC,
                   record.dimm_collection[i].dimm_rt_element.power_mW,
                   record.dimm_collection[i].dimm_rt_element.memory_freq_id,
                   record.dimm_collection[i].dimm_rt_element.throttle_source);
        }
    }
    // The record must match the last data read for each DIMM (no computation, just last value wins)
    for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
    {
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.temperature_dC, 70 + i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.power_mW, 1000 + 10 * i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.memory_freq_id, i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.throttle_source, i % 2);
    }
    // --- Case 2: s1 > s0 ---
    // Feed a different set of unique data for each DIMM (s1 > s0 scenario) and verify the record matches the last read values exactly.
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
        sensor_fifo_is_empty_mock[i] = true;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_DIMM_TEMP_FW] = false;
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);

    memset(dimm_infos, 0, sizeof(dimm_infos));
    for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
    {
        dimm_infos[i].dimm_id = i;
        dimm_infos[i].dimm_temp_s0_dC = 160 + i; // different range from first scenario
        dimm_infos[i].dimm_temp_s1_dC = 170 + i;
        dimm_infos[i].dimm_power_mW = 3000 + 20 * i;
        dimm_infos[i].dimm_memory_frequency_id = 100 + i;
        dimm_infos[i].dimm_throttling = (i + 2) % 2;
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, true);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, (i < NUMBER_OF_DIMMS - 1) ? true : false);
        will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_infos[i]);
    }

    data_proc_tlm_cmpnt_process_input_data();

    memset(&record, 0, sizeof(record));
    package_create_inst_soc_dimm_runtime_record(&record);
    // Optionally print record for debug
    if (print)
    {
        printf("DIMM Record (Case 2):\n");
        for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
        {
            printf("  DIMM %d: temp=%d, power=%d, freq=%d, throttle=%d\n",
                   i,
                   record.dimm_collection[i].dimm_rt_element.temperature_dC,
                   record.dimm_collection[i].dimm_rt_element.power_mW,
                   record.dimm_collection[i].dimm_rt_element.memory_freq_id,
                   record.dimm_collection[i].dimm_rt_element.throttle_source);
        }
    }
    // The record must match the last data read for each DIMM (no computation, just last value wins)
    for (int i = 0; i < NUMBER_OF_DIMMS; ++i)
    {
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.temperature_dC, 170 + i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.power_mW, 3000 + 20 * i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.memory_freq_id, 100 + i);
        assert_int_equal(record.dimm_collection[i].dimm_rt_element.throttle_source, (i + 2) % 2);
    }
}