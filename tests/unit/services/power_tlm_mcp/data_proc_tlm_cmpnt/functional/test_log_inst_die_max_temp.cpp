// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file test_log_inst_die_max_temp.cpp
 * Functional test for Instantaneous SOC Die Maximum Temperature Telemetry metrics
 *
 * Test Flow:
 * 1. Create test data set for tile temp and soc top temp (die0)
 * 2. Mock die1 value via die2die exchange
 * 3. Call data_proc_tlm_cmpnt_process_input_data() with the appropriate data for SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW and SENSOR_FIFO_PVT_TEMP_FW
 * 4. Call package_create_inst_soc_max_temp_record to create the record
 * 5. Validate the fields are the expected max values
 *
 * Each die is reported independently; instantaneous record reports the absolute latest max value between tile temp and soc top temp.
 */
// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779240

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <data_sampling_i.h>
#include <sensor_fifo_service.h> // soc_pvt_temp_t definition
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tlm_fuses.h>

extern "C" {
#include <FpFwCMocka.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <die_2_die_exchange_i.h>
#include <package_creation_i.h>
}

// Debug flag for controlling prints (set to false to disable debug output)
bool g_print_debug = true;

// Helper constants and macros
#define NUM_TILE_ENTRIES          3
#define NUM_TOP_SENSORS           2
#define CELSIUS_TO_DECICELSIUS(c) ((c) * 10)

// === Helper Functions ===
static void setup_tile_temp_fifo(int tile_temps_C[NUM_TILE_ENTRIES])
{
    static tile_temp_t temp_entries[NUM_TILE_ENTRIES];
    for (int entry = 0; entry < NUM_TILE_ENTRIES; ++entry)
    {
        memset(&temp_entries[entry], 0, sizeof(tile_temp_t));
        temp_entries[entry].temp0.max_temp =
            TLM_FUSE_TEMP_CEL_2_DOUT(tile_temps_C[entry], DEFAULT_DTS_FUSED_K_VAL, DEFAULT_DTS_FUSED_Y_VAL);
        if (g_print_debug)
        {
            printf("DEBUG: setup_tile_temp_fifo: entry=%d, tile_temp_C=%d\n", entry, tile_temps_C[entry]);
        }
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, 0);
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, (entry < NUM_TILE_ENTRIES - 1));
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &temp_entries[entry]);
    }
}

static void setup_soc_top_temp_fifo(int soc_top_temps_C[NUM_TOP_SENSORS])
{
    static soc_pvt_temp_t pvt_temp_entry;
    memset(&pvt_temp_entry, 0, sizeof(soc_pvt_temp_t));
    int max_soc_top_temp_C = soc_top_temps_C[0];
    for (int i = 1; i < NUM_TOP_SENSORS; ++i)
    {
        if (soc_top_temps_C[i] > max_soc_top_temp_C)
            max_soc_top_temp_C = soc_top_temps_C[i];
    }
    int peak_soc_top_temp_dC = CELSIUS_TO_DECICELSIUS(max_soc_top_temp_C);
    for (int i = 0; i < NUMBER_OF_SOC_TEMP_SENSORS; ++i)
    {
        pvt_temp_entry.sensor_temp_dC[i] = peak_soc_top_temp_dC;
    }
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, true);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_entry);
}

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

typedef struct
{
    int tile_temps_C[NUM_TILE_ENTRIES];
    int soc_top_temps_C[NUM_TOP_SENSORS];
    int die1_max_temp_C;
    int expected_die0_max_dC;
    int expected_die1_max_dC;
    const char* step_desc;
} die_max_temp_test_step_t;

static void run_die_max_temp_test_step(const die_max_temp_test_step_t* step)
{
    inst_soc_record_max_temp_t record = {{0}};
    bool sensor_fifo_is_empty_mock[SENSOR_FIFO_MAX_ID];
    for (int i = 0; i < SENSOR_FIFO_MAX_ID; ++i)
        sensor_fifo_is_empty_mock[i] = true;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW] = false;
    sensor_fifo_is_empty_mock[SENSOR_FIFO_PVT_TEMP_FW] = false;
    setup_tile_temp_fifo((int*)step->tile_temps_C);
    setup_soc_top_temp_fifo((int*)step->soc_top_temps_C);
    will_return(__wrap_sensor_fifo_svc_is_empty, &sensor_fifo_is_empty_mock);
    will_return(__wrap_die_2_die_exch_ib_read_inst_max_die_temp_dC, CELSIUS_TO_DECICELSIUS(step->die1_max_temp_C));
    data_proc_tlm_cmpnt_process_input_data();
    package_create_inst_soc_max_temp_record(&record);
    if (g_print_debug)
    {
        printf("\n=== Die Max Temp Test: %s ===\n", step->step_desc);
        printf("Inputs: tile_temps_C = [%d, %d, %d], soc_top_temps_C = [%d, %d], die1 = %d\n",
               step->tile_temps_C[0],
               step->tile_temps_C[1],
               step->tile_temps_C[2],
               step->soc_top_temps_C[0],
               step->soc_top_temps_C[1],
               step->die1_max_temp_C);
        printf("RESULT: die0_max = %d dC, die1_max = %d dC\n",
               record.temperature_collection.temperature_element.die0_max_temperature_dC,
               record.temperature_collection.temperature_element.die1_max_temperature_dC);
        printf("EXPECT: die0_max = %d dC, die1_max = %d dC\n", step->expected_die0_max_dC, step->expected_die1_max_dC);
    }
    assert_int_equal(record.temperature_collection.temperature_element.die0_max_temperature_dC, step->expected_die0_max_dC);
    assert_int_equal(record.temperature_collection.temperature_element.die1_max_temperature_dC, step->expected_die1_max_dC);
}

