// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file test_inst_die_temp_sensors_integration.cpp
 * [IB Telemetry] Log Instantaneous Die Temperature Sensors Integration Test
 *
 * Test Flow:
 * 1. Create test data set for die temperature sensors (unique values)
 * 2. Call data_smpl_process_pvt_temperature_sensor_fifo() to generate metrics
 * 3. Call package_create_pwr_soc_sensor_temp_record() to create the record
 * 4. Verify record contents match expected result
 */
// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2889385

#include "sensor_fifo_service.h" // For soc_pvt_temp_t
#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include <FpFwCMocka.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <package_creation_i.h>
}

#define NUM_DIE_TEMP_SENSORS      4
#define CELSIUS_TO_DECICELSIUS(c) ((c) * 10)

// Debug flag for controlling prints
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

static void setup_die_temp_fifo(int temps_C[NUM_DIE_TEMP_SENSORS])
{
    static soc_pvt_temp_t pvt_temp_entry;
    memset(&pvt_temp_entry, 0, sizeof(soc_pvt_temp_t));
    for (int i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
    {
        pvt_temp_entry.sensor_temp_dC[i] = CELSIUS_TO_DECICELSIUS(temps_C[i]);
    }
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, true);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_entry);
}
TEST_FUNCTION(test_log_instantaneous_die_temperature_sensors_integration, test_setup, test_teardown)
// === Test Flow ===
// 1. Create test data set for die temperature sensors (unique values)
// 2. Call data_proc_tlm_cmpnt_process_input_data() to generate metrics
// 3. Call package_create_inst_soc_sensor_temp_record() to create the record
// 4. Verify record contents match expected result
{
    if (g_print_debug)
    {
        printf("\n==============================\n");
        printf("START TEST: Instantaneous Die Temperature Sensors Integration\n");
        printf("This test validates telemetry for all die temperature sensors across multiple scenarios.\n");
        printf("==============================\n\n");
    }
    int i;
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
        sensor_fifo_is_empty_mock[i] = true;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_PVT_TEMP_FW] = false;

    // Scenario 1: Unique positive values for each sensor
    if (g_print_debug)
    {
        printf("\n=== Scenario 1: Unique positive values ===\n");
        printf("Test: Sensors set to 50C, 51C, 52C, 53C. Expect outputs to match deci-Celsius conversion.\n");
    }
    {
        int die_temp_init[NUM_DIE_TEMP_SENSORS] = {50, 51, 52, 53};
        setup_die_temp_fifo(die_temp_init);
        will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
        data_proc_tlm_cmpnt_process_input_data();
        inst_soc_record_die_temp_t record;
        package_create_inst_soc_sensor_temp_record(&record);
        for (i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
        {
            int actual = record.temperature_collection[i].temperature_element.temperature_dC;
            int expected = CELSIUS_TO_DECICELSIUS(die_temp_init[i]);
            if (g_print_debug)
                printf("[SC1] Sensor %d: input_init=%dC, expected=%d, actual=%d\n", i, die_temp_init[i], expected, actual);
            assert_int_equal((uint16_t)actual, (uint16_t)expected);
        }
    }

    // Scenario 2: Faulty sensors (negative and zero)
    if (g_print_debug)
    {
        printf("\n=== Scenario 2: Mixed zero, positive, negative ===\n");
        printf("Test: Sensors set to 0C, -10C, 25C, -20C. Negative values will be stored as unsigned.\n");
    }
    {
        int die_temp_init[NUM_DIE_TEMP_SENSORS] = {0, -10, 25, -20};
        setup_die_temp_fifo(die_temp_init);
        will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
        data_proc_tlm_cmpnt_process_input_data();
        inst_soc_record_die_temp_t record;
        package_create_inst_soc_sensor_temp_record(&record);
        for (i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
        {
            int actual = record.temperature_collection[i].temperature_element.temperature_dC;
            int expected = CELSIUS_TO_DECICELSIUS(die_temp_init[i]);
            if (g_print_debug)
                printf("[SC2] Sensor %d: input_init=%dC, expected=%u, actual=%u\n",
                       i,
                       die_temp_init[i],
                       (uint16_t)expected,
                       (uint16_t)actual);
            assert_int_equal((uint16_t)actual, (uint16_t)expected);
        }
    }

    // Scenario 3: Boundary values (min/max)
    if (g_print_debug)
    {
        printf("\n=== Scenario 3: Boundary values (min/max/negative) ===\n");
        printf("Test: Sensors set to -40C, 0C, 100C, -100C. Check unsigned representation for negatives.\n");
    }
    {
        int die_temp_init[NUM_DIE_TEMP_SENSORS] = {-40, 0, 100, -100};
        setup_die_temp_fifo(die_temp_init);
        will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
        data_proc_tlm_cmpnt_process_input_data();
        inst_soc_record_die_temp_t record;
        package_create_inst_soc_sensor_temp_record(&record);
        for (i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
        {
            int actual = record.temperature_collection[i].temperature_element.temperature_dC;
            int expected = CELSIUS_TO_DECICELSIUS(die_temp_init[i]);
            if (g_print_debug)
                printf("[SC3] Sensor %d: input_init=%dC, expected=%u, actual=%u\n",
                       i,
                       die_temp_init[i],
                       (uint16_t)expected,
                       (uint16_t)actual);
            assert_int_equal((uint16_t)actual, (uint16_t)expected);
        }
    }

    // Scenario 4: Single sensor fault (out-of-range)
    if (g_print_debug)
    {
        printf("\n=== Scenario 4: Out-of-range and negative fault ===\n");
        printf(
            "Test: Sensors set to 85C, 999C, -999C, 95C. 999C and -999C are out-of-range, check handling.\n");
    }
    {
        int die_temp_init[NUM_DIE_TEMP_SENSORS] = {85, 999, -999, 95};
        setup_die_temp_fifo(die_temp_init);
        will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
        data_proc_tlm_cmpnt_process_input_data();
        inst_soc_record_die_temp_t record;
        package_create_inst_soc_sensor_temp_record(&record);
        for (i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
        {
            int actual = record.temperature_collection[i].temperature_element.temperature_dC;
            int expected = CELSIUS_TO_DECICELSIUS(die_temp_init[i]);
            if (g_print_debug)
                printf("[SC4] Sensor %d: input_init=%dC, expected=%d, actual=%d\n", i, die_temp_init[i], expected, actual);
            assert_int_equal((uint16_t)actual, (uint16_t)expected);
        }
    }

    // Scenario 5: Negative temperature edge case
    if (g_print_debug)
    {
        printf("\n=== Scenario 5: Extreme negative and positive values ===\n");
        printf(
            "Test: Sensors set to -40C, -400C, 123C, -123C. Expect unsigned representation for negatives.\n");
    }
    {
        int die_temp_init[NUM_DIE_TEMP_SENSORS] = {-40, -400, 123, -123};
        setup_die_temp_fifo(die_temp_init);
        will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
        data_proc_tlm_cmpnt_process_input_data();
        inst_soc_record_die_temp_t record;
        package_create_inst_soc_sensor_temp_record(&record);
        for (i = 0; i < NUM_DIE_TEMP_SENSORS; ++i)
        {
            int expected = CELSIUS_TO_DECICELSIUS(die_temp_init[i]);
            uint16_t expected_unsigned = (uint16_t)expected;
            uint16_t actual = (uint16_t)record.temperature_collection[i].temperature_element.temperature_dC;
            if (g_print_debug)
                printf("[SC5] Sensor %d: input_init=%dC, expected=%u, actual=%u\n", i, die_temp_init[i], expected_unsigned, actual);
            assert_int_equal((uint16_t)actual, (uint16_t)expected_unsigned);
        }
    }

    // Print test end banner after all scenarios
    if (g_print_debug)
    {
        printf("--- END test_log_instantaneous_die_temperature_sensors_integration ---\n");
        printf("***\n");
    }
}
