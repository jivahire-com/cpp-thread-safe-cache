
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file test_soc_max_temperature.cpp
 * Functional test for SOC Max Temperature Telemetry metrics
 *
 * Test Flow:
 * ----------
 * 1. Create a data set of inputs for max soc temp (tile temp, soc top temp, die1 summary).
 * 2. Feed die0 data via SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW and SENSOR_FIFO_PVT_TEMP_FW.
 * 3. Feed die1 data via die_2_die_exch_ib_write_inst_max_die_temp().
 * 4. Call data_proc_tlm_cmpnt_process_input_data() to populate compute metrics.
 * 5. Call package_create_pwr_soc_max_temp_record() to create the record.
 * 6. Verify the record matches expected values for peak max temp.
 *
 * Test Cases:
 *   - Peak temp from tile temp
 *   - Peak temp from soc top temp
 *   - Peak temp from die1
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2889373

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tlm_fuses.h>
// Needed for soc_runtime_info_t definition and soc_rt extern
#include <data_sampling_i.h>
extern "C" {
#include <FpFwCMocka.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <die_2_die_exchange_i.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
}

#define NUM_TILE_ENTRIES          3
#define NUM_TOP_SENSORS           2
#define CELSIUS_TO_DECICELSIUS(c) ((c) * 10)

// Debug flag for controlling prints
static bool g_print_debug = true;

// === Test Setup/Teardown ===
static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    data_proc_tlm_cmpnt_init(0);
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
        core_is_active[core] = true;
    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

// === Helper Functions ===
static void setup_tile_temp_fifo(int temps_C[NUM_TILE_ENTRIES])
{
    static tile_temp_t temp_entries[NUM_TILE_ENTRIES];
    for (int entry = 0; entry < NUM_TILE_ENTRIES; ++entry)
    {
        memset(&temp_entries[entry], 0, sizeof(tile_temp_t));
        temp_entries[entry].temp0.temp_valid = 1;
        temp_entries[entry].temp0.max_temp =
            TLM_FUSE_TEMP_CEL_2_DOUT(temps_C[entry], DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL);
        temp_entries[entry].temp0.max_id = entry;
        // CMocka FIFO polling mocks
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, (entry < NUM_TILE_ENTRIES - 1));
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_entries[entry]);
    }
}

static void setup_soc_top_temp_fifo(int temps_C[NUM_TOP_SENSORS])
{
    static soc_pvt_temp_t pvt_temp_entry;
    memset(&pvt_temp_entry, 0, sizeof(soc_pvt_temp_t));
    int max_temp_C = temps_C[0];
    for (int i = 1; i < NUM_TOP_SENSORS; ++i)
    {
        if (temps_C[i] > max_temp_C)
            max_temp_C = temps_C[i];
    }
    int peak_dC = CELSIUS_TO_DECICELSIUS(max_temp_C); // Convert peak to deci-Celsius
    for (int i = 0; i < NUMBER_OF_SOC_TEMP_SENSORS; ++i)
    {
        pvt_temp_entry.sensor_temp_dC[i] = peak_dC;
    }
    // CMocka SOC top sensor polling mocks
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, true);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_entry);
}

static void setup_die1_summary(int die1_max_temp)
{
    die_2_die_exch_ib_write_inst_max_die_temp(die1_max_temp);
}

// === Test Cases ===

TEST_FUNCTION(test_soc_max_temperature_tile_peak, test_setup, test_teardown)
{
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
    {
        sensor_fifo_is_empty_mock[i] = true;
    }
    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = false;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_PVT_TEMP_FW] = false;
    if (g_print_debug)
    {
        printf("\n=== SOC Max Temperature Test: Tile Peak ===\n");
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(90) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(90, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(85) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(85, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
    }
    // ...existing code...
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);

    // --- Setup: Inputs ---
    int tile_temps_C[NUM_TILE_ENTRIES] = {71, 82, 93};      // Unique values for tile temp (Celsius)
    int soc_top_temps_C[NUM_TOP_SENSORS] = {61, 62};        // Unique values for soc top temp (Celsius)
    int expected_die0_peak_dC = CELSIUS_TO_DECICELSIUS(93); // tile peak, scaled (deci-Celsius)
    int expected_die1_peak_dC = CELSIUS_TO_DECICELSIUS(80); // die1 value, scaled (deci-Celsius)

    setup_tile_temp_fifo(tile_temps_C);
    setup_soc_top_temp_fifo(soc_top_temps_C);

    // --- Execution ---
    data_proc_tlm_cmpnt_process_input_data();
    setup_die1_summary(expected_die1_peak_dC);

    // --- Verification ---
    extern soc_runtime_info_t soc_rt;
    if (g_print_debug)
    {
        printf("DEBUG: soc_rt.latest_max_tile_temp_dC = 0x%x\n", soc_rt.latest_max_tile_temp_dC);
        printf("DEBUG: soc_rt.latest_max_soc_top_temp_dC = 0x%x\n", soc_rt.latest_max_soc_top_temp_dC);
    }
    inst_soc_record_max_temp_t max_temp_record;
    package_create_inst_soc_max_temp_record(&max_temp_record);
    if (g_print_debug)
    {
        printf("DEBUG: actual die0_max_temperature_dC = 0x%x\n",
               max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC);
    }
    // Expected: die0 = tile peak, die1 = die1 value
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC, expected_die0_peak_dC);
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die1_max_temperature_dC, expected_die1_peak_dC);
}