TEST_FUNCTION(test_log_inst_die_max_temp_freshness_and_die_independence, test_setup, test_teardown)
{
    die_max_temp_test_step_t steps[4];

    // Step 1 (Initial batch): Only tile2 is the peak, all others unique
    steps[0].tile_temps_C[0] = 11;                              // tile0 (unique low)
    steps[0].tile_temps_C[1] = 12;                              // tile1 (unique low)
    steps[0].tile_temps_C[2] = 83;                              // tile2 (peak)
    steps[0].soc_top_temps_C[0] = 13;                           // soc_top0 (unique low)
    steps[0].soc_top_temps_C[1] = 14;                           // soc_top1 (unique low)
    steps[0].die1_max_temp_C = 61;                              // die1 input (unique low)
    steps[0].expected_die0_max_dC = CELSIUS_TO_DECICELSIUS(83); // expect die0 max (tile2)
    steps[0].expected_die1_max_dC = CELSIUS_TO_DECICELSIUS(61); // expect die1 max
    steps[0].step_desc = "Step 1 (Initial batch, tile2 peak, all unique)";

    // Step 2 (Update die0): Only soc_top0 is the peak, all others unique
    steps[1].tile_temps_C[0] = 21;                              // tile0 (unique low)
    steps[1].tile_temps_C[1] = 22;                              // tile1 (unique low)
    steps[1].tile_temps_C[2] = 23;                              // tile2 (unique low)
    steps[1].soc_top_temps_C[0] = 94;                           // soc_top0 (peak)
    steps[1].soc_top_temps_C[1] = 24;                           // soc_top1 (unique low)
    steps[1].die1_max_temp_C = 62;                              // die1 input (unique low)
    steps[1].expected_die0_max_dC = CELSIUS_TO_DECICELSIUS(94); // expect die0 max (soc_top0)
    steps[1].expected_die1_max_dC = CELSIUS_TO_DECICELSIUS(62); // expect die1 max
    steps[1].step_desc = "Step 2 (Update die0, soc_top0 peak, all unique)";

    // Step 3 (Update die1): Only die1 is the peak, all others unique
    steps[2].tile_temps_C[0] = 31;                              // tile0 (unique low)
    steps[2].tile_temps_C[1] = 32;                              // tile1 (unique low)
    steps[2].tile_temps_C[2] = 33;                              // tile2 (unique low)
    steps[2].soc_top_temps_C[0] = 34;                           // soc_top0 (unique low)
    steps[2].soc_top_temps_C[1] = 35;                           // soc_top1 (unique low)
    steps[2].die1_max_temp_C = 99;                              // die1 input (peak)
    steps[2].expected_die0_max_dC = CELSIUS_TO_DECICELSIUS(35); // expect die0 max (highest of non-peak)
    steps[2].expected_die1_max_dC = CELSIUS_TO_DECICELSIUS(99); // expect die1 max
    steps[2].step_desc = "Step 3 (Update die1, die1 peak, all unique)";

    // Step 4 (Update die0 again): Only tile2 is the peak, all others unique
    steps[3].tile_temps_C[0] = 41;                               // tile0 (unique low)
    steps[3].tile_temps_C[1] = 42;                               // tile1 (unique low)
    steps[3].tile_temps_C[2] = 103;                              // tile2 (peak)
    steps[3].soc_top_temps_C[0] = 43;                            // soc_top0 (unique low)
    steps[3].soc_top_temps_C[1] = 44;                            // soc_top1 (unique low)
    steps[3].die1_max_temp_C = 98;                               // die1 input (unique low)
    steps[3].expected_die0_max_dC = CELSIUS_TO_DECICELSIUS(103); // expect die0 max (tile2)
    steps[3].expected_die1_max_dC = CELSIUS_TO_DECICELSIUS(98);  // expect die1 max
    steps[3].step_desc = "Step 4 (Update die0 again, tile2 peak, all unique)";
    for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i)
    {
        run_die_max_temp_test_step(&steps[i]);
    }
}