TEST_FUNCTION(test_soc_max_temperature_top_sensor_peak, test_setup, test_teardown)
{
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
    {
        sensor_fifo_is_empty_mock[i] = true;
    }
    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = false;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_PVT_TEMP_FW] = false;
    if (g_print_debug)
    {
        printf("\n=== SOC Max Temperature Test: Top Sensor Peak ===\n");
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(95) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(95, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(80) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(80, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
    }
    // ...existing code...
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);

    // --- Setup: Inputs ---
    int tile_temps_C[NUM_TILE_ENTRIES] = {17, 18, 19}; // Unique values, much lower than SOC top (Celsius)
    int soc_top_temps_C[NUM_TOP_SENSORS] = {96, 97};   // Unique values, peak from soc top temp (Celsius)
    int expected_die0_peak_dC = CELSIUS_TO_DECICELSIUS(97); // soc top peak, scaled (deci-Celsius)
    int expected_die1_peak_dC = CELSIUS_TO_DECICELSIUS(97); // die1 value, scaled (deci-Celsius)

    setup_tile_temp_fifo(tile_temps_C);
    setup_soc_top_temp_fifo(soc_top_temps_C);

    // --- Execution ---
    data_proc_tlm_cmpnt_process_input_data();
    setup_die1_summary(expected_die1_peak_dC);

    // --- Verification ---
    extern soc_runtime_info_t soc_rt;
    if (g_print_debug)
    {
        printf("DEBUG: soc_rt.latest_max_tile_temp_dC = 0x%x\n", soc_rt.latest_max_tile_temp_dC);
        printf("DEBUG: soc_rt.latest_max_soc_top_temp_dC = 0x%x\n", soc_rt.latest_max_soc_top_temp_dC);
    }
    inst_soc_record_max_temp_t max_temp_record;
    package_create_inst_soc_max_temp_record(&max_temp_record);
    if (g_print_debug)
    {
        printf("DEBUG: actual die0_max_temperature_dC = 0x%x\n",
               max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC);
    }
    // Expected: die0 = soc top peak, die1 = die1 value
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC, expected_die0_peak_dC);
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die1_max_temperature_dC, expected_die1_peak_dC);
}

TEST_FUNCTION(test_soc_max_temperature_die1_peak, test_setup, test_teardown)
{
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
    {
        sensor_fifo_is_empty_mock[i] = true;
    }
    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = false;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_PVT_TEMP_FW] = false;
    if (g_print_debug)
    {
        printf("\n=== SOC Max Temperature Test: Die1 Peak ===\n");
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(85) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(85, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
        printf("DEBUG: TLM_FUSE_TEMP_CEL_2_DOUT(100) = 0x%x\n",
               TLM_FUSE_TEMP_CEL_2_DOUT(100, DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL));
    }
    // ...existing code...
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);

    // --- Setup: Inputs ---
    int tile_temps_C[NUM_TILE_ENTRIES] = {72, 83, 86};       // Unique values, lower than die1 (Celsius)
    int soc_top_temps_C[NUM_TOP_SENSORS] = {81, 82};         // Unique values, lower than die1 (Celsius)
    int expected_die0_peak_dC = CELSIUS_TO_DECICELSIUS(86);  // tile peak, scaled (deci-Celsius)
    int expected_die1_peak_dC = CELSIUS_TO_DECICELSIUS(100); // die1 value, scaled (deci-Celsius)

    setup_tile_temp_fifo(tile_temps_C);
    setup_soc_top_temp_fifo(soc_top_temps_C);

    // --- Execution ---
    data_proc_tlm_cmpnt_process_input_data();
    setup_die1_summary(expected_die1_peak_dC);

    // --- Verification ---
    extern soc_runtime_info_t soc_rt;
    if (g_print_debug)
    {
        printf("DEBUG: soc_rt.latest_max_tile_temp_dC = 0x%x\n", soc_rt.latest_max_tile_temp_dC);
        printf("DEBUG: soc_rt.latest_max_soc_top_temp_dC = 0x%x\n", soc_rt.latest_max_soc_top_temp_dC);
    }
    inst_soc_record_max_temp_t max_temp_record;
    package_create_inst_soc_max_temp_record(&max_temp_record);
    if (g_print_debug)
    {
        printf("DEBUG: actual die0_max_temperature_dC = 0x%x\n",
               max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC);
    }
    // Expected: die0 = tile peak, die1 = die1 value
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die0_max_temperature_dC, expected_die0_peak_dC);
    assert_int_equal(max_temp_record.temperature_collection.temperature_element.die1_max_temperature_dC, expected_die1_peak_dC);
}
