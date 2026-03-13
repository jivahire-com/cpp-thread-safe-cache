//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_data_sampling.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include "data_proc_mock.h"

#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <aging_counters_i.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <dvfs.h>
#include <mcp_telemetry_shared.h> //for cstate_instr_timestamp_t
#include <sensor_fifo_service.h>  // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>               // for uint32_t, uint64_t, int32_t
#include <telemetry_package_defs.h>
#include <tlm_fuses.h>
}

/*-- Symbolic Constant Macros (defines) --*/
// Conversion macro: centi-amps to milliamps (1 cA = 10 mA)
#define CONVERT_CENTIAMPS_TO_MILLIAMPS(ca) ((ca) * 10)

#define TEST_DIE_ID         (1)
#define TEST_M1_ENTRY_COUNT (5)
#define TEST_M2_ENTRY_COUNT (3)
#define TEST_M0_RESIDENCY   (1500000000ULL) // 1.5 seconds worth of 2GHz clock cycles
#define TEST_M1_RESIDENCY   (500000000ULL)  // 0.5 seconds worth of 2GHz clock cycles
#define TEST_M2_RESIDENCY   (100000000ULL)  // 0.1 seconds worth of 2GHz clock cycles
#define TEST_DELIVERED_PERF (2000000000ULL) // 1 second worth of 2GHz clock cycles

extern "C" {
extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern aging_counter_t core_aging[NUMBER_OF_CORES_PER_DIE];
extern soc_runtime_info_t soc_rt;
extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
extern computed_metrics_d2d_2_min_t computed_metrics_d2d_2mins;
extern mpam_data_t mpam_staging[NUMBER_OF_MPAMS];
extern d2dss_runtime_info_t d2dss_rt[NUMBER_OF_D2D_INTERFACES];
extern bool core_is_active[NUMBER_OF_CORES_PER_DIE];
bool test_snsr_fifo_is_empty[SENSOR_FIFO_MAX_ID] = {0};
}

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    for (uint32_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        test_snsr_fifo_is_empty[i] = false;
    }

    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);
    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);
    will_return(__wrap_core_info_get_enable_cores_result, 0xffffffff);

    comp_metrics_init_active_cores();

    in_band_publishing_active = true;
    setup_cstate_tfa_mock_buffer();

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

//
// Tests
//

// Test for data sampling to init dts coefficient structures
TEST_FUNCTION(test_data_smpl_init_dts_coefficients, test_setup, test_teardown)
{
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, count, sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_tlm_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);
    data_smpl_init_dts_coefficients();
}

TEST_FUNCTION(test_data_smpl_init_dts_coefficients_fail, test_setup, test_teardown)
{
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_tlm_fuses_get_dts_coeff_tile, count, sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_tlm_fuses_get_dts_coeff_tile, FPFW_STATUS_UNEXPECTED);
    data_smpl_init_dts_coefficients();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_clear_pwr_tlm_data, test_setup, test_teardown)
{
    core_runtime_info_t zero_core[NUMBER_OF_CORES_PER_DIE] = {{0}};
    tile_runtime_info_t zero_tile[NUMBER_OF_TILES_PER_DIE] = {{0}};
    soc_runtime_info_t zero_soc_rt = {0};
    dimm_runtime_info_t zero_dimm_rt = {{{0}}};

    memset(core_rt, 0xFF, sizeof(core_rt));
    memset(tile_rt, 0xFF, sizeof(tile_rt));
    memset(&soc_rt, 0xFF, sizeof(soc_rt));
    memset(&dimm_rt, 0xFF, sizeof(dimm_rt));

    data_proc_tlm_cmpnt_clear_pwr_tlm_data();

    assert_memory_equal(&core_rt, &zero_core, sizeof(zero_core));
    assert_memory_equal(&tile_rt, &zero_tile, sizeof(zero_tile));
    assert_memory_equal(&soc_rt, &zero_soc_rt, sizeof(zero_soc_rt));
    assert_memory_equal(&dimm_rt, &zero_dimm_rt, sizeof(zero_dimm_rt));
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data, test_setup, test_teardown)
{
    will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

    sensor_ram_poll_status_t no_entries = {.curr_data_is_valid = false, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &no_entries);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &no_entries);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_all_empty, test_setup, test_teardown)
{
    for (uint32_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        test_snsr_fifo_is_empty[i] = true;
    }

    will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_smpl_parse_tile_temperature_entry, test_setup, test_teardown)
{
    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));

    tile_temp_t temperature_data = {
        .timestamp = 0,
        .temp0 =
            {
                .temp_valid = 1,
                .max_id = 7,
                .max_temp = 10, // Very small value to avoid DTS conversion
                .core0 = 5,     // Very small values
                .core1 = 5,
            },
        .temp1 =
            {
                .temp0 = 2, // Very small values to avoid DTS conversion
                .temp1 = 3,
                .temp2 = 4,
                .temp3 = 4,
            },
        .temp2 =
            {
                .temp4 = 3,
                .temp5 = 2,
                .temp6 = 4,
                .temp7 = 10,
            },
    };

    // Test case 1: DTS conversion with small raw values
    uint8_t index = 0;
    tileDtsCoefficients[index].k_val = 281;
    tileDtsCoefficients[index].y_val = 648;

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    // Verify core temperatures were updated with DTS conversion of small values
    // Core 0: max of temp0(2), temp1(3), temp2(4) = 4
    // Converted: (((4 / 16384.0) * 648 - 281) * 10.0) + 0.5 ≈ -2808 → 62728 (uint16_t wraparound)
    assert_int_equal(core_rt[0].latest_max_value_dC, 62729); // 0xF509
    // Core 1: max of temp3(4), temp4(3), temp5(2) = 4
    // Same calculation as core 0
    assert_int_equal(core_rt[1].latest_max_value_dC, 62729); // 0xF509

    // Verify HNF temperatures were updated with DTS conversion
    // HNF 0 (index 0): temp6(4) → same calculation as above = 62729
    assert_int_equal(soc_rt.latest_hnf_max_temp_dC[0], 62729);
    // HNF 1 (index 1): temp7(10) → (((10 / 16384.0) * 648 - 281) * 10.0) + 0.5 ≈ -2805 → 62731
    assert_int_equal(soc_rt.latest_hnf_max_temp_dC[1], 62731);

    // Verify tile temperature was updated with DTS conversion
    // Max tile temp: 10 → (((10 / 16384.0) * 648 - 281) * 10.0) + 0.5 ≈ -2805 → 62731
    assert_int_equal(tile_rt[0].latest_max_temp_dC, 62731);
    assert_int_equal(tile_rt[0].latest_max_temp_sensor_index, 7); // max_id from temp0

    // Clear runtime info before next test case
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 2: DTS temperature conversion with realistic values
    index = 1;
    temperature_data.temp0.max_temp = 16384; // Raw value for DTS conversion
    temperature_data.temp1.temp0 = 8192;     // Core 0 sensor values
    temperature_data.temp1.temp1 = 9216;
    temperature_data.temp1.temp2 = 10240;
    temperature_data.temp1.temp3 = 11264; // Core 1 sensor values
    temperature_data.temp2.temp4 = 12288;
    temperature_data.temp2.temp5 = 13312;
    temperature_data.temp2.temp6 = 14336; // HNF 0 sensor
    temperature_data.temp2.temp7 = 15360; // HNF 1 sensor

    tileDtsCoefficients[index].k_val = 281;
    tileDtsCoefficients[index].y_val = 648;
    soc_rt.latest_max_tile_temp_dC = 0;

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    // Verify max tile temperature was updated with DTS conversion
    // 16384 -> (((16384 / 16384.0) * 648 - 281) * 10.0) + 0.5 = 3670
    assert_int_equal(soc_rt.latest_max_tile_temp_dC, 3670);

    // Verify tile runtime info was updated
    assert_int_equal(tile_rt[index].latest_max_temp_dC, 3670);
    assert_int_equal(tile_rt[index].latest_max_temp_sensor_index, 7); // max_id from temp0

    // Verify core temperature calculations for tile 1 (cores 2 and 3)
    // Core 2: max of temp0(8192), temp1(9216), temp2(10240) = 10240
    // Converted: (((10240 / 16384.0) * 648 - 281) * 10.0) + 0.5 = 1240
    assert_int_equal(core_rt[2].latest_max_value_dC, 1240);

    // Core 3: max of temp3(11264), temp4(12288), temp5(13312) = 13312
    // Converted: (((13312 / 16384.0) * 648 - 281) * 10.0) + 0.5 = 2455
    assert_int_equal(core_rt[3].latest_max_value_dC, 2455);

    // Verify HNF temperature calculations
    // HNF 0 (index 2): temp6(14336) -> (((14336 / 16384.0) * 648 - 281) * 10.0) + 0.5 = 2860
    assert_int_equal(soc_rt.latest_hnf_max_temp_dC[2], 2860);

    // HNF 1 (index 3): temp7(15360) -> (((15360 / 16384.0) * 648 - 281) * 10.0) + 0.5 = 3265
    assert_int_equal(soc_rt.latest_hnf_max_temp_dC[3], 3265);

    // Clear runtime info before next test case
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 3: Test with different DTS coefficients
    index = 0;                               // Back to tile 0
    temperature_data.temp0.max_temp = 12288; // Different raw value
    tileDtsCoefficients[index].k_val = 300;  // Different K coefficient
    tileDtsCoefficients[index].y_val = 600;  // Different Y coefficient

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    // Verify tile temperature calculation with different coefficients
    // 12288 -> (((12288 / 16384.0) * 600 - 300) * 10.0) + 0.5 = 1500
    assert_int_equal(tile_rt[0].latest_max_temp_dC, 1500);

    // Verify soc_rt max temp is updated since 1500 > 0 (the cleared value)
    assert_int_equal(soc_rt.latest_max_tile_temp_dC, 1500);

    // Clear runtime info before next test case
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 4: Test temperature valid flag
    temperature_data.temp0.temp_valid = 0; // Invalid temperature

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    // Verify temperature was still updated even when temp_valid = 0
    // The function still processes max_temp=12288 with K=300, Y=600 → 1500
    assert_int_equal(tile_rt[0].latest_max_temp_dC, 1500);

    // Test case 5: Boundary test - index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    uint16_t soc_temp_before = soc_rt.latest_max_tile_temp_dC;

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    // Verify no changes when index is out of bounds
    assert_int_equal(soc_rt.latest_max_tile_temp_dC, soc_temp_before);
}

TEST_FUNCTION(test_data_smpl_process_tile_temperature_sensor_fifo, test_setup, test_teardown)
{
    will_return_always(__wrap_core_info_get_enable_cores_result, 0x00);
    comp_metrics_init(false); // false = dual die system (24hr metrics enabled)

    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Setup DTS coefficients for temperature conversion
    tileDtsCoefficients[0].k_val = 281;
    tileDtsCoefficients[0].y_val = 648;

    // Test case 1: Process multiple FIFO entries
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    tile_temp_t tile_temp_entry = {
        .timestamp = 1000,
        .temp0 = {.temp_valid = 1, .max_id = 7, .max_temp = 16384},
        .temp1 = {.temp0 = 8192, .temp1 = 9216, .temp2 = 10240, .temp3 = 11264},
        .temp2 = {.temp4 = 12288, .temp5 = 13312, .temp6 = 14336, .temp7 = 15360},
    };

    uint16_t tile_index = 0;

    // First entry with valid data
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &tile_temp_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, tile_index);

    // Second entry (last entry)
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &tile_temp_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, tile_index);

    // Call the function under test
    bool result = data_smpl_process_tile_temperature_sensor_fifo();

    // Verify the function returns true (indicating entries were processed)
    assert_true(result);

    // Test case 2: No entries to process
    sensor_ram_poll_status_t no_entries = {.curr_data_is_valid = false, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &no_entries);

    result = data_smpl_process_tile_temperature_sensor_fifo();

    // Verify the function returns false when no entries are processed
    assert_false(result);
}

TEST_FUNCTION(test_data_smpl_parse_tile_voltage_entry, test_setup, test_teardown)
{
    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));

    // Test case 1: Valid tile voltage entry
    tile_voltage_t voltage_data = {
        .timestamp = 1000,
        .data =
            {
                .vcore0 = MILLIVOLTS2DOUT(500),
                .vcore1 = MILLIVOLTS2DOUT(6),
                .vcpu = MILLIVOLTS2DOUT(10),
                .vsys = MILLIVOLTS2DOUT(80),
            },
    };

    uint8_t index = 0;
    bool result = data_smpl_parse_tile_voltage_entry(&voltage_data, index);

    // Verify the function returns true for valid processing
    assert_true(result);

    // Check core 0 and core 1 voltage
    // have to use range check due to float/fixed point conversions
    assert_in_range(core_rt[index].latest_voltage_mV, 499, 500);
    assert_in_range(core_rt[index + 1].latest_voltage_mV, 5, 6);

    // Test case 2: Index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    result = data_smpl_parse_tile_voltage_entry(&voltage_data, index);

    // Verify the function returns false for out of bounds index
    assert_false(result);
}

TEST_FUNCTION(test_data_smpl_process_tile_voltage_sensor_fifo, test_setup, test_teardown)
{
    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));

    // Test case 1: Process two FIFO entries - one valid, one invalid
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    tile_voltage_t voltage_entry = {
        .timestamp = 1000,
        .data =
            {
                .vcore0 = MILLIVOLTS2DOUT(500),
                .vcore1 = MILLIVOLTS2DOUT(600),
                .vcpu = MILLIVOLTS2DOUT(700),
                .vsys = MILLIVOLTS2DOUT(800),
            },
    };

    uint16_t tile_index = 0;
    uint16_t invalid_tile_index = NUMBER_OF_TILES_PER_DIE; // Out of range index

    // First entry with valid data
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, tile_index);

    // Second entry with invalid tile index (valid_entry == false)
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &voltage_entry);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, invalid_tile_index);

    // Call the function under test
    data_smpl_process_tile_voltage_sensor_fifo();

    // Verify voltage data was processed correctly
    // Tile 0 maps to cores 0 and 1
    assert_in_range(core_rt[0].latest_voltage_mV, 499, 500); // vcore0 for core 0
    assert_in_range(core_rt[1].latest_voltage_mV, 599, 600); // vcore1 for core 1

    // Test case 2: No entries to process
    sensor_ram_poll_status_t no_entries = {.curr_data_is_valid = false, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &no_entries);

    data_smpl_process_tile_voltage_sensor_fifo();
}

TEST_FUNCTION(test_data_smpl_parse_core_current_entry, test_setup, test_teardown)
{
    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));

    // Test case 1: Invalid timestamp (zero) should return false
    core_current_t current_data = {
        .timestamp = 0, // Invalid timestamp
        .data =
            {
                .avg = 100,
                .min = 10,
                .max = 150,
                .volt = 1,
                .pwr = 50,
                .pstate = 12,
                .change = 1,
                .mpam_id_low = 5,
                .mpam_id_high = 0,
                .cstate = 0,
            },
    };
    uint8_t core_id = 0;
    core_rt[core_id].status_flags.throttle_is_active = true; // pstate will come from this current packet

    bool valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_false(valid_entry); // timestamp of zero should fail

    // Test case 2: Valid entry with throttling active (pstate from current packet)
    current_data.timestamp = 1000; // Valid timestamp
    current_data.data.avg = 120;
    current_data.data.pwr = 75;
    current_data.data.pstate = 13;
    current_data.data.mpam_id_low = 8;
    // Set voltage for P=V×I calculation: (3864 mA * 621 mV) / 1000 ≈ 2399 mW
    core_rt[core_id].latest_voltage_mV = 621;

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify current, power, and MPAM ID were updated
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(120 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, 2399);
    assert_int_equal(core_rt[core_id].latest_mpam_id, 8);

    // Verify pstate handling during throttling
    assert_int_equal(core_rt[core_id].pstate_from_current_pkt, 13);
    assert_int_equal(core_rt[core_id].latest_pstate, 13);

    // Test case 3: Valid entry with throttling inactive (no pstate update from current packet)
    memset(core_rt, 0, sizeof(core_rt)); // Reset runtime info
    core_id = 1;
    core_rt[core_id].status_flags.throttle_is_active = false; // No throttling

    current_data.data.avg = 80;
    current_data.data.pwr = 40;
    current_data.data.pstate = 15; // This should not update latest_pstate when not throttling
    current_data.data.mpam_id_low = 3;
    // Set voltage for P=V×I calculation: (2576 mA * 497 mV) / 1000 ≈ 1280 mW
    core_rt[core_id].latest_voltage_mV = 497;

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify current, power, and MPAM ID were updated
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(80 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, 1280);
    assert_int_equal(core_rt[core_id].latest_mpam_id, 3);

    // Verify pstate is NOT updated from current packet when not throttling
    assert_int_equal(core_rt[core_id].latest_pstate, 0);           // Should remain at initial value
    assert_int_equal(core_rt[core_id].pstate_from_current_pkt, 0); // Should not be set

    // Test case 4: Edge values testing
    core_id = 2;
    core_rt[core_id].status_flags.throttle_is_active = true;

    current_data.data.avg = 0;         // Minimum current
    current_data.data.pwr = 0;         // Minimum power
    current_data.data.mpam_id_low = 0; // Minimum MPAM ID

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify edge values are handled correctly
    assert_int_equal(core_rt[core_id].latest_current_mA, 0);
    assert_int_equal(core_rt[core_id].latest_power_mW, 0);
    assert_int_equal(core_rt[core_id].latest_mpam_id, 0);

    // Test case 5: Maximum valid core ID
    core_id = NUMBER_OF_CORES_PER_DIE - 1; // Last valid core ID

    current_data.data.avg = 200;
    current_data.data.pwr = 100;
    // Set voltage for P=V×I calculation: (6440 mA * 497 mV) / 1000 ≈ 3200 mW
    core_rt[core_id].latest_voltage_mV = 497;

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify it works with maximum valid core ID
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(200 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, 3200);

    // Test case 6: Invalid core ID (out of range) should return false
    core_id = NUMBER_OF_CORES_PER_DIE; // Out of range

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_false(valid_entry); // Should fail due to invalid core ID

    // Test case 7: Another invalid core ID (way out of range)
    core_id = 255; // Way out of range

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_false(valid_entry); // Should fail due to invalid core ID
}

TEST_FUNCTION(test_data_smpl_process_core_current_sensor_fifo, test_setup, test_teardown)
{
    // Clear runtime info before test
    memset(core_rt, 0, sizeof(core_rt));

    // Test case 1: Process multiple FIFO entries with different scenarios
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    // First entry: Valid entry with throttling active (triggers all metric paths)
    core_current_t valid_current_entry = {
        .timestamp = 1000,
        .data =
            {
                .avg = 100,
                .pwr = 50,
                .pstate = 12,
                .mpam_id_low = 5,
            },
    };
    uint16_t core_id_1 = 0;

    // Set up core state for first entry (throttling active, pstate valid)
    core_rt[core_id_1].status_flags.throttle_is_active = true;
    core_rt[core_id_1].status_flags.pkt_pstate_is_valid = true;
    core_rt[core_id_1].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    core_rt[core_id_1].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;
    // Set voltage for P=V×I calculation: (3220 mA * 497 mV) / 1000 ≈ 1600 mW
    core_rt[core_id_1].latest_voltage_mV = 497;

    // Second entry: Valid entry with throttling inactive (simpler metric path)
    core_current_t valid_current_entry_2 = {
        .timestamp = 2000,
        .data =
            {
                .avg = 80,
                .pwr = 40,
                .pstate = 15,
                .mpam_id_low = 3,
            },
    };
    uint16_t core_id_2 = 1;

    // Set up core state for second entry (no throttling, pstate valid)
    core_rt[core_id_2].status_flags.throttle_is_active = false;
    core_rt[core_id_2].status_flags.pkt_pstate_is_valid = true;
    // Set voltage for P=V×I calculation: (2576 mA * 497 mV) / 1000 ≈ 1280 mW
    core_rt[core_id_2].latest_voltage_mV = 497;

    // Third entry: Invalid entry (invalid timestamp) - should be skipped
    core_current_t invalid_current_entry = {
        .timestamp = 0, // Invalid timestamp
        .data =
            {
                .avg = 200,
                .pwr = 100,
                .pstate = 20,
                .mpam_id_low = 8,
            },
    };
    uint16_t core_id_3 = 2;

    // Mock sequence for sensor_fifo_svc_poll_core_current calls

    // First valid entry
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &valid_current_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, core_id_1);

    // Second valid entry
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &valid_current_entry_2);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, core_id_2);

    // Third invalid entry (timestamp = 0)
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &invalid_current_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, core_id_3);

    // Mock for exec_tlm_cmpnt_get_timestamp_microseconds - called by data_smpl_calculate_mpam_data_from_cores
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10000);

    // Call the function under test
    data_smpl_process_core_current_sensor_fifo();

    // Verify first entry was processed correctly (throttling active path)
    assert_int_equal(core_rt[core_id_1].latest_current_mA, (uint16_t)(100 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id_1].latest_power_mW, 1600);
    assert_int_equal(core_rt[core_id_1].latest_mpam_id, 5);
    assert_int_equal(core_rt[core_id_1].latest_pstate, 12); // Should be updated from current packet

    // Verify second entry was processed correctly (no throttling path)
    assert_int_equal(core_rt[core_id_2].latest_current_mA, (uint16_t)(80 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id_2].latest_power_mW, 1280);
    assert_int_equal(core_rt[core_id_2].latest_mpam_id, 3);
    assert_int_equal(core_rt[core_id_2].latest_pstate, 0); // Should NOT be updated (no throttling)

    // Verify third entry was NOT processed (invalid timestamp)
    assert_int_equal(core_rt[core_id_3].latest_current_mA, 0);
    assert_int_equal(core_rt[core_id_3].latest_power_mW, 0);
    assert_int_equal(core_rt[core_id_3].latest_mpam_id, 0);

    // Test case 2: No entries to process (curr_data_is_valid = false)
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &invalid_entry);

    // Store previous values to verify they don't change
    uint16_t prev_current = core_rt[core_id_1].latest_current_mA;
    uint16_t prev_power = core_rt[core_id_1].latest_power_mW;

    data_smpl_process_core_current_sensor_fifo();

    // Verify no processing occurred (values unchanged)
    assert_int_equal(core_rt[core_id_1].latest_current_mA, prev_current);
    assert_int_equal(core_rt[core_id_1].latest_power_mW, prev_power);
}

TEST_FUNCTION(test_data_smpl_parse_vr_temperature_entry, test_setup, test_teardown)
{
    // Clear SOC runtime info before test
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 1: Standard VR temperature values
    vr_temp_t vr_temperature = {
        .timestamp = 1000,
        .vr_temp_dC = {15, 25, 35, 45, 55, 65, 75, 85},
    };

    data_smpl_parse_vr_temperature_entry(&vr_temperature);

    // Verify all VR temperatures were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], vr_temperature.vr_temp_dC[index]);
    }

    // Test case 2: Zero/minimum temperatures
    vr_temp_t vr_temperature_zeros = {
        .timestamp = 2000,
        .vr_temp_dC = {0, 0, 0, 0, 0, 0, 0, 0},
    };

    data_smpl_parse_vr_temperature_entry(&vr_temperature_zeros);

    // Verify all VR temperatures were updated to zero
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], 0);
    }

    // Test case 3: Maximum realistic temperatures
    vr_temp_t vr_temperature_high = {
        .timestamp = 3000,
        .vr_temp_dC = {100, 110, 95, 105, 90, 115, 120, 125},
    };

    data_smpl_parse_vr_temperature_entry(&vr_temperature_high);

    // Verify all high temperatures were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], vr_temperature_high.vr_temp_dC[index]);
    }

    // Test case 4: Mixed temperature values
    vr_temp_t vr_temperature_mixed = {
        .timestamp = 4000,
        .vr_temp_dC = {10, 50, 30, 80, 60, 40, 70, 20},
    };

    data_smpl_parse_vr_temperature_entry(&vr_temperature_mixed);

    // Verify mixed temperatures were updated correctly
    uint16_t expected_mixed[] = {10, 50, 30, 80, 60, 40, 70, 20};
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], expected_mixed[index]);
    }
}

TEST_FUNCTION(test_data_smpl_process_vr_temp_sensor_fifo, test_setup, test_teardown)
{
    // Clear SOC runtime info before test
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 1: Process multiple FIFO entries
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    // First VR temperature entry
    vr_temp_t vr_temp_entry_1 = {
        .timestamp = 1000,
        .vr_temp_dC = {30, 35, 40, 45, 50, 55, 60, 65},
    };

    // Second VR temperature entry
    vr_temp_t vr_temp_entry_2 = {
        .timestamp = 2000,
        .vr_temp_dC = {25, 30, 35, 40, 45, 50, 55, 60},
    };

    // Mock sequence for sensor_fifo_svc_poll_vr_temperature calls
    // Note: This function only takes vr_temp_entry as parameter and returns status

    // First valid entry
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &vr_temp_entry_1);

    // Second valid entry (last entry)
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &vr_temp_entry_2);

    // Call the function under test
    data_smpl_process_vr_temp_sensor_fifo();

    // Verify the last entry was processed (second entry overwrites first)
    uint16_t expected_final[] = {25, 30, 35, 40, 45, 50, 55, 60};
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], expected_final[index]);
    }

    // Test case 2: No entries to process (curr_data_is_valid = false)
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &invalid_entry);

    // Store previous values to verify they don't change
    uint16_t prev_temps[MAX_NUM_OF_VR_RAILS];
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        prev_temps[index] = soc_rt.latest_rail_temperature_dC[index];
    }

    data_smpl_process_vr_temp_sensor_fifo();

    // Verify no processing occurred (values unchanged)
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], prev_temps[index]);
    }

    // Test case 3: Single valid entry
    memset(&soc_rt, 0, sizeof(soc_rt)); // Reset for clean test

    vr_temp_t vr_temp_single = {
        .timestamp = 3000,
        .vr_temp_dC = {100, 90, 80, 70, 60, 50, 40, 30},
    };

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &vr_temp_single);

    data_smpl_process_vr_temp_sensor_fifo();

    // Verify single entry was processed correctly
    uint16_t expected_single[] = {100, 90, 80, 70, 60, 50, 40, 30};
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_temperature_dC[index], expected_single[index]);
    }
}

TEST_FUNCTION(test_data_smpl_parse_vr_current_entry, test_setup, test_teardown)
{
    // Clear SOC runtime info before test
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 1: Standard VR current and voltage values
    vr_current_t vr_data = {
        .timestamp = 1000,
        .vr_current_cA = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage_mV = {1000, 900, 800, 700, 600, 700, 800, 900},
    };

    data_smpl_parse_vr_current_entry(&vr_data);

    // Verify all VR current and voltage values were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index],
                         CONVERT_CENTIAMPS_TO_MILLIAMPS(vr_data.vr_current_cA[index]));
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data.vr_voltage_mV[index]);
    }

    // Test case 2: Zero/minimum values
    vr_current_t vr_data_zeros = {
        .timestamp = 2000,
        .vr_current_cA = {0, 0, 0, 0, 0, 0, 0, 0},
        .vr_voltage_mV = {0, 0, 0, 0, 0, 0, 0, 0},
    };

    data_smpl_parse_vr_current_entry(&vr_data_zeros);

    // Verify all values were updated to zero
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], 0);
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], 0);
    }

    // Test case 3: High current and voltage values
    vr_current_t vr_data_high = {
        .timestamp = 3000,
        .vr_current_cA = {5000, 4500, 4000, 3500, 3000, 2500, 2000, 1500},
        .vr_voltage_mV = {1200, 1100, 1050, 1000, 950, 900, 850, 800},
    };

    data_smpl_parse_vr_current_entry(&vr_data_high);

    // Verify high values were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index],
                         CONVERT_CENTIAMPS_TO_MILLIAMPS(vr_data_high.vr_current_cA[index]));
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data_high.vr_voltage_mV[index]);
    }

    // Test case 4: Mixed patterns
    vr_current_t vr_data_mixed = {
        .timestamp = 4000,
        .vr_current_cA = {100, 200, 150, 250, 175, 225, 125, 275},
        .vr_voltage_mV = {1100, 950, 1050, 900, 1000, 850, 950, 800},
    };

    data_smpl_parse_vr_current_entry(&vr_data_mixed);

    // Verify mixed pattern values were updated correctly
    uint16_t expected_current[] = {100, 200, 150, 250, 175, 225, 125, 275};
    uint16_t expected_voltage[] = {1100, 950, 1050, 900, 1000, 850, 950, 800};

    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], CONVERT_CENTIAMPS_TO_MILLIAMPS(expected_current[index]));
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], expected_voltage[index]);
    }

    // Test case 5: Verify overwrite behavior (new values replace old ones)
    vr_current_t vr_data_overwrite = {
        .timestamp = 5000,
        .vr_current_cA = {999, 888, 777, 666, 555, 444, 333, 222},
        .vr_voltage_mV = {1111, 1000, 889, 778, 667, 556, 445, 334},
    };

    data_smpl_parse_vr_current_entry(&vr_data_overwrite);

    // Verify overwrite occurred correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index],
                         CONVERT_CENTIAMPS_TO_MILLIAMPS(vr_data_overwrite.vr_current_cA[index]));
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data_overwrite.vr_voltage_mV[index]);
    }
}

TEST_FUNCTION(test_data_smpl_process_vr_current_sensor_fifo, test_setup, test_teardown)
{
    // Initialize die ID for this test - use die 1 which has 2 VR rails
    die_2_die_exch_init(1);

    // Test case 1: Process multiple FIFO entries
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    vr_current_t vr_current_entry_1 = {
        .timestamp = 1000,
        .vr_current_cA = {100, 0, 0, 0, 0, 0, 0, 0}, // Simple data for FIFO testing
        .vr_voltage_mV = {1000, 0, 0, 0, 0, 0, 0, 0},
    };

    vr_current_t vr_current_entry_2 = {
        .timestamp = 2000,
        .vr_current_cA = {200, 0, 0, 0, 0, 0, 0, 0}, // Simple data for FIFO testing
        .vr_voltage_mV = {1100, 0, 0, 0, 0, 0, 0, 0},
    };

    // Set up mock expectations for load line loss functions
    // Die 1 has NUM_DIE1_VR_RAILS (2) active VR rails

    // Entry 1: mock load line loss for die 1's 2 VR rails
    for (uint8_t rail = 0; rail < NUM_DIE1_VR_RAILS; rail++)
    {
        expect_value(__wrap_power_telemetry_loadline_loss_die1, rail_id, rail);
        expect_value(__wrap_power_telemetry_loadline_loss_die1,
                     current_cA,
                     (rail == 0) ? 100 : 0); // Only rail 0 has current (100 cA converted from 1000 mA)
        will_return(__wrap_power_telemetry_loadline_loss_die1, 5); // 5mW loss
    }

    // Entry 2: mock load line loss for die 1's 2 VR rails
    for (uint8_t rail = 0; rail < NUM_DIE1_VR_RAILS; rail++)
    {
        expect_value(__wrap_power_telemetry_loadline_loss_die1, rail_id, rail);
        expect_value(__wrap_power_telemetry_loadline_loss_die1,
                     current_cA,
                     (rail == 0) ? 200 : 0); // Only rail 0 has current (200 cA converted from 2000 mA)
        will_return(__wrap_power_telemetry_loadline_loss_die1, 8); // 8mW loss
    }

    // Mock sequence for multiple entries
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &vr_current_entry_1);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &vr_current_entry_2);

    // Call the function under test
    data_smpl_process_vr_current_sensor_fifo();

    // Basic verification that processing occurred (last entry processed)
    assert_int_equal(soc_rt.latest_rail_current_mA[0], CONVERT_CENTIAMPS_TO_MILLIAMPS(200));
    assert_int_equal(soc_rt.latest_rail_voltage_mV[0], 1100);

    // Test case 2: No entries to process (curr_data_is_valid = false)
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &invalid_entry);

    // Store previous value to verify no change
    uint32_t prev_current = soc_rt.latest_rail_current_mA[0];

    data_smpl_process_vr_current_sensor_fifo();

    // Verify no processing occurred (value unchanged)
    assert_int_equal(soc_rt.latest_rail_current_mA[0], prev_current);
}

TEST_FUNCTION(test_data_smpl_parse_pvt_temperature_entry, test_setup, test_teardown)
{
    // Clear SOC runtime info before test
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 1: Standard PVT temperature values with maximum calculation
    soc_pvt_temp_t pvt_temperature = {
        .timestamp = 1000,
        .sensor_temp_dC = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature);

    // Verify all individual sensor temperatures were copied correctly
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        assert_int_equal(soc_rt.latest_soc_top_temp_dC[pvt_index], pvt_temperature.sensor_temp_dC[pvt_index]);
    }

    // Verify maximum temperature was calculated correctly (should be 60)
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 60);

    // Test case 2: All zero temperatures
    soc_pvt_temp_t pvt_temperature_zeros = {
        .timestamp = 2000,
        .sensor_temp_dC = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_zeros);

    // Verify all temperatures are zero
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        assert_int_equal(soc_rt.latest_soc_top_temp_dC[pvt_index], 0);
    }

    // Verify maximum temperature is zero
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 0);

    // Test case 3: Maximum temperature at different positions
    soc_pvt_temp_t pvt_temperature_max_first = {
        .timestamp = 3000,
        .sensor_temp_dC = {100, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_max_first);

    // Verify maximum is found at first position
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 100);

    // Test case 4: Maximum temperature at last position
    soc_pvt_temp_t pvt_temperature_max_last = {
        .timestamp = 4000,
        .sensor_temp_dC = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 120},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_max_last);

    // Verify maximum is found at last position
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 120);

    // Test case 5: Multiple sensors with same maximum temperature
    soc_pvt_temp_t pvt_temperature_same_max = {
        .timestamp = 5000,
        .sensor_temp_dC = {20, 80, 40, 80, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_same_max);

    // Verify maximum is correctly identified (should be 80)
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 80);

    // Test case 6: Realistic high temperature values
    soc_pvt_temp_t pvt_temperature_high = {
        .timestamp = 6000,
        .sensor_temp_dC = {85, 90, 88, 92, 87, 89, 91, 86, 93, 84, 88, 90, 89, 87},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_high);

    // Verify all high temperatures were copied correctly
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        assert_int_equal(soc_rt.latest_soc_top_temp_dC[pvt_index], pvt_temperature_high.sensor_temp_dC[pvt_index]);
    }

    // Verify maximum temperature calculation with high values (should be 93)
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 93);

    // Test case 7: Verify reset behavior (latest_max_soc_top_temp_dC is reset to 0 at start)
    soc_pvt_temp_t pvt_temperature_lower = {
        .timestamp = 7000,
        .sensor_temp_dC = {10, 15, 20, 25, 30, 16, 17, 11, 8, 12, 18, 9, 22, 14},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_lower);

    // Verify maximum was recalculated from the new data (should be 30, not influenced by previous 93)
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 30);

    // Test case 8: Single sensor with maximum temperature (edge case)
    soc_pvt_temp_t pvt_temperature_single_max = {
        .timestamp = 8000,
        .sensor_temp_dC = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 50},
    };

    data_smpl_parse_pvt_temperature_entry(&pvt_temperature_single_max);

    // Verify the single maximum is correctly identified
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 50);
    assert_int_equal(soc_rt.latest_soc_top_temp_dC[13], 50); // Last sensor
}

TEST_FUNCTION(test_data_smpl_process_pvt_temperature_sensor_fifo, test_setup, test_teardown)
{
    // Clear soc_rt structure to ensure clean test state
    memset(&soc_rt, 0, sizeof(soc_rt));

    // Test case 1: Process multiple FIFO entries and return true
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    // Simple PVT temperature data for FIFO testing
    soc_pvt_temp_t pvt_temp_entry_1 = {
        .timestamp = 1000,
        .sensor_temp_dC = {50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // All 15 sensors
    };

    soc_pvt_temp_t pvt_temp_entry_2 = {
        .timestamp = 2000,
        .sensor_temp_dC = {60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // All 15 sensors
    };

    // Mock sequence for multiple entries
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_entry_1);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_entry_2);

    // Call the function under test
    bool result = data_smpl_process_pvt_temperature_sensor_fifo();

    // Verify function returns true (at least one valid entry processed)
    assert_true(result);

    // Basic verification that processing occurred (last entry processed)
    assert_int_equal(soc_rt.latest_soc_top_temp_dC[0], 60);
    assert_int_equal(soc_rt.latest_max_soc_top_temp_dC, 60);

    // Test case 2: No valid entries to process (should return false)
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &invalid_entry);

    result = data_smpl_process_pvt_temperature_sensor_fifo();

    // Verify function returns false (no valid entries processed)
    assert_false(result);

    // Test case 3: Mixed valid/invalid entries (should return true if any valid)
    sensor_ram_poll_status_t invalid_first = {.curr_data_is_valid = false, .more_entries = true};

    soc_pvt_temp_t pvt_temp_final = {
        .timestamp = 3000,
        .sensor_temp_dC = {70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &invalid_first);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &pvt_temp_final);

    result = data_smpl_process_pvt_temperature_sensor_fifo();

    // Verify function returns true (at least one valid entry was processed)
    assert_true(result);

    // Verify the valid entry was processed
    assert_int_equal(soc_rt.latest_soc_top_temp_dC[0], 70);
}

TEST_FUNCTION(test_data_smpl_parse_dimm_entry, test_setup, test_teardown)
{
    // Clear dimm_rt structure for clean test state
    memset(&dimm_rt, 0, sizeof(dimm_rt));

    // Test case 1: Valid DIMM entry with s0 temperature higher
    sensor_ram_dimm_info_t dimm_entry_1 = {
        .timestamp = 1000,
        .dimm_temp_s0_dC = 85,
        .dimm_temp_s1_dC = 80,
        .dimm_power_mW = 1500,
        .dimm_throttle_count = 2,
        .dimm_id = 0,
        .dimm_throttling = 1,
        .dimm_memory_frequency_id = 3,
    };

    bool result = data_smpl_parse_dimm_entry(&dimm_entry_1);

    // Verify function returns true for valid dimm_id
    assert_true(result);

    // Verify higher temperature (s0) is selected
    assert_int_equal(dimm_rt.latest_dimm[0].temperature_dC, 85);

    // Verify power and other data are stored correctly
    assert_int_equal(dimm_rt.latest_dimm[0].power_mW, 1500);
    assert_int_equal(dimm_rt.latest_dimm[0].memory_freq_id, 3);
    assert_int_equal(dimm_rt.latest_dimm[0].throttle_source, 1);

    // Verify maximum temperature tracking
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 85);

    // Verify total power accumulation
    assert_int_equal(dimm_rt.latest_dimm_total_pwr_mW, 1500);

    // Test case 2: Valid DIMM entry with s1 temperature higher
    sensor_ram_dimm_info_t dimm_entry_2 = {
        .timestamp = 2000,
        .dimm_temp_s0_dC = 75,
        .dimm_temp_s1_dC = 90,
        .dimm_power_mW = 1200,
        .dimm_throttle_count = 0,
        .dimm_id = 1,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 2,
    };

    result = data_smpl_parse_dimm_entry(&dimm_entry_2);

    assert_true(result);

    // Verify higher temperature (s1) is selected
    assert_int_equal(dimm_rt.latest_dimm[1].temperature_dC, 90);

    // Verify maximum temperature updated to higher value
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 90);

    // Verify total power accumulated
    assert_int_equal(dimm_rt.latest_dimm_total_pwr_mW, 2700); // 1500 + 1200

    // Test case 3: Equal temperatures (edge case)
    sensor_ram_dimm_info_t dimm_entry_3 = {
        .timestamp = 3000,
        .dimm_temp_s0_dC = 70,
        .dimm_temp_s1_dC = 70,
        .dimm_power_mW = 800,
        .dimm_id = 2,
        .dimm_throttling = 2,
        .dimm_memory_frequency_id = 1,
    };

    result = data_smpl_parse_dimm_entry(&dimm_entry_3);

    assert_true(result);

    // When equal, s0 should be selected (based on ternary operator logic)
    assert_int_equal(dimm_rt.latest_dimm[2].temperature_dC, 70);

    // Maximum should remain 90 (not updated by lower value)
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 90);

    // Test case 4: Invalid DIMM ID (boundary case)
    sensor_ram_dimm_info_t dimm_entry_invalid = {
        .timestamp = 4000,
        .dimm_temp_s0_dC = 95,
        .dimm_temp_s1_dC = 100,
        .dimm_power_mW = 2000,
        .dimm_id = 6, // NUMBER_OF_DIMMS_PER_DIE = 6, so valid range is 0-5
        .dimm_throttling = 1,
        .dimm_memory_frequency_id = 4,
    };

    result = data_smpl_parse_dimm_entry(&dimm_entry_invalid);

    // Verify function returns false for invalid dimm_id
    assert_false(result);

    // Verify no data is modified for invalid DIMM ID
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 90);    // Should remain unchanged
    assert_int_equal(dimm_rt.latest_dimm_total_pwr_mW, 3500); // Should remain unchanged (1500 + 1200 + 800)

    // Test case 5: Maximum DIMM ID (valid boundary)
    sensor_ram_dimm_info_t dimm_entry_max = {
        .timestamp = 5000,
        .dimm_temp_s0_dC = 65,
        .dimm_temp_s1_dC = 68,
        .dimm_power_mW = 900,
        .dimm_id = 5, // Maximum valid DIMM ID
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 0,
    };

    result = data_smpl_parse_dimm_entry(&dimm_entry_max);

    assert_true(result);
    assert_int_equal(dimm_rt.latest_dimm[5].temperature_dC, 68);
    assert_int_equal(dimm_rt.latest_dimm[5].power_mW, 900);

    // Verify total power accumulation continues
    assert_int_equal(dimm_rt.latest_dimm_total_pwr_mW, 4400); // Previous + 900

    // Test case 6: High temperature that becomes new maximum
    sensor_ram_dimm_info_t dimm_entry_hot = {
        .timestamp = 6000,
        .dimm_temp_s0_dC = 105,
        .dimm_temp_s1_dC = 102,
        .dimm_power_mW = 1800,
        .dimm_id = 3,
        .dimm_throttling = 3,
        .dimm_memory_frequency_id = 4,
    };

    result = data_smpl_parse_dimm_entry(&dimm_entry_hot);

    assert_true(result);
    assert_int_equal(dimm_rt.latest_dimm[3].temperature_dC, 105);

    // Verify new maximum temperature
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 105);
}

TEST_FUNCTION(test_data_smpl_process_dimm_sensor_fifo, test_setup, test_teardown)
{
    // Test case 1: Process multiple valid DIMM entries and return true
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    // Simple DIMM data for FIFO testing
    sensor_ram_dimm_info_t dimm_entry_1 = {
        .timestamp = 1000,
        .dimm_temp_s0_dC = 75,
        .dimm_temp_s1_dC = 70,
        .dimm_power_mW = 1200,
        .dimm_id = 0,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 2,
    };

    sensor_ram_dimm_info_t dimm_entry_2 = {
        .timestamp = 2000,
        .dimm_temp_s0_dC = 80,
        .dimm_temp_s1_dC = 85,
        .dimm_power_mW = 1500,
        .dimm_id = 1,
        .dimm_throttling = 1,
        .dimm_memory_frequency_id = 3,
    };

    // Mock sequence for multiple entries
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_1);

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_2);

    // Call the function under test
    bool result = data_smpl_process_dimm_sensor_fifo();

    // Verify function returns true (at least one valid entry processed)
    assert_true(result);

    // Test case 2: No valid entries to process (should return false)
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &invalid_entry);

    result = data_smpl_process_dimm_sensor_fifo();

    // Verify function returns false (no valid entries processed)
    assert_false(result);

    // Test case 3: Mixed valid/invalid entries (should return true if any valid)
    sensor_ram_poll_status_t invalid_first = {.curr_data_is_valid = false, .more_entries = true};

    sensor_ram_dimm_info_t dimm_entry_final = {
        .timestamp = 3000,
        .dimm_temp_s0_dC = 65,
        .dimm_temp_s1_dC = 68,
        .dimm_power_mW = 900,
        .dimm_id = 2,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 1,
    };

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &invalid_first);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_1); // Won't be processed

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_final);

    result = data_smpl_process_dimm_sensor_fifo();

    // Verify function returns true (at least one valid entry was processed)
    assert_true(result);

    // Test case 4: Invalid DIMM ID should not prevent function from returning true for other valid entries
    sensor_ram_dimm_info_t dimm_entry_invalid_id = {
        .timestamp = 4000,
        .dimm_temp_s0_dC = 90,
        .dimm_temp_s1_dC = 95,
        .dimm_power_mW = 2000,
        .dimm_id = 6, // Invalid ID (valid range 0-5)
        .dimm_throttling = 2,
        .dimm_memory_frequency_id = 4,
    };

    sensor_ram_dimm_info_t dimm_entry_valid = {
        .timestamp = 5000,
        .dimm_temp_s0_dC = 72,
        .dimm_temp_s1_dC = 74,
        .dimm_power_mW = 1100,
        .dimm_id = 3,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 2,
    };

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_invalid_id);

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_valid);

    result = data_smpl_process_dimm_sensor_fifo();

    // Verify function returns true (the valid entry was processed despite the invalid one)
    assert_true(result);

    // Test case 5: Only invalid DIMM IDs (should return false)
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_entry_invalid_id);

    result = data_smpl_process_dimm_sensor_fifo();

    // Verify function returns false (no valid entries processed)
    assert_false(result);
}

TEST_FUNCTION(test_data_smpl_parse_pstate, test_setup, test_teardown)
{
    // Clear core runtime info for clean test state
    memset(&core_rt[0], 0, sizeof(core_rt[0]));

    core_state_entry_data_t core_entry_data = {0};

    // Test case 1: First pstate packet (pkt_pstate_is_valid = false)
    pstate_telem_t pstate_data = {
        .timestamp = 1000,
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 0,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    core_entry_data.packet_timestamp_uS = 2000;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify first packet behavior
    assert_true(core_rt[0].status_flags.pkt_pstate_is_valid);
    assert_true(core_entry_data.valid_entry_pstate);
    assert_true(core_entry_data.pstate_change);               // Always true for first packet
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 0); // No residency for first packet
    assert_int_equal(core_rt[0].latest_pstate, 12);
    assert_int_equal(core_rt[0].latest_plimit, 5);
    assert_int_equal(core_rt[0].pstate_from_pstate_pkt, 12);
    assert_int_equal(core_rt[0].pstate_res_timestamp_uS, 2000);

    // Test case 2: Pstate change (same pstate)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000;
    pstate_data.data.pstate = 12; // Same pstate
    pstate_data.data.plimit = 7;  // Different plimit

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify no pstate change
    assert_true(core_entry_data.valid_entry_pstate);
    assert_false(core_entry_data.pstate_change);                 // Same pstate
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 1000); // 3000 - 2000
    assert_int_equal(core_rt[0].latest_pstate, 12);
    assert_int_equal(core_rt[0].latest_plimit, 7); // Updated
    assert_int_equal(core_rt[0].pstate_res_timestamp_uS, 3000);

    // Test case 3: Actual pstate change
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 5000;
    pstate_data.data.pstate = 8; // Different pstate
    pstate_data.data.plimit = 3;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify pstate change
    assert_true(core_entry_data.valid_entry_pstate);
    assert_true(core_entry_data.pstate_change);                  // Different pstate
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 2000); // 5000 - 3000
    assert_int_equal(core_rt[0].latest_pstate, 8);
    assert_int_equal(core_rt[0].latest_plimit, 3);
    assert_int_equal(core_rt[0].pstate_res_timestamp_uS, 5000);

    // Test case 4: Invalid timestamp (older than current)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 4000; // Older than current (5000)
    uint8_t previous_pstate = core_rt[0].latest_pstate;
    uint8_t previous_plimit = core_rt[0].latest_plimit;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify early return behavior (no changes except timestamp)
    assert_false(core_entry_data.valid_entry_pstate);
    assert_false(core_entry_data.pstate_change);
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 0);
    assert_int_equal(core_rt[0].latest_pstate, previous_pstate); // Unchanged
    assert_int_equal(core_rt[0].latest_plimit, previous_plimit); // Unchanged
    assert_int_equal(core_rt[0].pstate_res_timestamp_uS, 4000);  // Updated for SVP workaround

    // Test case 5: Different core ID
    memset(&core_rt[1], 0, sizeof(core_rt[1])); // Clear core 1
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 1500;
    pstate_data.data.core = 1;
    pstate_data.data.pstate = 15;
    pstate_data.data.plimit = 10;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify core 1 is updated, core 0 unchanged
    assert_true(core_rt[1].status_flags.pkt_pstate_is_valid);
    assert_true(core_entry_data.valid_entry_pstate);
    assert_true(core_entry_data.pstate_change); // First packet for core 1
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 0);
    assert_int_equal(core_rt[1].latest_pstate, 15);
    assert_int_equal(core_rt[1].latest_plimit, 10);
    assert_int_equal(core_rt[1].pstate_res_timestamp_uS, 1500);

    // Core 0 should be unchanged
    assert_int_equal(core_rt[0].latest_pstate, 8);
    assert_int_equal(core_rt[0].latest_plimit, 3);

    // Test case 6: Edge case - same timestamp
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 1500; // Same as core 1's current timestamp
    pstate_data.data.pstate = 20;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Should process normally (timestamp >= is allowed)
    assert_true(core_entry_data.valid_entry_pstate);
    assert_true(core_entry_data.pstate_change);
    assert_int_equal(core_entry_data.pstate_time_diff_uS, 0); // Same timestamp
    assert_int_equal(core_rt[1].latest_pstate, 20);

    // Test case 7: Maximum values
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 10000;
    pstate_data.data.pstate = 31; // Maximum 5-bit value
    pstate_data.data.plimit = 31;

    data_smpl_parse_pstate(&pstate_data, &core_entry_data);

    // Verify maximum values handled correctly
    assert_true(core_entry_data.valid_entry_pstate);
    assert_true(core_entry_data.pstate_change);
    assert_int_equal(core_rt[1].latest_pstate, 31);
    assert_int_equal(core_rt[1].latest_plimit, 31);
    assert_int_equal(core_rt[1].pstate_from_pstate_pkt, 31);
}

TEST_FUNCTION(test_data_smpl_parse_cstate, test_setup, test_teardown)
{
    // Clear core runtime info for clean test state
    memset(&core_rt[0], 0, sizeof(core_rt[0]));

    core_state_entry_data_t core_entry_data = {0};

    // Test case 1: First cstate packet (pkt_cstate_is_valid = false)
    pstate_telem_t cstate_data = {
        .timestamp = 1000,
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 0, // C0 state
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    core_entry_data.packet_timestamp_uS = 2000;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify first packet behavior
    assert_true(core_rt[0].status_flags.pkt_cstate_is_valid);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);               // Always true for first packet
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0); // No residency for first packet
    assert_int_equal(core_rt[0].latest_cstate, 0);            // C0
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 2000);

    // Test case 2: Same cstate (no change)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000;
    cstate_data.data.cstate = 0; // Same C0 state

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify no cstate change
    assert_true(core_entry_data.valid_entry_cstate);
    assert_false(core_entry_data.cstate_change);                 // Same cstate
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 1000); // 3000 - 2000
    assert_int_equal(core_rt[0].latest_cstate, 0);
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 3000);

    // Test case 3: Valid cstate change (C0 to C1)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 5000;
    cstate_data.data.cstate = 1; // C1 state
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify cstate change
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);                  // Different cstate
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 2000); // 5000 - 3000
    assert_int_equal(core_rt[0].latest_cstate, 1);               // C1
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 5000);

    // Test case 4: Valid cstate change (C1 back to C0)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 7000;
    cstate_data.data.cstate = 0; // Back to C0
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify valid transition back to C0
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 2000); // 7000 - 5000
    assert_int_equal(core_rt[0].latest_cstate, 0);               // Back to C0

    // Test case 5: Valid transition (C0 to C2)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 8000;
    cstate_data.data.cstate = 2; // C2 state
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify valid C0 to C2 transition
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[0].latest_cstate, 2); // C2

    // Test case 6: Valid same cstate (C2 to C2) - should be allowed
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 10000;
    cstate_data.data.cstate = 2; // Same C2 state
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify same cstate is allowed
    assert_true(core_entry_data.valid_entry_cstate);
    assert_false(core_entry_data.cstate_change);   // Same cstate
    assert_int_equal(core_rt[0].latest_cstate, 2); // Still C2

    // Test case 7: Invalid timestamp (older than current)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 9500; // Older than current (10000)
    uint8_t previous_cstate = core_rt[0].latest_cstate;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify early return behavior (no changes except timestamp)
    assert_false(core_entry_data.valid_entry_cstate);
    assert_false(core_entry_data.cstate_change);
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0);
    assert_int_equal(core_rt[0].latest_cstate, previous_cstate); // Unchanged
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 9500);  // Updated for SVP workaround

    // Test case 8: Different core ID
    memset(&core_rt[1], 0, sizeof(core_rt[1])); // Clear core 1
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 1500;
    cstate_data.data.core = 1;
    cstate_data.data.cstate = 0;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify core 1 is updated, core 0 unchanged
    assert_true(core_rt[1].status_flags.pkt_cstate_is_valid);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change); // First packet for core 1
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0);
    assert_int_equal(core_rt[1].latest_cstate, 0);
    assert_int_equal(core_rt[1].cstate_res_timestamp_uS, 1500);

    // Core 0 should be unchanged
    assert_int_equal(core_rt[0].latest_cstate, 2);

    // Test case 9: Valid complex transition sequence (C0->C3->C0->C1)
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 2000;
    cstate_data.data.cstate = 3; // C0 to C3

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[1].latest_cstate, 3);
    // C3 back to C0
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 2500;
    cstate_data.data.cstate = 0;
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[1].latest_cstate, 0);

    // C0 to C1
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000;
    cstate_data.data.cstate = 1;
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[1].latest_cstate, 1);

    // Test case 10: Edge case - same timestamp
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000; // Same timestamp
    cstate_data.data.cstate = 0;
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Should process normally (timestamp >= is allowed)
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0); // Same timestamp
    assert_int_equal(core_rt[1].latest_cstate, 0);
}

TEST_FUNCTION(test_data_smpl_parse_cstate_with_entry_latency, test_setup, test_teardown)
{
    // Test the processing of C-state entry latency in case of a change and read timesstamps from TFA
    uint8_t core_id = 0;
    // Initialize the tfa buffer
    uint8_t cstate_timestamp_id = 0;
    die_2_die_exch_init(0);
    //  init temp timestamp for each cstate entry/exit point
    uint64_t timestamp_cstate_uS[CSTATE_MAX_ID] = {1000, 1010, 1020, 1030, 1040, 1050};
    cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[core_id];
    core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS

    // Clear core runtime info for clean test state
    memset(&core_rt[0], 0, sizeof(core_rt[0]));
    core_state_entry_data_t core_entry_data = {0};
    // Test case 1: First cstate packet (pkt_cstate_is_valid = false)
    pstate_telem_t cstate_data = {
        .timestamp = 1000,
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 0, // C0 state
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    core_entry_data.packet_timestamp_uS = 2000;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify first packet behavior
    assert_true(core_rt[0].status_flags.pkt_cstate_is_valid);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);               // Always true for first packet
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0); // No residency for first packet
    assert_int_equal(core_rt[0].latest_cstate, 0);            // C0
    assert_int_equal(core_rt[0].cstate_res_timestamp_uS, 2000);

    memset(&core_entry_data, 0, sizeof(core_entry_data));

    core_entry_data.packet_timestamp_uS = 5000;
    cstate_data.data.cstate = 2;                                        // Change to C2 state
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, true); // to make sure we have a valid entry latency
    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify cstate latency update
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change); // Different cstate
    assert_int_equal(core_rt[core_id].latest_cstate, 2);
    // Calculate expected latency: packet timestamp - cstate entry timestamp from TFA
    assert_int_equal(core_rt[core_id].latest_cstate_entry_latency_uS, core_entry_data.packet_timestamp_uS - 1000);
}
TEST_FUNCTION(test_data_proc_tlm_cmpnt_24hr_pkg_completed, test_setup, test_teardown)
{

    uint8_t core_id = 0;
    for (core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);
    }
    data_proc_tlm_cmpnt_24hr_pkg_completed();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_pwr_pkg_completed, test_setup, test_teardown)
{
    data_proc_tlm_cmpnt_pwr_pkg_completed();
}

TEST_FUNCTION(test_data_smpl_update_max_die_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(0);
    soc_rt.latest_max_die_temp_dC = 0;
    soc_rt.latest_max_tile_temp_dC = 400;
    soc_rt.latest_max_soc_top_temp_dC = 300;
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    data_smpl_update_max_die_temp();

    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, true);
    data_smpl_update_max_die_temp();

    soc_rt.latest_max_tile_temp_dC = 200;
    soc_rt.latest_max_soc_top_temp_dC = 300;
    die_2_die_exch_init(1);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, true);
    data_smpl_update_max_die_temp();
}

TEST_FUNCTION(test_data_smpl_process_aging_data, test_setup, test_teardown)
{
    uint64_t this_pwr_pkg_timestamp_uS = 2000;
    uint8_t core_id = 0;
    core_aging[core_id].measurement_index = 0;
    core_is_active[core_id] = 1;
    core_rt[core_id].latest_vmin_mV = 100;
    core_rt[core_id].latest_max_value_dC = 20;
    core_aging[core_id].measurement_armed = true;
    uint8_t counter_id = core_aging[core_id].measurement_index;

    will_return(__wrap_dvfs_c2_pcm_aging_get_sensor_status, 1);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 30);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 28);
    will_return(__wrap_dvfs_c2_get_pcm_bank_sensor_data, 0);
    expect_function_call(__wrap_dvfs_c2_pcm_enable_aging_sensor_measurement);

    data_smpl_process_aging_data(core_id, this_pwr_pkg_timestamp_uS);

    assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS, 2000);
    assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter, 28);
    assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter, 30);
    assert_int_equal(computed_metrics_24_hrs.cores[0].core_aging_counters[counter_id].voltage_mV, 100);
    assert_int_equal(computed_metrics_24_hrs.cores[0].core_aging_counters[counter_id].temperature_dC, 20);
}

TEST_FUNCTION(test_data_smpl_update_metrics_for_cores_aging_counters, test_setup, test_teardown)
{
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 2000);
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_is_active[core_id] = false;
    }
    data_smpl_update_metrics_for_cores_aging_counters();
}

TEST_FUNCTION(test_data_smpl_finalize_pwr_pkg_metrics, test_setup, test_teardown)
{
    static uint64_t expected_droop_counts[NUMBER_OF_CORES_PER_DIE];

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        expected_droop_counts[i] = i * 10;
    }

    // Test Case 1: No valid states (all flags false) - should only call timestamp function
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 1000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);
    // Call the function to be tested

    data_smpl_finalize_pwr_pkg_metrics();

    // Should not crash and no metrics should be updated since no flags are set

    // Test Case 2: Only CState valid for some cores
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t core_id_1 = 0;
    uint8_t core_id_2 = 2;

    // Set up core 0 with valid cstate
    core_rt[core_id_1].status_flags.pkt_cstate_is_valid = true;
    core_rt[core_id_1].latest_cstate = CSTATE_C1;
    core_rt[core_id_1].cstate_res_timestamp_uS = 500;
    computed_metrics_2_mins.cores[core_id_1].cstate[CSTATE_C1].residency_uS = 100;

    // Set up core 2 with valid cstate
    core_rt[core_id_2].status_flags.pkt_cstate_is_valid = true;
    core_rt[core_id_2].latest_cstate = CSTATE_C2;
    core_rt[core_id_2].cstate_res_timestamp_uS = 300;
    computed_metrics_2_mins.cores[core_id_2].cstate[CSTATE_C2].residency_uS = 200;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 2000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // Core 0: 100 + (2000 - 500) = 1600
    assert_int_equal(computed_metrics_2_mins.cores[core_id_1].cstate[CSTATE_C1].residency_uS, 1600);
    // Core 2: 200 + (2000 - 300) = 1900
    assert_int_equal(computed_metrics_2_mins.cores[core_id_2].cstate[CSTATE_C2].residency_uS, 1900);

    // Test Case 3: Only PState valid for some cores
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t core_id_3 = 1;
    uint8_t pstate_3 = 5;

    // Set up core 1 with valid pstate
    core_rt[core_id_3].status_flags.pkt_pstate_is_valid = true;
    core_rt[core_id_3].latest_pstate = pstate_3;
    core_rt[core_id_3].pstate_res_timestamp_uS = 800;
    computed_metrics_2_mins.cores[core_id_3].pstate[pstate_3].residency_uS = 150;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 3000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // Core 1: 150 + (3000 - 800) = 2350
    assert_int_equal(computed_metrics_2_mins.cores[core_id_3].pstate[pstate_3].residency_uS, 2350);

    // Test Case 4: Both CState and PState valid for the same core
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t core_id_4 = 0;
    uint8_t pstate_4 = 3;

    // Set up core 0 with both valid cstate and pstate
    core_rt[core_id_4].status_flags.pkt_cstate_is_valid = true;
    core_rt[core_id_4].status_flags.pkt_pstate_is_valid = true;
    core_rt[core_id_4].latest_cstate = CSTATE_C0;
    core_rt[core_id_4].latest_pstate = pstate_4;
    core_rt[core_id_4].cstate_res_timestamp_uS = 1200;
    core_rt[core_id_4].pstate_res_timestamp_uS = 1500;
    computed_metrics_2_mins.cores[core_id_4].cstate[CSTATE_C0].residency_uS = 300;
    computed_metrics_2_mins.cores[core_id_4].pstate[pstate_4].residency_uS = 400;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 4000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // CState: 300 + (4000 - 1200) = 3100
    assert_int_equal(computed_metrics_2_mins.cores[core_id_4].cstate[CSTATE_C0].residency_uS, 3100);
    // PState: 400 + (4000 - 1500) = 2900
    assert_int_equal(computed_metrics_2_mins.cores[core_id_4].pstate[pstate_4].residency_uS, 2900);

    // Test Case 5: Multiple cores with different combinations of valid states
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    // Core 0: Only CState valid
    core_rt[0].status_flags.pkt_cstate_is_valid = true;
    core_rt[0].latest_cstate = CSTATE_C3;
    core_rt[0].cstate_res_timestamp_uS = 2000;
    computed_metrics_2_mins.cores[0].cstate[CSTATE_C3].residency_uS = 50;

    // Core 1: Only PState valid
    core_rt[1].status_flags.pkt_pstate_is_valid = true;
    core_rt[1].latest_pstate = 7;
    core_rt[1].pstate_res_timestamp_uS = 2500;
    computed_metrics_2_mins.cores[1].pstate[7].residency_uS = 75;

    // Core 2: Both valid
    core_rt[2].status_flags.pkt_cstate_is_valid = true;
    core_rt[2].status_flags.pkt_pstate_is_valid = true;
    core_rt[2].latest_cstate = CSTATE_C1;
    core_rt[2].latest_pstate = 4;
    core_rt[2].cstate_res_timestamp_uS = 1800;
    core_rt[2].pstate_res_timestamp_uS = 1900;
    computed_metrics_2_mins.cores[2].cstate[CSTATE_C1].residency_uS = 25;
    computed_metrics_2_mins.cores[2].pstate[4].residency_uS = 35;

    // Core 3: Neither valid (no flags set) - should be skipped

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // Core 0 CState: 50 + (5000 - 2000) = 3050
    assert_int_equal(computed_metrics_2_mins.cores[0].cstate[CSTATE_C3].residency_uS, 3050);
    // Core 1 PState: 75 + (5000 - 2500) = 2575
    assert_int_equal(computed_metrics_2_mins.cores[1].pstate[7].residency_uS, 2575);
    // Core 2 CState: 25 + (5000 - 1800) = 3225
    assert_int_equal(computed_metrics_2_mins.cores[2].cstate[CSTATE_C1].residency_uS, 3225);
    // Core 2 PState: 35 + (5000 - 1900) = 3135
    assert_int_equal(computed_metrics_2_mins.cores[2].pstate[4].residency_uS, 3135);
    // Core 3 should remain at 0 (not updated)
    assert_int_equal(computed_metrics_2_mins.cores[3].cstate[CSTATE_C0].residency_uS, 0);
    assert_int_equal(computed_metrics_2_mins.cores[3].pstate[0].residency_uS, 0);

    // Test Case 6: Throttle active branch - tests the data_smpl_finalize_pwr_pkg_throttling_metrics call
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t core_throttle = 1;
    uint8_t throttle_src = THROTTLE_SOURCE_CURRENT;

    // Set up core 1 with throttle active
    core_rt[core_throttle].status_flags.throttle_is_active = true;
    core_rt[core_throttle].throttle_source_tracker[throttle_src] = true;
    core_rt[core_throttle].throttle_res_timestamp_uS[throttle_src] = 3000;
    computed_metrics_2_mins.cores[core_throttle].throttle_info[throttle_src].residency_uS = 500;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 6000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // Should have called data_smpl_finalize_pwr_pkg_throttling_metrics and updated throttle residency
    // Throttle residency: 500 + (6000 - 3000) = 3500
    assert_int_equal(computed_metrics_2_mins.cores[core_throttle].throttle_info[throttle_src].residency_uS, 3500);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        if (core_is_active[i])
        {
            assert_int_equal(computed_metrics_2_mins.cores[i].droop_count, expected_droop_counts[i]);
        }
    }

    // Test Case 7: MPAM throttling active - tests the MPAM throttling loop
    memset(core_rt, 0, sizeof(core_rt));
    memset(mpam_rt, 0, sizeof(mpam_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t mpam_id_1 = 0;
    uint8_t mpam_id_2 = 2;
    uint8_t nominal_pstate_1 = 3;
    uint8_t nominal_pstate_2 = 5;

    // Set up MPAM 0 with active throttling
    mpam_rt[mpam_id_1].status_flags.active = true;
    mpam_rt[mpam_id_1].status_flags.throttling = true;
    mpam_rt[mpam_id_1].residency_timestamp_uS = 2000;
    mpam_rt[mpam_id_1].nominal_pstate = nominal_pstate_1;
    computed_metrics_2_mins.mpam[mpam_id_1].residency_uS = 1000;

    // Set up MPAM 2 with active throttling (different initial values)
    mpam_rt[mpam_id_2].status_flags.active = true;
    mpam_rt[mpam_id_2].status_flags.throttling = true;
    mpam_rt[mpam_id_2].residency_timestamp_uS = 1500;
    mpam_rt[mpam_id_2].nominal_pstate = nominal_pstate_2;
    computed_metrics_2_mins.mpam[mpam_id_2].residency_uS = 800;

    // Set up MPAM 1 with active=true but throttling=false (should be skipped)
    mpam_rt[1].status_flags.active = true;
    mpam_rt[1].status_flags.throttling = false;
    mpam_rt[1].residency_timestamp_uS = 3000;
    mpam_rt[1].nominal_pstate = 2;
    computed_metrics_2_mins.mpam[1].residency_uS = 500;

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 7000);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    data_smpl_finalize_pwr_pkg_metrics();

    // MPAM 0: Should have called comp_metrics_for_mpam_throttling with residency_diff = 7000 - 2000 = 5000
    // and updated timestamp to 7000
    assert_int_equal(mpam_rt[mpam_id_1].residency_timestamp_uS, 7000);
    // The comp_metrics_for_mpam_throttling function should have been called with mpam_id=0, residency_diff_uS=5000, nominal_pstate=3

    // MPAM 2: Should have called comp_metrics_for_mpam_throttling with residency_diff = 7000 - 1500 = 5500
    // and updated timestamp to 7000
    assert_int_equal(mpam_rt[mpam_id_2].residency_timestamp_uS, 7000);
    // The comp_metrics_for_mpam_throttling function should have been called with mpam_id=2, residency_diff_uS=5500, nominal_pstate=5

    // MPAM 1: Should NOT have been processed (throttling=false), so timestamp should remain unchanged
    assert_int_equal(mpam_rt[1].residency_timestamp_uS, 3000);
    // The computed_metrics_2_mins.mpam[1].residency_uS should remain unchanged at 500
    assert_int_equal(computed_metrics_2_mins.mpam[1].residency_uS, 500);
}

TEST_FUNCTION(test_data_smpl_finalize_pwr_pkg_throttling_metrics, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t time_stamp_uS = 5000;

    // Test Case 1: Single throttle source active (temperature throttling)
    uint8_t throttle_source = THROTTLE_SOURCE_TEMPERATURE; // throttle_source = 1
    core_rt[core_id].throttle_source_tracker[throttle_source] = true;
    core_rt[core_id].throttle_res_timestamp_uS[throttle_source] = 1000;

    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS = 10;

    data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &time_stamp_uS);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS, 4010);

    // Test Case 2: Multiple throttle sources active (current + adaptive clock)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&computed_metrics_2_mins.cores[core_id], 0, sizeof(computed_metrics_2_mins.cores[core_id]));

    uint8_t current_source = THROTTLE_SOURCE_CURRENT;           // throttle_source = 0
    uint8_t adaptive_clk_source = THROTTLE_SOURCE_ADAPTIVE_CLK; // throttle_source = 4

    core_rt[core_id].throttle_source_tracker[current_source] = true;
    core_rt[core_id].throttle_source_tracker[adaptive_clk_source] = true;
    core_rt[core_id].throttle_res_timestamp_uS[current_source] = 2000;
    core_rt[core_id].throttle_res_timestamp_uS[adaptive_clk_source] = 1500;

    computed_metrics_2_mins.cores[core_id].throttle_info[current_source].residency_uS = 100;
    computed_metrics_2_mins.cores[core_id].throttle_info[adaptive_clk_source].residency_uS = 200;

    data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &time_stamp_uS);

    // Both throttle sources should have been updated
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[current_source].residency_uS, 3100);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[adaptive_clk_source].residency_uS, 3700);

    // Test Case 3: Rack throttle active
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&computed_metrics_2_mins.cores[core_id], 0, sizeof(computed_metrics_2_mins.cores[core_id]));

    uint8_t rack_priority = 2;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].latest_rack_throttle_priority = rack_priority;
    core_rt[core_id].rack_pri_res_timestamp_uS[rack_priority] = 1800;

    computed_metrics_2_mins.cores[core_id].rack_priorities[rack_priority].residency_uS = 50;

    data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &time_stamp_uS);

    // Rack throttle metrics should have been updated
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[rack_priority].residency_uS, 3250);

    // Test Case 4: Both regular throttle sources and rack throttle active
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&computed_metrics_2_mins.cores[core_id], 0, sizeof(computed_metrics_2_mins.cores[core_id]));

    uint8_t vr_hot_source = THROTTLE_SOURCE_VR_HOT; // throttle_source = 3
    core_rt[core_id].throttle_source_tracker[vr_hot_source] = true;
    core_rt[core_id].throttle_res_timestamp_uS[vr_hot_source] = 2500;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].latest_rack_throttle_priority = 1;
    core_rt[core_id].rack_pri_res_timestamp_uS[1] = 2200;

    computed_metrics_2_mins.cores[core_id].throttle_info[vr_hot_source].residency_uS = 300;
    computed_metrics_2_mins.cores[core_id].rack_priorities[1].residency_uS = 400;

    data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &time_stamp_uS);

    // Both regular and rack throttle metrics should have been updated
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[vr_hot_source].residency_uS, 2800);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[1].residency_uS, 3200);

    // Test Case 5: No throttle sources active (should not crash, no changes)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&computed_metrics_2_mins.cores[core_id], 0, sizeof(computed_metrics_2_mins.cores[core_id]));

    // All throttle_source_tracker values are false by default
    // rack_throttle_is_active is false by default

    data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &time_stamp_uS);

    // Should not crash and no metrics should change from zero
    for (uint8_t i = 0; i <= THROTTLE_SOURCE_ADAPTIVE_CLK; i++)
    {
        assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[i].residency_uS, 0);
    }
}

TEST_FUNCTION(test_data_smpl_get_active_throttle_sources, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    { // make all active
        core_rt[core_id].throttle_source_tracker[i] = true;
    }
    // Call the function to be tested
    uint8_t active_throttlings = data_smpl_get_active_throttle_sources(core_id);
    assert_int_equal(active_throttlings, 0x7F);

    // make all inactive
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    {
        core_rt[core_id].throttle_source_tracker[i] = false;
    }
    active_throttlings = data_smpl_get_active_throttle_sources(core_id);

    assert_int_equal(active_throttlings, 0);
}

TEST_FUNCTION(test_data_smpl_handle_throttle_source_start, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    core_state_entry_data_t entry_data = {0};
    entry_data.packet_timestamp_uS = 5000;

    // Test case 1: Normal throttle source start (THROTTLE_SOURCE_TEMPERATURE)
    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_TEMPERATURE, &entry_data);

    // Verify tracker is set to true
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // Verify timestamp is latched
    assert_int_equal(core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE], 5000);
    // Verify throttle is active
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    // Verify entry_data fields are set correctly
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_TEMPERATURE);
    assert_true(entry_data.throttling_state_change);
    assert_true(entry_data.throttle_start);

    // Test case 2: Second start on same source (should continue residency)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 8000;

    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_TEMPERATURE, &entry_data);

    // Verify tracker remains true
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // Verify timestamp is not updated (unexpected to get another start without end)
    assert_int_equal(core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE], 5000);
    // Verify throttle is still active
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    // entry_data should not be updated for unexpected start
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);

    // Test case 3: Start different throttle source (THROTTLE_SOURCE_CURRENT)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 10000;

    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_CURRENT, &entry_data);

    // Verify new tracker is set to true
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    // Verify new timestamp is latched
    assert_int_equal(core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT], 10000);
    // Verify throttle is still active
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    // Verify entry_data fields are set correctly for new source
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_CURRENT);
    assert_true(entry_data.throttling_state_change);
    assert_true(entry_data.throttle_start);

    // Test case 4: Overrun throttle sources (should return early)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 12000;

    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_CURRENT_OVERRUN, &entry_data);

    // Verify overrun tracker is not set (residencies not tracked for overrun)
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT_OVERRUN]);
    // Verify entry_data remains unchanged
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);

    // Test case 5: Adaptive clock overrun
    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN, &entry_data);

    // Verify overrun tracker is not set
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN]);
    // Verify entry_data remains unchanged
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);
}

// Unit test function for data_smpl_handle_throttle_source_end
TEST_FUNCTION(test_data_smpl_handle_throttle_source_end, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    core_state_entry_data_t entry_data = {0};

    // Clear all throttle source trackers to ensure clean test state
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    {
        core_rt[core_id].throttle_source_tracker[i] = false;
    }
    core_rt[core_id].status_flags.throttle_is_active = false;

    // Setup: Start a throttle source first
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE] = 5000;
    core_rt[core_id].status_flags.throttle_is_active = true;

    // Test case 1: Normal throttle source end with valid timestamp
    entry_data.packet_timestamp_uS = 8000;

    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_TEMPERATURE, &entry_data);

    // Verify tracker is set to false
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // Verify entry_data fields are set correctly
    assert_int_equal(entry_data.throttle_time_diff_uS, 3000); // 8000 - 5000
    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_TEMPERATURE);
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);
    // Verify throttle is no longer active (no other throttle sources active)
    assert_false(core_rt[core_id].status_flags.throttle_is_active);

    // Test case 2: End throttle source that is not active (should not crash)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 10000;

    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_CURRENT, &entry_data);

    // Verify tracker remains false
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    // Verify entry_data is not updated when throttle source is not active
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);

    // Test case 3: Invalid timestamp (timestamp goes backwards)
    // Setup another throttle source
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT] = 15000;
    core_rt[core_id].status_flags.throttle_is_active = true;

    entry_data = {0};                       // Reset entry_data
    entry_data.packet_timestamp_uS = 12000; // Earlier than start timestamp

    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_CURRENT, &entry_data);

    // Verify tracker is set to false even with invalid timestamp
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    // Verify time_diff is 0 when timestamp is invalid
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_CURRENT);
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);

    // Test case 4: Multiple throttle sources active, end one
    // Setup multiple throttle sources
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE] = 20000;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_ADAPTIVE_CLK] = 21000;
    core_rt[core_id].status_flags.throttle_is_active = true;

    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 25000;

    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_TEMPERATURE, &entry_data);

    // Verify only the specified tracker is set to false
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK]); // Should remain active
    // Verify entry_data fields are set correctly
    assert_int_equal(entry_data.throttle_time_diff_uS, 5000); // 25000 - 20000
    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_TEMPERATURE);
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);
    // Verify throttle is still active (other source still active)
    assert_true(core_rt[core_id].status_flags.throttle_is_active);

    // Test case 5: Overrun throttle sources (should return early)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 30000;

    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_CURRENT_OVERRUN, &entry_data);

    // Verify entry_data remains unchanged for overrun sources
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);

    // Test case 6: Adaptive clock overrun
    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN, &entry_data);

    // Verify entry_data remains unchanged for overrun sources
    assert_int_equal(entry_data.throttle_time_diff_uS, 0);
    assert_int_equal(entry_data.throttle_source, 0);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.throttle_start);
}

// Unit test function for data_smpl_handle_rack_throttle_start
TEST_FUNCTION(test_data_smpl_handle_rack_throttle_start, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    core_state_entry_data_t entry_data = {0};

    // Clear all throttle source trackers and rack throttle state to ensure clean test state
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    {
        core_rt[core_id].throttle_source_tracker[i] = false;
    }
    core_rt[core_id].status_flags.throttle_is_active = false;
    core_rt[core_id].status_flags.rack_throttle_is_active = false;
    core_rt[core_id].latest_rack_throttle_priority = 0;

    // Test case 1: Invalid priority (should return early)
    entry_data.packet_timestamp_uS = 5000;

    data_smpl_handle_rack_throttle_start(core_id, NUMBER_OF_RACK_THROTTLE_PRIORITIES, &entry_data);

    // Verify no state changes occurred due to invalid priority
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);

    // Test case 2: First rack throttle start (no previous rack throttling)
    uint8_t new_priority = 3;
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 10000;

    data_smpl_handle_rack_throttle_start(core_id, new_priority, &entry_data);

    // Verify rack throttle state is set
    assert_true(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, new_priority);
    // Verify timestamp is latched for new priority
    assert_int_equal(core_rt[core_id].rack_pri_res_timestamp_uS[new_priority], 10000);
    // Verify entry_data fields for new rack throttle start
    assert_true(entry_data.rack_priority_start);
    assert_true(entry_data.rack_throttling_state_change);
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 0); // starting so no time diff

    // Test case 3: Rack throttle already active with same priority (continue residency)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 15000;

    data_smpl_handle_rack_throttle_start(core_id, new_priority, &entry_data); // Same priority

    // Verify rack throttle state remains active
    assert_true(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, new_priority);
    // Verify entry_data shows continuation (no priority start)
    assert_false(entry_data.rack_priority_start);
    assert_false(entry_data.rack_throttling_state_change);

    // Test case 4: Rack throttle active with different priority (priority change)
    uint8_t different_priority = 5;
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 20000;

    data_smpl_handle_rack_throttle_start(core_id, different_priority, &entry_data);

    // Verify rack throttle remains active but priority changed
    assert_true(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, different_priority);
    // Verify time diff calculated from previous priority timestamp
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 10000); // 20000 - 10000
    // Verify new priority timestamp is latched
    assert_int_equal(core_rt[core_id].rack_pri_res_timestamp_uS[different_priority], 20000);
    // Verify entry_data shows priority change
    assert_true(entry_data.rack_priority_start);
    assert_true(entry_data.rack_throttling_state_change);

    // Test case 5: Edge case - priority 0 (valid minimum)
    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 25000;

    data_smpl_handle_rack_throttle_start(core_id, 0, &entry_data);

    // Verify priority 0 is accepted
    assert_true(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);
    assert_int_equal(core_rt[core_id].rack_pri_res_timestamp_uS[0], 25000);
}

// Unit test function for data_smpl_handle_rack_throttle_end
TEST_FUNCTION(test_data_smpl_handle_rack_throttle_end, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    core_state_entry_data_t entry_data = {0};

    // Clear all throttle source trackers and rack throttle state to ensure clean test state
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    {
        core_rt[core_id].throttle_source_tracker[i] = false;
    }
    core_rt[core_id].status_flags.throttle_is_active = false;
    core_rt[core_id].status_flags.rack_throttle_is_active = false;
    core_rt[core_id].latest_rack_throttle_priority = 0;

    // Test case 1: End rack throttle when not active (should not crash)
    entry_data.packet_timestamp_uS = 5000;

    data_smpl_handle_rack_throttle_end(core_id, &entry_data);

    // Verify rack throttle state remains inactive
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);
    // Verify entry_data is not updated when rack throttle is not active
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 0);
    assert_false(entry_data.rack_throttling_state_change);
    assert_false(entry_data.rack_priority_start);

    // Test case 2: Normal rack throttle end with valid timestamp
    // Setup active rack throttle first
    uint8_t test_priority = 3;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].latest_rack_throttle_priority = test_priority;
    core_rt[core_id].rack_pri_res_timestamp_uS[test_priority] = 10000;

    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 15000;

    data_smpl_handle_rack_throttle_end(core_id, &entry_data);

    // Verify rack throttle state is deactivated
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);
    // Verify entry_data fields are set correctly
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 5000); // 15000 - 10000
    assert_true(entry_data.rack_throttling_state_change);
    assert_false(entry_data.rack_priority_start); // rack throttling ended

    // Test case 3: Invalid timestamp (timestamp goes backwards)
    // Setup active rack throttle again
    test_priority = 2;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].latest_rack_throttle_priority = test_priority;
    core_rt[core_id].rack_pri_res_timestamp_uS[test_priority] = 20000;

    entry_data = {0};                       // Reset entry_data
    entry_data.packet_timestamp_uS = 18000; // Earlier than start timestamp

    data_smpl_handle_rack_throttle_end(core_id, &entry_data);

    // Verify rack throttle state is still deactivated even with invalid timestamp
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);
    // Verify time_diff is 0 when timestamp is invalid
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 0);
    assert_true(entry_data.rack_throttling_state_change);
    assert_false(entry_data.rack_priority_start);

    // Test case 4: End rack throttle with priority 0 (edge case)
    // Setup active rack throttle with priority 0
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].latest_rack_throttle_priority = 0;
    core_rt[core_id].rack_pri_res_timestamp_uS[0] = 25000;

    entry_data = {0}; // Reset entry_data
    entry_data.packet_timestamp_uS = 30000;

    data_smpl_handle_rack_throttle_end(core_id, &entry_data);

    // Verify rack throttle state is deactivated
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 0);
    // Verify time calculation works with priority 0
    assert_int_equal(entry_data.rack_throttle_time_diff_uS, 5000); // 30000 - 25000
    assert_true(entry_data.rack_throttling_state_change);
    assert_false(entry_data.rack_priority_start);
}

TEST_FUNCTION(test_data_smpl_update_soc_avg_pstate, test_setup, test_teardown)
{
    // Initialize all core runtime info to ensure clean test state
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    }

    // Set up test data with mixed cstate values for a few representative cores
    uint8_t test_core_id_1 = 0;
    uint8_t test_core_id_2 = 1;
    uint8_t test_core_id_3 = 2;

    // Test Case 1: Core with cstate >= CSTATE_C2 should get INVALID_PSTATE
    core_rt[test_core_id_1].latest_cstate = CSTATE_C2; // Should get INVALID_PSTATE
    core_rt[test_core_id_1].latest_pstate = 5; // This should be used for cores with cstate < CSTATE_C2

    // Test Case 2: Core with cstate < CSTATE_C2 should keep original pstate
    core_rt[test_core_id_2].latest_cstate = CSTATE_C1; // Should keep pstate
    core_rt[test_core_id_2].latest_pstate = 10;        // This should remain unchanged

    // Test Case 3: Core with cstate > CSTATE_C2 should get INVALID_PSTATE
    core_rt[test_core_id_3].latest_cstate = CSTATE_C3; // Should get INVALID_PSTATE
    core_rt[test_core_id_3].latest_pstate = 15; // This should be used for cores with cstate < CSTATE_C2

    // Call the function under test
    // The function creates a local latest_pstate array, populates it based on cstate conditions,
    // and passes it to comp_metrics_for_soc_avg_pstate() - we can't assert the local array contents
    data_smpl_update_soc_avg_pstate();

    // Verify that the original core_rt values remain unchanged
    // (the function should only modify the local latest_pstate array, not the core_rt structures)
    assert_int_equal(core_rt[test_core_id_1].latest_cstate, CSTATE_C2);
    assert_int_equal(core_rt[test_core_id_1].latest_pstate, 5);
    assert_int_equal(core_rt[test_core_id_2].latest_cstate, CSTATE_C1);
    assert_int_equal(core_rt[test_core_id_2].latest_pstate, 10);
    assert_int_equal(core_rt[test_core_id_3].latest_cstate, CSTATE_C3);
    assert_int_equal(core_rt[test_core_id_3].latest_pstate, 15);

    // Test Case 4: Edge case with CSTATE_C2 exactly (should result in INVALID_PSTATE in local array)
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_cstate = CSTATE_C2;
        core_rt[core_id].latest_pstate = core_id + 20; // Different pstate for each core
    }

    data_smpl_update_soc_avg_pstate();

    // Verify original values are preserved in core_rt (the function shouldn't modify core_rt)
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core_rt[core_id].latest_cstate, CSTATE_C2);
        assert_int_equal(core_rt[core_id].latest_pstate, core_id + 20);
    }
}

TEST_FUNCTION(test_data_smpl_terminate_non_rack_throttle_sources, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t test_timestamp = 50000; // 50ms

    // Initialize core runtime info
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    // Test Case 1: No active throttle sources - function should do nothing
    data_smpl_terminate_non_rack_throttle_sources(core_id, &test_timestamp);

    // Verify throttle_is_active remains false
    assert_false(core_rt[core_id].status_flags.throttle_is_active);

    // Test Case 2: Multiple non-rack throttle sources active
    // Set up active throttle sources (excluding rack limit and overrun sources)
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] = true; // Should be skipped

    // Set timestamps for active sources
    uint64_t base_timestamp = 10000; // 10ms
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT] = base_timestamp;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_TEMPERATURE] = base_timestamp + 5000;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_ADAPTIVE_CLK] = base_timestamp + 10000;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_RACK_LIMIT] = base_timestamp + 15000;

    core_rt[core_id].status_flags.throttle_is_active = true;

    // Call function under test
    // The function will call comp_metrics_for_single_core_single_throttle_source for each active non-rack
    // source but since it's not mocked, we can't verify the parameters - we can only verify the behavior
    data_smpl_terminate_non_rack_throttle_sources(core_id, &test_timestamp);

    // Verify non-rack throttle sources are deactivated
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK]);

    // Verify RACK_LIMIT is NOT deactivated (should be skipped)
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT]);

    // Verify throttle_is_active remains true (because RACK_LIMIT is still active)
    assert_true(core_rt[core_id].status_flags.throttle_is_active);

    // Test Case 3: Invalid timestamp (timestamp < throttle start timestamp)
    core_id = 1;
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    uint64_t invalid_timestamp = 5000; // Earlier than start timestamp
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT] = 10000; // Later than test timestamp
    core_rt[core_id].status_flags.throttle_is_active = true;

    // Call function under test - it will call comp_metrics_for_single_core_single_throttle_source
    // with time_diff_uS = 0 due to invalid timestamp, but we can't verify since it's not mocked
    data_smpl_terminate_non_rack_throttle_sources(core_id, &invalid_timestamp);

    // Verify source is deactivated
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);

    // Verify throttle_is_active is now false (no active sources)
    assert_false(core_rt[core_id].status_flags.throttle_is_active);
}

TEST_FUNCTION(test_data_smpl_parse_core_states_entry, test_setup, test_teardown)
{
    pstate_telem_t pstate_entry;
    core_state_entry_data_t entry_data;
    uint8_t core_id = 0;

    // Test Case 1: Invalid core_id (should return early)
    memset(&pstate_entry, 0, sizeof(pstate_entry));
    memset(&entry_data, 0, sizeof(entry_data));
    pstate_entry.data.core = NUMBER_OF_CORES_PER_DIE; // Invalid core_id
    pstate_entry.data.cstate = CSTATE_C0;
    pstate_entry.timestamp = 1000;

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);
    // Should return early, entry_data.packet_timestamp_uS should remain 0
    assert_int_equal(entry_data.packet_timestamp_uS, 0);
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.valid_entry_cstate);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 2: Invalid cstate (should return early)
    memset(&pstate_entry, 0, sizeof(pstate_entry));
    memset(&entry_data, 0, sizeof(entry_data));
    pstate_entry.data.core = core_id;
    pstate_entry.data.cstate = NUMBER_OF_CSTATES; // Invalid cstate
    pstate_entry.timestamp = 1000;

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);
    // Should return early, entry_data.packet_timestamp_uS should remain 0
    assert_int_equal(entry_data.packet_timestamp_uS, 0);
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.valid_entry_cstate);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 3: Transition from active to inactive core (C2+) with throttling active
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&pstate_entry, 0, sizeof(pstate_entry));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.core = core_id;
    pstate_entry.data.cstate = CSTATE_C2; // Inactive state
    pstate_entry.data.throttle_status = NO_THROTTLING;
    pstate_entry.timestamp = 1000;

    // Set up core as having been in active state with throttling
    core_rt[core_id].latest_cstate = CSTATE_C2; // Will trigger inactive path
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have called data_smpl_handle_rack_throttle_end and data_smpl_terminate_non_rack_throttle_sources
    assert_false(core_rt[core_id].status_flags.throttle_is_active);
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // No pstate processing in inactive state
    assert_false(entry_data.valid_entry_pstate);
    // Rack throttle end should have been called
    assert_true(entry_data.rack_throttling_state_change);
    // Non-rack throttle sources terminated but not through normal throttling_state_change
    assert_false(entry_data.throttling_state_change);

    // Test Case 4: Transition from active to inactive without throttling but with valid pstate
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&pstate_entry, 0, sizeof(pstate_entry));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.core = core_id;
    pstate_entry.data.cstate = CSTATE_C3; // Inactive state
    pstate_entry.data.pstate = 5;
    pstate_entry.timestamp = 1000;

    core_rt[core_id].latest_pstate = 6;
    core_rt[core_id].latest_cstate = CSTATE_C3; // Will trigger inactive path
    core_rt[core_id].status_flags.throttle_is_active = false;
    core_rt[core_id].status_flags.pkt_pstate_is_valid = true;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have set pkt_pstate_is_pending_invalid
    assert_true(core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // Should have called data_smpl_parse_pstate
    assert_true(entry_data.valid_entry_pstate);
    // on transition to low cstate, pstate_change is false, so not to utilize this packet pstate
    assert_false(entry_data.pstate_change);
    // No throttling changes in this path
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 5: NO_THROTTLING with throttling currently active (should terminate non-rack sources)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&pstate_entry, 0, sizeof(pstate_entry));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.core = core_id;
    pstate_entry.data.cstate = CSTATE_C0; // Active state
    pstate_entry.data.throttle_status = NO_THROTTLING;
    pstate_entry.data.pstate = 3;
    pstate_entry.timestamp = 1000;

    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have called data_smpl_terminate_non_rack_throttle_sources
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // Should have called data_smpl_parse_cstate and data_smpl_parse_pstate (NO_THROTTLING case)
    assert_true(entry_data.valid_entry_cstate);
    assert_true(entry_data.valid_entry_pstate);
    // No direct throttling state change from this case
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 6: CURRENT_THROTTLING_OVERRUN
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = CURRENT_THROTTLING_OVERRUN;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_CURRENT_OVERRUN);
    assert_true(entry_data.overrun_count_change);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // No pstate or throttling state changes for overrun
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 7: ADPT_CLK_THROTTLING_OVERRUN
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = ADPT_CLK_THROTTLING_OVERRUN;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_int_equal(entry_data.throttle_source, THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN);
    assert_true(entry_data.overrun_count_change);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // No pstate or throttling state changes for overrun
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 8: RACK_THROTTLING_START
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = RACK_THROTTLING_START;
    pstate_entry.data.vm_throttle_pri = 2;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have called data_smpl_handle_rack_throttle_start - verify some expected state
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // complete previous pstate residency on transition to rack throttle start
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling and rack throttling state changes
    assert_true(entry_data.throttling_state_change);
    assert_true(entry_data.rack_throttling_state_change);

    // Test Case 9: TEMP_THROTTLING_START
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = TEMP_THROTTLING_START;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have called data_smpl_handle_throttle_source_start - verify expected state
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // complete previous pstate residency on transition to rack throttle start
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 10: CURRENT_THROTTLING_START
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = CURRENT_THROTTLING_START;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // complete previous pstate residency on transition to rack throttle start
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 11: ADPT_CLK_THROTTLING_START
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = ADPT_CLK_THROTTLING_START;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // complete previous pstate residency on transition to rack throttle start
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 11a: RACK_THROTTLING_END
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    // Set up as currently rack throttling
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].status_flags.rack_throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT] = true;
    core_rt[core_id].latest_rack_throttle_priority = 1;
    core_rt[core_id].rack_pri_res_timestamp_uS[1] = 500; // Earlier timestamp

    pstate_entry.data.throttle_status = RACK_THROTTLING_END;
    pstate_entry.timestamp = 1000;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_RACK_LIMIT]);
    assert_false(core_rt[core_id].status_flags.rack_throttle_is_active);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // start tracking pstate now
    assert_true(entry_data.valid_entry_pstate);
    // Should have both throttling and rack throttling state changes
    assert_true(entry_data.throttling_state_change);
    assert_true(entry_data.rack_throttling_state_change);

    // Test Case 11b: ADPT_CLK_THROTTLING_END
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    // Set up as currently adaptive clock throttling
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_ADAPTIVE_CLK] = 500; // Earlier timestamp

    pstate_entry.data.throttle_status = ADPT_CLK_THROTTLING_END;
    pstate_entry.timestamp = 1000;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_ADAPTIVE_CLK]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // start tracking pstate now
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 11c: CURRENT_THROTTLING_END
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    // Set up as currently current throttling
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_CURRENT] = 500; // Earlier timestamp

    pstate_entry.data.throttle_status = CURRENT_THROTTLING_END;
    pstate_entry.timestamp = 1000;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_CURRENT]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // start tracking pstate now
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 12: Transition from not throttling to throttling (pstate invalidation)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = TEMP_THROTTLING_START;
    pstate_entry.data.pstate = 7;
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].latest_pstate = 5;                       // Previous pstate
    core_rt[core_id].status_flags.throttle_is_active = false; // Not throttling initially

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have transitioned to throttling and set pstate_is_pending_invalid
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    assert_true(core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid);
    assert_int_equal(core_rt[core_id].latest_pstate, 5); // Should restore previous pstate
    assert_false(entry_data.pstate_change);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // Should have called data_smpl_parse_pstate (transition to throttling)
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 13: Transition from throttling to not throttling (non-NO_THROTTLING case)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    // Set up as currently throttling
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE] = true;

    pstate_entry.data.throttle_status = TEMP_THROTTLING_END;
    pstate_entry.data.pstate = 4;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have ended throttling and called data_smpl_parse_pstate at the end
    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);
    // The exact behavior depends on whether other sources are still active,
    // but we've covered the path
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // Should have called data_smpl_parse_pstate (throttling ended, non-NO_THROTTLING case)
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 14: SYS_FRC_PMIN_THROTTLING_START (VR HOT throttling start)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = SYS_FRC_PMIN_THROTTLING_START;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should have called data_smpl_handle_throttle_source_start - verify expected state
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_VR_HOT]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // complete previous pstate residency on transition to throttle start
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 15: SYS_FRC_PMIN_THROTTLING_END (VR HOT throttling end)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    // Set up as currently VR HOT throttling
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].status_flags.throttle_is_active = true;
    core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_VR_HOT] = true;
    core_rt[core_id].throttle_res_timestamp_uS[THROTTLE_SOURCE_VR_HOT] = 500; // Earlier timestamp

    pstate_entry.data.throttle_status = SYS_FRC_PMIN_THROTTLING_END;
    pstate_entry.timestamp = 1000;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    assert_false(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_VR_HOT]);
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // start tracking pstate now
    assert_true(entry_data.valid_entry_pstate);
    // Should have throttling state change but not rack throttling
    assert_true(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);

    // Test Case 16: Inactive core (should return early)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.core = core_id;
    pstate_entry.data.cstate = CSTATE_C0;
    pstate_entry.data.pstate = 5;
    pstate_entry.data.throttle_status = NO_THROTTLING;
    pstate_entry.timestamp = 1000;

    core_is_active[core_id] = false; // Set core as inactive

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should return early, entry_data fields should remain 0
    assert_int_equal(entry_data.packet_timestamp_uS, 0);
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.valid_entry_cstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);
    assert_false(entry_data.pstate_change);
    assert_false(entry_data.cstate_change);

    // Reset core_is_active for other tests
    core_is_active[core_id] = true;
}

TEST_FUNCTION(test_data_smpl_process_pstate_sensor_fifo, test_setup, test_teardown)
{
    pstate_telem_t pstate_data;
    uint8_t core_id = 0;

    // Clear all core runtime info for clean testing
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    // Test Case 1: Invalid data (curr_data_is_valid = false) - should skip processing
    memset(&pstate_data, 0, sizeof(pstate_data));
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &invalid_entry);

    data_smpl_process_pstate_sensor_fifo();

    // No state should change since data is invalid
    assert_int_equal(core_rt[core_id].latest_pstate, 0);

    // Test Case 2: Valid data with pstate processing
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&pstate_data, 0, sizeof(pstate_data));

    pstate_data.data.core = core_id;
    pstate_data.data.cstate = CSTATE_C0;
    pstate_data.data.pstate = 5;
    pstate_data.data.throttle_status = NO_THROTTLING;
    pstate_data.timestamp = 1000;

    sensor_ram_poll_status_t valid_entry = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &valid_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // For timestamp conversion
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);
    data_smpl_process_pstate_sensor_fifo();

    // Should have processed pstate and cstate
    assert_int_equal(core_rt[core_id].latest_pstate, 5);
    assert_int_equal(core_rt[core_id].latest_cstate, CSTATE_C0);

    // Test Case 3: Valid data with throttling state change
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    pstate_data.data.throttle_status = TEMP_THROTTLING_START;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    sensor_ram_poll_status_t throttle_entry = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &throttle_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_smpl_process_pstate_sensor_fifo();

    // Should have activated throttling
    assert_true(core_rt[core_id].status_flags.throttle_is_active);
    assert_true(core_rt[core_id].throttle_source_tracker[THROTTLE_SOURCE_TEMPERATURE]);

    // Test Case 4: Valid data with rack throttling
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    pstate_data.data.throttle_status = RACK_THROTTLING_START;
    pstate_data.data.vm_throttle_pri = 2;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    sensor_ram_poll_status_t rack_throttle_entry = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &rack_throttle_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_smpl_process_pstate_sensor_fifo();

    // Should have activated rack throttling
    assert_true(core_rt[core_id].status_flags.rack_throttle_is_active);
    assert_int_equal(core_rt[core_id].latest_rack_throttle_priority, 2);

    // Test Case 5: Valid data with overrun count change
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    pstate_data.data.throttle_status = CURRENT_THROTTLING_OVERRUN;
    core_rt[core_id].latest_cstate = CSTATE_C0;

    sensor_ram_poll_status_t overrun_entry = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &overrun_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_smpl_process_pstate_sensor_fifo();

    // Should have processed overrun (no specific state to check, just ensure no crash)

    // Test Case 6: Valid data with pstate pending invalid
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    pstate_data.data.throttle_status = TEMP_THROTTLING_START;
    pstate_data.data.pstate = 7;
    core_rt[core_id].latest_cstate = CSTATE_C0;
    core_rt[core_id].latest_pstate = 5;
    core_rt[core_id].status_flags.throttle_is_active = false;

    sensor_ram_poll_status_t pstate_pending_entry = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_pending_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_smpl_process_pstate_sensor_fifo();

    // Should have set pstate pending invalid and then cleared it
    assert_false(core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid);
    assert_false(core_rt[core_id].status_flags.pkt_pstate_is_valid);

    // Test Case 7: Multiple entries (more_entries = true)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    pstate_data.data.throttle_status = NO_THROTTLING;
    pstate_data.data.pstate = 3;

    // First call with more_entries = true
    sensor_ram_poll_status_t first_entry = {.curr_data_is_valid = true, .more_entries = true};
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &first_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);

    // Second call with more_entries = false
    sensor_ram_poll_status_t second_entry = {.curr_data_is_valid = true, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &second_entry);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &pstate_data);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);

    data_smpl_process_pstate_sensor_fifo();

    // Should have processed both entries
    assert_int_equal(core_rt[core_id].latest_pstate, 3);
}

TEST_FUNCTION(test_data_smpl_calculate_mpam_data_from_cores, test_setup, test_teardown)
{
    // Test the MPAM data calculation function that handles both primary and secondary die logic

    // Clear all MPAM staging data and core runtime info for clean test state
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    // Test Case 1: Secondary die behavior (non-primary die)
    // Initialize as secondary die (die_id = 1)
    die_2_die_exch_init(1);

    // Set up some cores with different MPAM IDs and states
    uint8_t test_core_0 = 0;
    uint8_t test_core_1 = 1;
    uint8_t test_core_2 = 2;

    // Core 0: Active core with MPAM ID 5
    core_rt[test_core_0].latest_mpam_id = 5;
    core_rt[test_core_0].latest_power_mW = 1000;
    core_rt[test_core_0].latest_pstate = 10;
    core_rt[test_core_0].status_flags.throttle_is_active = true;
    core_is_active[test_core_0] = true;

    // Core 1: Active core with MPAM ID 7
    core_rt[test_core_1].latest_mpam_id = 7;
    core_rt[test_core_1].latest_power_mW = 1500;
    core_rt[test_core_1].latest_pstate = 8;
    core_rt[test_core_1].status_flags.throttle_is_active = false;
    core_is_active[test_core_1] = true;

    // Core 2: Inactive core with MPAM ID 5 (should be ignored)
    core_rt[test_core_2].latest_mpam_id = 5;
    core_rt[test_core_2].latest_power_mW = 2000; // Should be ignored
    core_rt[test_core_2].latest_pstate = 12;
    core_rt[test_core_2].status_flags.throttle_is_active = true;
    core_is_active[test_core_2] = false; // Inactive

    // Mock the die_2_die_exch_ib_write_pwr_pkg_mpam_data function for secondary die
    // Note: die_2_die_exch functions are included in test build, not mocked

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify MPAM staging data was populated correctly for secondary die
    // MPAM 5: Only core 0 should contribute (core 2 is inactive)
    assert_true(mpam_staging[5].active);
    assert_true(mpam_staging[5].throttling);                     // Core 0 is throttling
    assert_int_equal(mpam_staging[5].latest_total_pwr_mW, 1000); // Only core 0's power
    assert_int_equal(mpam_staging[5].latest_pstate, 10);         // Core 0's pstate

    // MPAM 7: Core 1 should contribute
    assert_true(mpam_staging[7].active);
    assert_false(mpam_staging[7].throttling);                    // Core 1 is not throttling
    assert_int_equal(mpam_staging[7].latest_total_pwr_mW, 1500); // Core 1's power
    assert_int_equal(mpam_staging[7].latest_pstate, 8);          // Core 1's pstate

    // All other MPAMs should remain inactive
    for (uint8_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        if (mpam_id != 5 && mpam_id != 7)
        {
            assert_false(mpam_staging[mpam_id].active);
            assert_false(mpam_staging[mpam_id].throttling);
            assert_int_equal(mpam_staging[mpam_id].latest_total_pwr_mW, 0);
            assert_int_equal(mpam_staging[mpam_id].latest_pstate, 0);
        }
    }

    // Test Case 2: Primary die behavior
    // Reset MPAM staging and core runtime
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    // Initialize as primary die (die_id = 0)
    die_2_die_exch_init(0);

    // Set up primary die cores
    uint8_t primary_core_0 = 0;
    uint8_t primary_core_1 = 1;

    // Core 0: MPAM ID 3
    core_rt[primary_core_0].latest_mpam_id = 3;
    core_rt[primary_core_0].latest_power_mW = 800;
    core_rt[primary_core_0].latest_pstate = 15;
    core_rt[primary_core_0].status_flags.throttle_is_active = false;
    core_is_active[primary_core_0] = true;

    // Core 1: MPAM ID 3 (same as core 0, should accumulate)
    core_rt[primary_core_1].latest_mpam_id = 3;
    core_rt[primary_core_1].latest_power_mW = 600;
    core_rt[primary_core_1].latest_pstate = 15; // Same pstate
    core_rt[primary_core_1].status_flags.throttle_is_active = true;
    core_is_active[primary_core_1] = true;

    // Note: die_2_die_exch functions are included in test build, not mocked
    // This will read actual secondary die data (which should be empty/zero for this test)

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 20000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify MPAM staging data for primary die cores
    // MPAM 3: Should have data from both primary cores (0,1)
    assert_true(mpam_staging[3].active);
    assert_true(mpam_staging[3].throttling); // Should be OR'd (primary core 1 is throttling)
    assert_int_equal(mpam_staging[3].latest_total_pwr_mW, 1400); // 800 + 600
    assert_int_equal(mpam_staging[3].latest_pstate, 15);         // Core pstate

    // Test Case 3: Invalid MPAM ID handling
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    die_2_die_exch_init(1); // Secondary die for simpler test

    // Set up core with invalid MPAM ID
    uint8_t invalid_core = 0;
    core_rt[invalid_core].latest_mpam_id = NUMBER_OF_MPAMS; // Invalid (out of range)
    core_rt[invalid_core].latest_power_mW = 500;
    core_rt[invalid_core].latest_pstate = 5;
    core_rt[invalid_core].status_flags.throttle_is_active = true;
    core_is_active[invalid_core] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 30000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify that invalid MPAM ID doesn't corrupt any valid MPAM data
    // All MPAM entries should remain at their default (zero) state
    for (uint8_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        assert_false(mpam_staging[mpam_id].active);
        assert_false(mpam_staging[mpam_id].throttling);
        assert_int_equal(mpam_staging[mpam_id].latest_total_pwr_mW, 0);
        assert_int_equal(mpam_staging[mpam_id].latest_pstate, 0);
    }

    // Test Case 4: Edge case - MPAM ID at boundary (valid maximum)
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    die_2_die_exch_init(1); // Secondary die

    // Set up core with maximum valid MPAM ID
    uint8_t boundary_core = 0;
    core_rt[boundary_core].latest_mpam_id = NUMBER_OF_MPAMS - 1; // Maximum valid MPAM ID
    core_rt[boundary_core].latest_power_mW = 750;
    core_rt[boundary_core].latest_pstate = 20;
    core_rt[boundary_core].status_flags.throttle_is_active = false;
    core_is_active[boundary_core] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 40000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify maximum valid MPAM ID is handled correctly
    uint8_t max_mpam_id = NUMBER_OF_MPAMS - 1;
    assert_true(mpam_staging[max_mpam_id].active);
    assert_false(mpam_staging[max_mpam_id].throttling);
    assert_int_equal(mpam_staging[max_mpam_id].latest_total_pwr_mW, 750);
    assert_int_equal(mpam_staging[max_mpam_id].latest_pstate, 20);

    // Test Case 5: Multiple cores with same MPAM ID - power accumulation and throttling OR
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    die_2_die_exch_init(1); // Secondary die

    uint8_t mpam_test_id = 25;

    // Core 0: MPAM 25, not throttling
    core_rt[0].latest_mpam_id = mpam_test_id;
    core_rt[0].latest_power_mW = 300;
    core_rt[0].latest_pstate = 18;
    core_rt[0].status_flags.throttle_is_active = false;
    core_is_active[0] = true;

    // Core 1: MPAM 25, throttling (should OR with core 0)
    core_rt[1].latest_mpam_id = mpam_test_id;
    core_rt[1].latest_power_mW = 400;
    core_rt[1].latest_pstate = 16;
    core_rt[1].status_flags.throttle_is_active = true;
    core_is_active[1] = true;

    // Core 2: MPAM 25, not throttling
    core_rt[2].latest_mpam_id = mpam_test_id;
    core_rt[2].latest_power_mW = 200;
    core_rt[2].latest_pstate = 14;
    core_rt[2].status_flags.throttle_is_active = false;
    core_is_active[2] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 50000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify power accumulation and throttling OR logic
    assert_true(mpam_staging[mpam_test_id].active);
    assert_true(mpam_staging[mpam_test_id].throttling); // Should be true (OR of false|true|false)
    assert_int_equal(mpam_staging[mpam_test_id].latest_total_pwr_mW, 900); // 300 + 400 + 200
    assert_int_equal(mpam_staging[mpam_test_id].latest_pstate, 14);        // Last processed core's pstate

    // Test Case 6: No active cores
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    die_2_die_exch_init(1); // Secondary die

    // Set up cores but make them all inactive
    for (uint8_t core_id = 0; core_id < 4; core_id++)
    {
        core_rt[core_id].latest_mpam_id = core_id; // Different MPAM IDs
        core_rt[core_id].latest_power_mW = 100 + core_id * 100;
        core_rt[core_id].latest_pstate = 10 + core_id;
        core_rt[core_id].status_flags.throttle_is_active = (core_id % 2) == 0;
        core_is_active[core_id] = false; // All inactive
    }

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 60000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify that inactive cores don't contribute to MPAM data
    for (uint8_t mpam_id = 0; mpam_id < 4; mpam_id++)
    {
        assert_false(mpam_staging[mpam_id].active);
        assert_false(mpam_staging[mpam_id].throttling);
        assert_int_equal(mpam_staging[mpam_id].latest_total_pwr_mW, 0);
        assert_int_equal(mpam_staging[mpam_id].latest_pstate, 0);
    }
}

TEST_FUNCTION(test_data_smpl_get_cstate_tfa_timestamp, test_setup, test_teardown)
{
    // Test the function to retrieve C-state TFA timestamps for cores

    // Test 1 : for Die 0 (Primary)
    uint8_t core_id = 0;
    uint8_t cstate_timestamp_id = 0;
    die_2_die_exch_init(0);
    uint8_t die_offset = die_2_die_exch_get_this_die_id() * NUMBER_OF_CORES_PER_DIE;
    // init temp timestamp for each cstate entry/exit point
    uint64_t timestamp_cstate_uS[CSTATE_MAX_ID] = {1000, 1010, 1020, 1030, 1040, 1050};

    // write these timestamp on the cstate tfa buffers.
    for (cstate_timestamp_id = 0; cstate_timestamp_id < CSTATE_MAX_ID; cstate_timestamp_id++)
    {

        // Each core has a structure of cstate_instr_timestamp_t laid out consecutively
        // Size is sizeof(cstate_instr_timestamp_t) (packed) so use pointer arithmetic
        cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[core_id];

        core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS
    }

    uint64_t timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_PSCI);

    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_PSCI]);
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_PSCI);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_PSCI]);
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_HW_LOW_PWR);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_HW_LOW_PWR]);
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_HW_LOW_PWR);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_HW_LOW_PWR]);
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_CFLUSH);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_CFLUSH]);
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_CFLUSH);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_CFLUSH]);

    // Test 2 : for Die 1 (Secondary)
    core_id = 0;
    die_2_die_exch_init(1);
    die_offset = die_2_die_exch_get_this_die_id() * NUMBER_OF_CORES_PER_DIE;
    // init temp timestamp for each cstate entry/exit point
    memset(timestamp_cstate_uS, 0, sizeof(timestamp_cstate_uS));
    for (uint8_t i = 0; i < CSTATE_MAX_ID; i++)
    {
        timestamp_cstate_uS[i] = 2000 + i * 10;
    }
    // write these timestamp on the cstate tfa buffers.
    for (cstate_timestamp_id = 0; cstate_timestamp_id < CSTATE_MAX_ID; cstate_timestamp_id++)
    {

        // Each core has a structure of cstate_instr_timestamp_t laid out consecutively
        // Size is sizeof(cstate_instr_timestamp_t) (packed) so use pointer arithmetic
        cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[core_id + die_offset];

        core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS
    }
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_PSCI);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_PSCI]);

    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_PSCI);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_PSCI]);

    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_HW_LOW_PWR);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_HW_LOW_PWR]);

    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_HW_LOW_PWR);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_HW_LOW_PWR]);

    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_ENTER_CFLUSH);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_ENTER_CFLUSH]);

    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_EXIT_CFLUSH);
    assert_int_equal(timestamp, timestamp_cstate_uS[CSTATE_EXIT_CFLUSH]);

    // Test 3 : Invalid core ID
    timestamp = data_smpl_get_cstate_tfa_timestamp(core_id, CSTATE_MAX_ID);
    assert_int_equal(timestamp, 0);
}

TEST_FUNCTION(test_data_smpl_process_cstate_entry_latency, test_setup, test_teardown)
{
    // Test the processing of C-state entry latency in case of a change and read timesstamps from TFA
    uint8_t core_id = 0;
    // Initialize the tfa buffer
    uint8_t cstate_timestamp_id = 0;
    die_2_die_exch_init(0);
    //  init temp timestamp for each cstate entry/exit point
    uint64_t timestamp_cstate_uS[CSTATE_MAX_ID] = {1000, 1010, 1020, 1030, 1040, 1050};
    cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[core_id];
    core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS

    // Clear all core runtime info for clean testing
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    core_rt[core_id].latest_cstate = 2;
    // Log the entry latency for the cstate we are entering
    uint64_t packet_timestamp_uS = 5000; // Example packet timestamp in microseconds
    data_smpl_process_cstate_entry_latency(core_id, core_rt[core_id].latest_cstate, packet_timestamp_uS);

    // Calculate expected latency: packet timestamp - cstate entry timestamp from TFA
    assert_int_equal(core_rt[core_id].latest_cstate_entry_latency_uS, packet_timestamp_uS - 1000);
}

TEST_FUNCTION(test_data_smpl_process_cstate_exit_latency, test_setup, test_teardown)
{
    // Test the processing of C-state exit latency in case of a change and read timesstamps from TFA
    uint8_t core_id = 0;
    // Initialize the tfa buffer
    uint8_t cstate_timestamp_id = CSTATE_EXIT_PSCI;
    // Clear all core runtime info for clean testing
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));

    core_rt[core_id].latest_cstate_exit_timestamp_uS = 2000;
    die_2_die_exch_init(0);
    //  init temp timestamp for each cstate entry/exit point
    uint64_t timestamp_cstate_uS[CSTATE_MAX_ID] = {3000, 3010, 3020, 3030, 3040, 3050};
    cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[core_id];
    core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS

    core_rt[core_id].latest_cstate = 1; // exited the previus deep cstate  and going to either C0 or C1.
    // Log the entry latency for the cstate we are entering
    data_smpl_process_cstate_exit_latency(core_id);

    // Calculate expected latency: packet timestamp - cstate entry timestamp from TFA
    assert_int_equal(core_rt[core_id].latest_cstate_exit_latency_uS,
                     timestamp_cstate_uS[CSTATE_EXIT_PSCI] - core_rt[core_id].latest_cstate_exit_timestamp_uS);
}

TEST_FUNCTION(test_data_smpl_calculate_mpam_throttling_transitions, test_setup, test_teardown)
{
    // Test the MPAM throttling transition detection logic added to data_smpl_calculate_mpam_data_from_cores
    // This test verifies both throttling start and end detection with proper metrics calls

    // Clear all data structures for clean test state
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(&mpam_rt, 0, sizeof(mpam_rt));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));

    // Initialize as primary die for MPAM transition testing (secondary die doesn't do transition logic)
    die_2_die_exch_init(0);

    // Test Case 1: Active+Throttling End Detection (active+throttling→inactive transition)
    // Set up MPAM runtime state showing previous active+throttling state
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));
    uint8_t test_mpam_id = 10;
    mpam_rt[test_mpam_id].status_flags.active = true;     // Previous state: active
    mpam_rt[test_mpam_id].status_flags.throttling = true; // Previous state: throttling
    mpam_rt[test_mpam_id].residency_timestamp_uS = 5000;  // Set initial throttling start time
    mpam_rt[test_mpam_id].nominal_pstate = 12;            // Set nominal pstate for metrics

    // Set up core that will create inactive MPAM staging state
    // (no active cores with this MPAM ID means mpam_staging will remain inactive)
    uint8_t test_core = 0;
    core_rt[test_core].latest_mpam_id = test_mpam_id + 1; // Different MPAM ID
    core_rt[test_core].latest_power_mW = 1000;
    core_rt[test_core].latest_pstate = 10;
    core_rt[test_core].status_flags.throttle_is_active = false;
    core_is_active[test_core] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    // Residency = 15000 - 5000 = 10000 microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 15000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify active+throttling end transition was detected
    // mpam_staging[test_mpam_id].active should be false (no cores with this MPAM ID)
    assert_false(mpam_staging[test_mpam_id].active);
    assert_false(mpam_staging[test_mpam_id].throttling);

    // Verify mpam_rt was synchronized with mpam_staging after transition handling
    assert_false(mpam_rt[test_mpam_id].status_flags.active);
    assert_false(mpam_rt[test_mpam_id].status_flags.throttling);

    // Verify comp_metrics_for_mpam_throttling was called for the active+throttling→inactive transition
    // Expected: residency_uS = 15000 - 5000 = 10000, nominal_pstate = 12
    assert_int_equal(computed_metrics_2_mins.mpam[test_mpam_id].residency_uS, 10000);
    assert_int_equal(computed_metrics_2_mins.mpam[test_mpam_id].nominal_pstate, 12);

    // Test Case 2: Throttling Start Detection (non-throttling→throttling transition)
    // Reset state for next test
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(&mpam_rt, 0, sizeof(mpam_rt));
    memset(core_rt, 0, sizeof(core_rt));

    uint8_t test_mpam_id2 = 15;
    // Set up MPAM runtime state showing previous non-throttling state
    mpam_rt[test_mpam_id2].status_flags.active = true;      // Previous state: active
    mpam_rt[test_mpam_id2].status_flags.throttling = false; // Previous state: not throttling

    // Set up core that will create throttling MPAM staging state
    core_rt[0].latest_mpam_id = test_mpam_id2;
    core_rt[0].latest_power_mW = 1500;
    core_rt[0].latest_pstate = 12;
    core_rt[0].status_flags.throttle_is_active = true; // This core is throttling
    core_is_active[0] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 25000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify throttling start transition was detected
    assert_true(mpam_staging[test_mpam_id2].active);
    assert_true(mpam_staging[test_mpam_id2].throttling);

    // Verify mpam_rt was synchronized with mpam_staging after transition handling
    assert_true(mpam_rt[test_mpam_id2].status_flags.active);
    assert_true(mpam_rt[test_mpam_id2].status_flags.throttling);

    // Test Case 3: Throttling End Detection (throttling→non-throttling transition)
    // Reset state for next test
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(&mpam_rt, 0, sizeof(mpam_rt));
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    uint8_t test_mpam_id3 = 20;
    // Set up MPAM runtime state showing previous throttling state
    mpam_rt[test_mpam_id3].status_flags.active = true;     // Previous state: active
    mpam_rt[test_mpam_id3].status_flags.throttling = true; // Previous state: throttling
    mpam_rt[test_mpam_id3].residency_timestamp_uS = 10000; // Set initial throttling start time
    mpam_rt[test_mpam_id3].nominal_pstate = 5;             // Set nominal pstate for metrics

    // Set up core that will create non-throttling MPAM staging state
    core_rt[0].latest_mpam_id = test_mpam_id3;
    core_rt[0].latest_power_mW = 800;
    core_rt[0].latest_pstate = 8;
    core_rt[0].nominal_pstate = 5;                      // Set core's nominal_pstate to match MPAM's
    core_rt[0].status_flags.throttle_is_active = false; // This core is not throttling
    core_is_active[0] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    // Residency = 35000 - 10000 = 25000 microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 35000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify throttling end transition was detected
    assert_true(mpam_staging[test_mpam_id3].active);
    assert_false(mpam_staging[test_mpam_id3].throttling);

    // Verify mpam_rt was synchronized with mpam_staging after transition handling
    assert_true(mpam_rt[test_mpam_id3].status_flags.active);
    assert_false(mpam_rt[test_mpam_id3].status_flags.throttling);

    // Verify comp_metrics_for_mpam_throttling was called and updated metrics correctly
    // Expected: residency_uS = 25000, nominal_pstate = 5
    assert_int_equal(computed_metrics_2_mins.mpam[test_mpam_id3].residency_uS, 25000);
    assert_int_equal(computed_metrics_2_mins.mpam[test_mpam_id3].nominal_pstate, 5);

    // Test Case 4: No Transition (steady state)
    // Reset state for next test
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(&mpam_rt, 0, sizeof(mpam_rt));
    memset(core_rt, 0, sizeof(core_rt));

    uint8_t test_mpam_id4 = 25;
    // Set up MPAM runtime state showing previous throttling state
    mpam_rt[test_mpam_id4].status_flags.active = true;     // Previous state: active
    mpam_rt[test_mpam_id4].status_flags.throttling = true; // Previous state: throttling

    // Set up core that will maintain throttling MPAM staging state
    core_rt[0].latest_mpam_id = test_mpam_id4;
    core_rt[0].latest_power_mW = 1200;
    core_rt[0].latest_pstate = 14;
    core_rt[0].status_flags.throttle_is_active = true; // This core is still throttling
    core_is_active[0] = true;

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 45000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify no transition was detected (steady state)
    assert_true(mpam_staging[test_mpam_id4].active);
    assert_true(mpam_staging[test_mpam_id4].throttling);

    // Verify mpam_rt was synchronized with mpam_staging (should remain the same)
    assert_true(mpam_rt[test_mpam_id4].status_flags.active);
    assert_true(mpam_rt[test_mpam_id4].status_flags.throttling);

    // Test Case 5: Multiple MPAM IDs with mixed transitions
    // Reset state for comprehensive test
    memset(&mpam_staging, 0, sizeof(mpam_staging));
    memset(&mpam_rt, 0, sizeof(mpam_rt));
    memset(core_rt, 0, sizeof(core_rt));
    memset(core_is_active, 0, sizeof(core_is_active));
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    // MPAM 5: throttling→non-throttling (end)
    mpam_rt[5].status_flags.active = true;
    mpam_rt[5].status_flags.throttling = true;
    mpam_rt[5].residency_timestamp_uS = 20000; // Set initial throttling start time
    mpam_rt[5].nominal_pstate = 8;             // Set nominal pstate for metrics

    // MPAM 6: non-throttling→throttling (start)
    mpam_rt[6].status_flags.active = true;
    mpam_rt[6].status_flags.throttling = false;

    // MPAM 7: active→inactive (end)
    mpam_rt[7].status_flags.active = true;
    mpam_rt[7].status_flags.throttling = false; // Set up cores for mixed transitions
    // Core 0: MPAM 5, not throttling (was throttling)
    core_rt[0].latest_mpam_id = 5;
    core_rt[0].latest_power_mW = 900;
    core_rt[0].latest_pstate = 9;
    core_rt[0].nominal_pstate = 8; // Set core's nominal_pstate to match MPAM's
    core_rt[0].status_flags.throttle_is_active = false;
    core_is_active[0] = true;

    // Core 1: MPAM 6, throttling (was not throttling)
    core_rt[1].latest_mpam_id = 6;
    core_rt[1].latest_power_mW = 1100;
    core_rt[1].latest_pstate = 11;
    core_rt[1].status_flags.throttle_is_active = true;
    core_is_active[1] = true;

    // No core for MPAM 7 (becomes inactive)

    // Mock timestamp for exec_tlm_cmpnt_get_timestamp_microseconds call
    // MPAM 5 residency = 55000 - 20000 = 35000 microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 55000);

    data_smpl_calculate_mpam_data_from_cores();

    // Verify mixed transitions were handled correctly
    // MPAM 5: throttling end detected
    assert_true(mpam_staging[5].active);
    assert_false(mpam_staging[5].throttling);
    assert_true(mpam_rt[5].status_flags.active);
    assert_false(mpam_rt[5].status_flags.throttling);

    // MPAM 6: throttling start detected
    assert_true(mpam_staging[6].active);
    assert_true(mpam_staging[6].throttling);
    assert_true(mpam_rt[6].status_flags.active);
    assert_true(mpam_rt[6].status_flags.throttling);

    // MPAM 7: active end detected
    assert_false(mpam_staging[7].active);
    assert_false(mpam_staging[7].throttling);
    assert_false(mpam_rt[7].status_flags.active);
    assert_false(mpam_rt[7].status_flags.throttling);

    // Verify comp_metrics_for_mpam_throttling was called for MPAM 5 throttling end
    // Expected: residency_uS = 55000 - 20000 = 35000, nominal_pstate = 8
    assert_int_equal(computed_metrics_2_mins.mpam[5].residency_uS, 35000);
    assert_int_equal(computed_metrics_2_mins.mpam[5].nominal_pstate, 8);
}

TEST_FUNCTION(test_data_smpl_die_mesh_tlm_init_success, test_setup, test_teardown)
{
    // Expected mesh_clock_telemetry call with enable=true and PER_DIE_MESH_PWR_TLM_INTERVAL
    expect_value(__wrap_mesh_clock_telemetry, enable, true);
    expect_value(__wrap_mesh_clock_telemetry, interval_count, PER_DIE_MESH_PWR_TLM_INTERVAL);
    expect_function_call(__wrap_mesh_clock_telemetry);

    // Call the function under test
    data_smpl_die_mesh_tlm_init();
}

TEST_FUNCTION(test_data_smpl_update_metrics_for_per_die_mesh_counters_full_flow, test_setup, test_teardown)
{
    // Ensure publishing is active
    in_band_publishing_active = true;

    // Clear computed metrics
    memset(&computed_metrics_2_mins, 0, sizeof(computed_metrics_2_mins));

    // Set up mock return values for mesh hardware APIs (inlined from data_smpl_die_mesh_pwr_tlm_get_data)
    will_return(__wrap_mesh_get_m1_entry_count, TEST_M1_ENTRY_COUNT);
    will_return(__wrap_mesh_get_m2_entry_count, TEST_M2_ENTRY_COUNT);
    will_return(__wrap_mesh_get_m0_residency, (uint32_t)TEST_M0_RESIDENCY);
    will_return(__wrap_mesh_get_m1_residency, (uint32_t)TEST_M1_RESIDENCY);
    will_return(__wrap_mesh_get_m2_residency, (uint32_t)TEST_M2_RESIDENCY);
    will_return(__wrap_mesh_get_telemetry_delivered_perf_count, (uint32_t)TEST_DELIVERED_PERF);

    // Set up expected call for data_smpl_die_mesh_tlm_init (re-initialization)
    expect_value(__wrap_mesh_clock_telemetry, enable, true);
    expect_value(__wrap_mesh_clock_telemetry, interval_count, PER_DIE_MESH_PWR_TLM_INTERVAL);
    expect_function_call(__wrap_mesh_clock_telemetry);

    // Call the function under test
    data_smpl_update_metrics_for_per_die_mesh_counters();

    // Verify computed metrics were updated
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.m1_entry_count,
                     TEST_M1_ENTRY_COUNT); // Should directly store TEST_M1_ENTRY_COUNT (5)
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.m2_entry_count,
                     TEST_M2_ENTRY_COUNT); // Should directly store TEST_M2_ENTRY_COUNT (3)

    // Verify residency values stored as raw counts
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.m0_residency_count, TEST_M0_RESIDENCY);
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.m1_residency_count, TEST_M1_RESIDENCY);
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.m2_residency_count, TEST_M2_RESIDENCY);
    assert_int_equal(computed_metrics_2_mins.mesh.die_mesh_pwr.delivered_perf_count, TEST_DELIVERED_PERF);
}

TEST_FUNCTION(test_data_smpl_die_mesh_tlm_reset_success, test_setup, test_teardown)
{
    // Test case: Verify reset function disables mesh telemetry with correct parameters

    // Expected mesh_clock_telemetry call with enable=false and PER_DIE_MESH_PWR_TLM_INTERVAL
    expect_value(__wrap_mesh_clock_telemetry, enable, false);
    expect_value(__wrap_mesh_clock_telemetry, interval_count, PER_DIE_MESH_PWR_TLM_INTERVAL);
    expect_function_call(__wrap_mesh_clock_telemetry);

    // Call the function under test
    data_smpl_die_mesh_tlm_reset();

    // Note: This function should disable mesh telemetry by calling mesh_clock_telemetry with enable=false
    // The interval_count parameter is passed but not used when disabling
}

TEST_FUNCTION(test_data_smpl_update_core_histogram, test_setup, test_teardown)
{
    // Clear core runtime info and computed metrics for clean test state
    memset(core_rt, 0, sizeof(core_rt));
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));

    // Enable both conditions for histogram updates
    in_band_publishing_active = true;
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_is_active[core_id] = true;
    }

    // Test case 1: Verify function processes all cores
    // Set unique voltage/temperature values for each core to verify individual processing
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_rt[core_id].latest_voltage_mV = 900 + (core_id * 10);  // 900, 910, 920, etc.
        core_rt[core_id].latest_max_value_dC = 700 + (core_id * 5); // 700, 705, 710, etc.
    }

    // Call the function under test
    data_smpl_update_core_histogram();

    // Verify that histogram was updated for each core (count total entries)
    uint32_t total_histogram_entries = 0;
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        for (uint8_t v = 0; v < NUMBER_OF_HS_VOLTAGE_SCALES; v++)
        {
            for (uint8_t t = 0; t < NUMBER_OF_HS_TEMP_SCALES; t++)
            {
                total_histogram_entries += computed_metrics_24_hrs.cores[core_id].histogram.bin_count[v][t];
            }
        }
    }

    // Should have exactly NUMBER_OF_CORES_PER_DIE entries (one per core)
    assert_int_equal(total_histogram_entries, NUMBER_OF_CORES_PER_DIE);

    // Test case 2: Verify function respects guard conditions
    // Clear histogram and test with publishing disabled
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));
    in_band_publishing_active = false;

    data_smpl_update_core_histogram();

    // No histogram entries should be created when publishing is disabled
    total_histogram_entries = 0;
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        for (uint8_t v = 0; v < NUMBER_OF_HS_VOLTAGE_SCALES; v++)
        {
            for (uint8_t t = 0; t < NUMBER_OF_HS_TEMP_SCALES; t++)
            {
                total_histogram_entries += computed_metrics_24_hrs.cores[core_id].histogram.bin_count[v][t];
            }
        }
    }
    assert_int_equal(total_histogram_entries, 0);

    // Test case 3: Verify function works with mixed active/inactive cores
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));
    in_band_publishing_active = true; // Re-enable publishing

    // Set only half the cores active
    uint8_t active_cores = 0;
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core_is_active[core_id] = (core_id % 2 == 0); // Even cores active
        if (core_is_active[core_id])
            active_cores++;

        // Set voltage/temperature for all cores
        core_rt[core_id].latest_voltage_mV = 950;
        core_rt[core_id].latest_max_value_dC = 750;
    }

    data_smpl_update_core_histogram();

    // Should only have entries for active cores
    total_histogram_entries = 0;
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        for (uint8_t v = 0; v < NUMBER_OF_HS_VOLTAGE_SCALES; v++)
        {
            for (uint8_t t = 0; t < NUMBER_OF_HS_TEMP_SCALES; t++)
            {
                total_histogram_entries += computed_metrics_24_hrs.cores[core_id].histogram.bin_count[v][t];
            }
        }
    }
    assert_int_equal(total_histogram_entries, active_cores);
}

TEST_FUNCTION(test_data_smpl_d2dss_link_tlm_reset, test_setup, test_teardown)
{
    // Pre-populate d2dss_rt with some non-zero data to verify it gets cleared
    for (uint8_t i = 0; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0] = 0x1000 + i;
        d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0] = 0x2000 + i;
        d2dss_rt[i].latest_bw_tx_flit_counter[D2D_LINK_L0] = 0x3000 + i;
        d2dss_rt[i].latest_bw_rx_flit_counter[D2D_LINK_L0] = 0x4000 + i;
    }

    // Setup mock expectations for d2dss_pmu_init calls

    for (uint8_t interface_id = 0; interface_id < NUMBER_OF_D2D_INTERFACES; interface_id++)
    {
        for (uint8_t event_source = 0; event_source <= D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT; event_source++)
        {
            expect_value(__wrap_d2dss_pmu_init, d2dss_index, interface_id);
            expect_value(__wrap_d2dss_pmu_init, event_number, event_source);
            expect_value(__wrap_d2dss_pmu_init, event_count, 0); // 0 to disable
            expect_value(__wrap_d2dss_pmu_init, enable, false);  // false to disable
            will_return(__wrap_d2dss_pmu_init, 0);               // Return success
        }
    }

    // Call the function to reset D2D telemetry
    data_smpl_d2dss_link_tlm_reset();

    // Verify that d2dss_rt runtime data has been cleared (memset to 0)
    for (uint8_t i = 0; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);

        // Also verify other link states are cleared
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0S], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0S], 0);
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L1], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L1], 0);
    }
}

TEST_FUNCTION(test_data_smpl_d2dss_link_tlm_reset_with_failure, test_setup, test_teardown)
{
    // Pre-populate d2dss_rt with some non-zero data to verify it gets cleared even on failure
    for (uint8_t i = 0; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0] = 0x5000 + i;
        d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0] = 0x6000 + i;
        d2dss_rt[i].latest_bw_tx_flit_counter[D2D_LINK_L0] = 0x7000 + i;
        d2dss_rt[i].latest_bw_rx_flit_counter[D2D_LINK_L0] = 0x8000 + i;
    }

    // Setup mock expectations for d2dss_pmu_init calls - simulate some failures
    for (uint8_t interface_id = 0; interface_id < NUMBER_OF_D2D_INTERFACES; interface_id++)
    {
        for (uint8_t event_source = 0; event_source <= D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT; event_source++)
        {
            expect_value(__wrap_d2dss_pmu_init, d2dss_index, interface_id);
            expect_value(__wrap_d2dss_pmu_init, event_number, event_source);
            expect_value(__wrap_d2dss_pmu_init, event_count, 0); // 0 to disable
            expect_value(__wrap_d2dss_pmu_init, enable, false);  // false to disable

            // Simulate failure for some specific interface/event combinations
            if (interface_id == 2 && event_source == D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY)
            {
                will_return(__wrap_d2dss_pmu_init, 1); // Return failure (non-zero)
            }
            else if (interface_id == 5 && event_source == D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT)
            {
                will_return(__wrap_d2dss_pmu_init, -1); // Return different failure code
            }
            else
            {
                will_return(__wrap_d2dss_pmu_init, 0); // Return success for others
            }
        }
    }

    // Call the function to reset D2D telemetry (should handle failures gracefully)
    data_smpl_d2dss_link_tlm_reset();

    // Verify that d2dss_rt runtime data has been cleared despite PMU init failures
    // The function should still perform memset at the end even if some PMU inits fail
    for (uint8_t i = 0; i < NUMBER_OF_D2D_INTERFACES; i++)
    {
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
        assert_int_equal(d2dss_rt[i].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);

        // Also verify other link states are cleared
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L0S], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L0S], 0);
        assert_int_equal(d2dss_rt[i].latest_tx_res_counter[D2D_LINK_L1], 0);
        assert_int_equal(d2dss_rt[i].latest_rx_res_counter[D2D_LINK_L1], 0);
    }
}

TEST_FUNCTION(test_data_smpl_read_d2dss_pmu_counter_success, test_setup, test_teardown)
{
    uint64_t counter_value = 0;
    uint8_t test_interface_id = 0;
    uint8_t test_event_number = 1;

    // Expected 64-bit counter value: 0x123456789ABCDEF0
    uint32_t expected_counter_low = 0x9ABCDEF0;  // Lower 32 bits
    uint32_t expected_counter_high = 0x12345678; // Upper 32 bits
    uint64_t expected_full_counter = 0x123456789ABCDEF0ULL;

    // Setup mock expectations for d2dss_pmu_read
    // Use the correct parameter names from the mock function signature
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, test_event_number);
    will_return(__wrap_d2dss_pmu_read, expected_counter_low);  // Counter low value
    will_return(__wrap_d2dss_pmu_read, expected_counter_high); // Counter high value
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);        // Return success code

    // Call the function
    bool result = data_smpl_read_d2dss_pmu_counter(test_interface_id, test_event_number, &counter_value);

    // Verify the result - the function should succeed
    assert_true(result);
    // Verify that the counter value matches the expected 64-bit value
    assert_int_equal(counter_value, expected_full_counter);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_pmu_counter_failure, test_setup, test_teardown)
{
    uint64_t counter_value = 0x999; // Pre-set to non-zero to verify it gets cleared on failure
    uint8_t test_interface_id = 1;
    uint8_t test_event_number = 2;

    // Setup mock expectations for d2dss_pmu_read to return failure
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, test_event_number);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low value (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high value (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 1); // Return non-zero (failure code)

    // Call the function
    bool result = data_smpl_read_d2dss_pmu_counter(test_interface_id, test_event_number, &counter_value);

    // Verify the result - the function should fail
    assert_false(result);
    // Verify that counter_value was set to 0 on error (as per implementation)
    assert_int_equal(counter_value, 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_pmu_counter_invalid_event, test_setup, test_teardown)
{
    uint64_t counter_value = 0x999; // Pre-set to non-zero
    uint8_t test_interface_id = 0;
    uint8_t invalid_event_number = D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT + 1; // Invalid (too high)

    // No mock expectations needed since function should return early due to validation

    // Call the function with invalid event number
    bool result = data_smpl_read_d2dss_pmu_counter(test_interface_id, invalid_event_number, &counter_value);

    // Verify the result - the function should fail due to invalid event number
    assert_false(result);
    // counter_value should remain unchanged since function returns early
    assert_int_equal(counter_value, 0x999);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_success, test_setup, test_teardown)
{
    uint8_t test_interface_id = 0;
    uint64_t rx_l0_count = 1000;
    uint64_t tx_l0_count = 2000;
    uint64_t rx_flit_count = 1000;
    uint64_t tx_flit_count = 2000;

    // Set previous values for residency counters
    d2dss_rt[test_interface_id].latest_tx_res_counter[0] = tx_l0_count - 800; // Previous TX L0 value
    d2dss_rt[test_interface_id].latest_rx_res_counter[0] = rx_l0_count - 500; // Previous RX L0 value

    // Set previous values for bandwidth counters
    d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[0] = tx_flit_count - 800; // Previous TX flit value
    d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[0] = rx_flit_count - 500; // Previous RX flit value

    // Declare local variables to receive diff values from output parameters
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // Setup mock expectations for all 4 PMU counter reads required by L0 state function
    // PMU Counter 6 - Incoming RX PHY L0 residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_l0_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_l0_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 4 - Off-die TX PHY L0 residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)tx_l0_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(tx_l0_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 7 - Incoming RX PHY flit count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_flit_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_flit_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 5 - Off-die TX PHY flit count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)tx_flit_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(tx_flit_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function with output parameters
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);
    // Verify the function succeeded
    assert_true(result);

    // Verify the calculated differences match expected values
    assert_int_equal(tx_res_diff, 800); // tx_l0_count - previous_tx_value = 2000 - 1200 = 800
    assert_int_equal(rx_res_diff, 500); // rx_l0_count - previous_rx_value = 1000 - 500 = 500
    assert_int_equal(bw_tx_diff, 800);  // tx_flit_count - previous_tx_flit = 2000 - 1200 = 800
    assert_int_equal(bw_rx_diff, 500);  // rx_flit_count - previous_rx_flit = 1000 - 500 = 500
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_failure_rx_l0_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 1;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // PMU Counter 6 - Incoming RX PHY L0 residency count (first call that will fail)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 1); // Return failure (non-zero)

    // Call the function
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_failure_tx_l0_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 2;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // PMU Counter 6 - RX PHY L0 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 1000); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);    // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 4 - TX PHY L0 residency (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0);  // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0);  // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, -1); // Return failure

    // Call the function
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_failure_rx_flit_count, test_setup, test_teardown)
{
    uint8_t test_interface_id = 3;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // PMU Counter 6 - RX PHY L0 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 1500); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);    // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 4 - TX PHY L0 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 2000); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);    // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 7 - RX PHY flit count (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 2); // Return failure

    // Call the function
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that the residency counters were updated before failure occurred
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0], 2000);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0], 1500);
    // Bandwidth counters should remain at initial state since failure occurred before updating them
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_failure_tx_flit_count, test_setup, test_teardown)
{
    uint8_t test_interface_id = 4;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // PMU Counter 6 - RX PHY L0 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 2500); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);    // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 4 - TX PHY L0 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 3000); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);    // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 7 - RX PHY flit count (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, 500); // Counter low
    will_return(__wrap_d2dss_pmu_read, 0);   // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 5 - TX PHY flit count (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 5); // Return failure

    // Call the function
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that the residency counters were updated before failure occurred
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0], 3000);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0], 2500);
    // Bandwidth counters should remain at initial state since failure occurred before completing bandwidth updates
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0], 0);
}

// D2D L0s state read test
TEST_FUNCTION(test_data_smpl_read_d2dss_l0s_state_success, test_setup, test_teardown)
{
    uint8_t test_interface_id = 1;
    uint64_t rx_l0s_count = 1000;
    uint64_t tx_l0s_count = 2000;

    uint64_t rx_flit_count = 1000;
    uint64_t tx_flit_count = 2000;

    // Set previous values for residency counters
    d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0S] = tx_l0s_count - 800; // Previous TX L0 value
    d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0S] = rx_l0s_count - 500; // Previous RX L0 value

    // Set previous values for bandwidth counters
    d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0S] = tx_flit_count - 800; // Previous TX flit value
    d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0S] = rx_flit_count - 500; // Previous RX flit value

    // Declare local variables to receive diff values from output parameters
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // Setup mock expectations for L0s state detection (only 2 PMU counters)
    // PMU Counter 0 - Incoming RX PHY L0s residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_l0s_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_l0s_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 2 - Off-die TX PHY L0s residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)tx_l0s_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(tx_l0s_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function with output parameters
    bool result = data_smpl_read_d2dss_l0s_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function succeeded
    assert_true(result);

    assert_int_equal(tx_res_diff, 800);
    assert_int_equal(rx_res_diff, 500);
}

// D2D L1 state read test
TEST_FUNCTION(test_data_smpl_read_d2dss_l1_state_success, test_setup, test_teardown)
{
    uint8_t test_interface_id = 0;
    uint64_t rx_l1_count = 3000;
    uint64_t tx_l1_count = 4000;

    d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L1] = rx_l1_count - 800; // Previous value
    d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L1] = tx_l1_count - 500; // Previous value

    // Declare local variables to receive diff values from output parameters
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // Setup mock expectations for L1 state detection (only 2 PMU counters)
    // PMU Counter 1 - Incoming RX PHY L1 residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L1_RESIDENCY);

    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_l1_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_l1_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);
    // PMU Counter 3 - Off-die TX PHY L1 residency count increment
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)tx_l1_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(tx_l1_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function with output parameters
    bool result = data_smpl_read_d2dss_l1_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function succeeded
    assert_true(result);

    assert_int_equal(tx_res_diff, 500);
    assert_int_equal(rx_res_diff, 800);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0s_state_failure_rx_l0s_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 2;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // PMU Counter 0 - RX PHY L0s residency (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 1); // Return failure (non-zero)

    // Call the function
    bool result = data_smpl_read_d2dss_l0s_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0S], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0S], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0s_state_failure_tx_l0s_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 3;
    uint64_t rx_l0s_count = 1500;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // PMU Counter 0 - RX PHY L0s residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_l0s_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_l0s_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 2 - TX PHY L0s residency (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0);  // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0);  // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, -1); // Return failure

    // Call the function
    bool result = data_smpl_read_d2dss_l0s_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    // since failure occurred before updating any counters
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0S], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0S], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l1_state_failure_rx_l1_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 4;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // PMU Counter 1 - RX PHY L1 residency (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 2); // Return failure (non-zero)

    // Call the function
    bool result = data_smpl_read_d2dss_l1_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L1], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L1], 0);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l1_state_failure_tx_l1_residency, test_setup, test_teardown)
{
    uint8_t test_interface_id = 5;
    uint64_t rx_l1_count = 2500;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // PMU Counter 1 - RX PHY L1 residency (success)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rx_l1_count);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rx_l1_count >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 3 - TX PHY L1 residency (failure)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, 0); // Counter low (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 0); // Counter high (don't care on failure)
    will_return(__wrap_d2dss_pmu_read, 3); // Return failure

    // Call the function
    bool result = data_smpl_read_d2dss_l1_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function failed as expected
    assert_false(result);

    // Verify that runtime tracking variables remain at their initial state (0)
    // since failure occurred before updating any counters
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L1], 0);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L1], 0);
}

// Counter Rollover Tests - Test 64-bit counter wraparound scenarios

TEST_FUNCTION(test_data_smpl_read_d2dss_l0_state_counter_rollover, test_setup, test_teardown)
{
    uint8_t test_interface_id = 6;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;
    uint64_t bw_tx_diff = 0;
    uint64_t bw_rx_diff = 0;

    // Simulate counter near max value (just before rollover)
    uint64_t near_max_value = 0xFFFFFFFFFFFFFFF0ULL;    // Close to UINT64_MAX
    uint64_t rolled_over_value = 0x0000000000000010ULL; // After rollover

    // First call - Set baseline near maximum
    d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0] = near_max_value - 1000;
    d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0] = near_max_value - 800;
    d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0] = near_max_value - 600;
    d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0] = near_max_value - 400;

    // Setup mock expectations - counters have rolled over
    // PMU Counter 6 - RX PHY L0 residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_value);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_value >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 4 - TX PHY L0 residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_value);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_value >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 7 - RX PHY flit count (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_value);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_value >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 5 - TX PHY flit count (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_FLIT_COUNT);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_value);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_value >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function
    bool result = data_smpl_read_d2dss_l0_state(test_interface_id, &tx_res_diff, &rx_res_diff, &bw_tx_diff, &bw_rx_diff);

    // Verify the function succeeded
    assert_true(result);

    // Verify rollover calculation: (rolled_over_value - previous_value) wraps around correctly
    // Expected diff = rolled_over_value + (UINT64_MAX - previous_value + 1)
    // Due to unsigned arithmetic, this automatically handles rollover
    uint64_t expected_tx_res_diff = rolled_over_value - (near_max_value - 1000);
    uint64_t expected_rx_res_diff = rolled_over_value - (near_max_value - 800);
    uint64_t expected_bw_tx_diff = rolled_over_value - (near_max_value - 600);
    uint64_t expected_bw_rx_diff = rolled_over_value - (near_max_value - 400);

    assert_int_equal(tx_res_diff, expected_tx_res_diff);
    assert_int_equal(rx_res_diff, expected_rx_res_diff);
    assert_int_equal(bw_tx_diff, expected_bw_tx_diff);
    assert_int_equal(bw_rx_diff, expected_bw_rx_diff);

    // Verify that runtime tracking variables were updated with new values
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0], rolled_over_value);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0], rolled_over_value);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_tx_flit_counter[D2D_LINK_L0], rolled_over_value);
    assert_int_equal(d2dss_rt[test_interface_id].latest_bw_rx_flit_counter[D2D_LINK_L0], rolled_over_value);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l0s_state_counter_rollover, test_setup, test_teardown)
{
    uint8_t test_interface_id = 7;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // Simulate counter near max value (just before rollover)
    uint64_t near_max_tx = 0xFFFFFFFFFFFFFF00ULL;
    uint64_t near_max_rx = 0xFFFFFFFFFFFFFFF0ULL;
    uint64_t rolled_over_tx = 0x0000000000000100ULL; // After rollover
    uint64_t rolled_over_rx = 0x0000000000000050ULL;

    // Set baseline near maximum
    d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0S] = near_max_tx;
    d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0S] = near_max_rx;

    // Setup mock expectations - counters have rolled over
    // PMU Counter 0 - RX PHY L0s residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_rx);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_rx >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 2 - TX PHY L0s residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L0S_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)rolled_over_tx);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(rolled_over_tx >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function
    bool result = data_smpl_read_d2dss_l0s_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function succeeded
    assert_true(result);

    // Verify rollover calculation handles wraparound correctly
    uint64_t expected_tx_diff = rolled_over_tx - near_max_tx;
    uint64_t expected_rx_diff = rolled_over_rx - near_max_rx;

    assert_int_equal(tx_res_diff, expected_tx_diff);
    assert_int_equal(rx_res_diff, expected_rx_diff);

    // Verify that runtime tracking variables were updated
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L0S], rolled_over_tx);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L0S], rolled_over_rx);
}

TEST_FUNCTION(test_data_smpl_read_d2dss_l1_state_counter_rollover, test_setup, test_teardown)
{
    uint8_t test_interface_id = 6;
    uint64_t tx_res_diff = 0;
    uint64_t rx_res_diff = 0;

    // Simulate exact rollover scenario - counter at max, then wraps to 0
    uint64_t max_value = 0xFFFFFFFFFFFFFFFFULL;      // UINT64_MAX
    uint64_t after_rollover = 0x0000000000000001ULL; // First value after max

    // Set baseline at maximum
    d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L1] = max_value;
    d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L1] = max_value;

    // Setup mock expectations - counters have rolled over from max to 1
    // PMU Counter 1 - RX PHY L1 residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_RX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)after_rollover);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(after_rollover >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // PMU Counter 3 - TX PHY L1 residency (rolled over)
    expect_value(__wrap_d2dss_pmu_read, d2dss_index, test_interface_id);
    expect_value(__wrap_d2dss_pmu_read, event_number, D2DSS_SOURCE_CNTR_TX_PHY_L1_RESIDENCY);
    will_return(__wrap_d2dss_pmu_read, (uint32_t)after_rollover);         // Counter low
    will_return(__wrap_d2dss_pmu_read, (uint32_t)(after_rollover >> 32)); // Counter high
    will_return(__wrap_d2dss_pmu_read, SILIBS_SUCCESS);

    // Call the function
    bool result = data_smpl_read_d2dss_l1_state(test_interface_id, &tx_res_diff, &rx_res_diff);

    // Verify the function succeeded
    assert_true(result);

    // Verify rollover: after_rollover - max_value = 1 - UINT64_MAX = 2 (in unsigned arithmetic)
    uint64_t expected_diff = after_rollover - max_value;

    printf("\n L1 counter rollover test : %llu  and %llu \n", tx_res_diff, expected_diff);

    assert_int_equal(tx_res_diff, expected_diff); // Should be 2
    assert_int_equal(rx_res_diff, expected_diff); // Should be 2

    // Verify that runtime tracking variables were updated
    assert_int_equal(d2dss_rt[test_interface_id].latest_tx_res_counter[D2D_LINK_L1], after_rollover);
    assert_int_equal(d2dss_rt[test_interface_id].latest_rx_res_counter[D2D_LINK_L1], after_rollover);
}

// D2D PMU initialization test - success case
TEST_FUNCTION(test_data_smpl_init_d2dss_pmu_counters_success, test_setup, test_teardown)
{
    // Setup mock expectations for d2dss_pmu_init
    for (uint8_t interface_id = 0; interface_id < NUMBER_OF_D2D_INTERFACES; interface_id++)
    {

        for (uint8_t event_source = 0; event_source <= D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT; event_source++)
        {
            expect_value(__wrap_d2dss_pmu_init, d2dss_index, interface_id);
            expect_value(__wrap_d2dss_pmu_init, event_number, event_source);
            if (event_source == D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY || event_source == D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY)
            {
                expect_value(__wrap_d2dss_pmu_init, event_count, 2); // L0 residency counters need config = 2
            }
            else
            {
                expect_value(__wrap_d2dss_pmu_init, event_count, 1);
            }
            expect_value(__wrap_d2dss_pmu_init, enable, true); // true to enable
            will_return(__wrap_d2dss_pmu_init, 0);             // Return success
        }
    }

    // Call the function
    data_smpl_init_d2dss_pmu_counters();

    // Function should initialize all PMU counters successfully
}

// D2D PMU initialization test - failure case
TEST_FUNCTION(test_data_smpl_init_d2dss_pmu_counters_failure, test_setup, test_teardown)
{
    // Setup mock expectations for d2dss_pmu_init - simulate failure on interface 1, event 2
    for (uint8_t interface_id = 0; interface_id < NUMBER_OF_D2D_INTERFACES; interface_id++)
    {
        for (uint8_t event_source = 0; event_source <= D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT; event_source++)
        {
            expect_value(__wrap_d2dss_pmu_init, d2dss_index, interface_id);
            expect_value(__wrap_d2dss_pmu_init, event_number, event_source);
            if (event_source == D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY || event_source == D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY)
            {
                expect_value(__wrap_d2dss_pmu_init, event_count, 2); // L0 residency counters need config = 2
            }
            else
            {
                expect_value(__wrap_d2dss_pmu_init, event_count, 1);
            }
            expect_value(__wrap_d2dss_pmu_init, enable, true); // true to enable

            // Simulate failure on interface 1, event 2 (arbitrary choice for test)
            if (interface_id == 1 && event_source == 2)
            {
                will_return(__wrap_d2dss_pmu_init, 1); // Return failure (non-zero)
            }
            else
            {
                will_return(__wrap_d2dss_pmu_init, 0); // Return success
            }
        }
    }

    // Call the function
    data_smpl_init_d2dss_pmu_counters();

    // Function should handle failures gracefully and continue with remaining initializations
}

// Test for data_smpl_process_package_cstates - all cores in C3
TEST_FUNCTION(test_data_smpl_process_package_cstates_all_cores_c3, test_setup, test_teardown)
{
    // Setup: Initialize die 1 (secondary die), set all cores to C3, enable power record
    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        core_rt[i].latest_cstate = CSTATE_C3;
        core_is_active[i] = true;
    }

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function
    data_smpl_process_package_cstates();

    // Verify: latest_pkg_cstate should be ALL_CORES_IN_C3_state
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_IN_C3_state);
}

// Test for data_smpl_process_package_cstates - all cores in C4
TEST_FUNCTION(test_data_smpl_process_package_cstates_all_cores_c4, test_setup, test_teardown)
{
    // Setup: Initialize die 1 (secondary die), set all cores to C4, enable power record
    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        core_rt[i].latest_cstate = CSTATE_C4;
        core_is_active[i] = true;
    }

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function
    data_smpl_process_package_cstates();

    // Verify: latest_pkg_cstate should be ALL_CORES_IN_C4_state
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_IN_C4_state);
}

// Test for data_smpl_process_package_cstates - mixed states (C3 and C4)
TEST_FUNCTION(test_data_smpl_process_package_cstates_mixed_states, test_setup, test_teardown)
{
    // Setup: Initialize die 1 (secondary die), set cores to mixed C-states
    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        if (i < NUMBER_OF_CORES_PER_DIE / 2)
        {
            core_rt[i].latest_cstate = CSTATE_C3;
        }
        else
        {
            core_rt[i].latest_cstate = CSTATE_C4;
        }
        core_is_active[i] = true;
    }
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);
    // Call the function - with new implementation, returns early
    data_smpl_process_package_cstates();

    // Verify: latest_pkg_cstate should be ALL_CORES_MIXED_C3_C4_state
    // The function detects mixed state in first loop (not all C3) and returns immediately
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_MIXED_C3_C4_state);
}

// Test for data_smpl_process_package_cstates - mixed states (C0 and C3)
TEST_FUNCTION(test_data_smpl_process_package_cstates_mixed_c0_c3, test_setup, test_teardown)
{
    // Setup: Mix of C0 and C3 states
    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        if (i < NUMBER_OF_CORES_PER_DIE / 2)
        {
            core_rt[i].latest_cstate = CSTATE_C0;
        }
        else
        {
            core_rt[i].latest_cstate = CSTATE_C3;
        }
        core_is_active[i] = true;
    }
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, false);
    // Call the function
    data_smpl_process_package_cstates();

    // Verify: Should detect mixed state (not all C3, not all C4)
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_MIXED_C3_C4_state);
}

// Test for data_smpl_process_package_cstates - primary die doesn't write
TEST_FUNCTION(test_data_smpl_process_package_cstates_primary_die_no_write, test_setup, test_teardown)
{
    // Setup: Initialize die 0 (primary die), set all cores to C3
    die_2_die_exch_init(0);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        core_rt[i].latest_cstate = CSTATE_C3;
        core_is_active[i] = true;
    }
    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function - primary die should not write to exchange
    data_smpl_process_package_cstates();

    // Verify: latest_pkg_cstate should be set to ALL_CORES_IN_C3_state
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_IN_C3_state);
    // Primary die does not write to exchange (no assertion needed for write)
}

// Test for data_smpl_process_package_cstates - inactive cores ignored
TEST_FUNCTION(test_data_smpl_process_package_cstates_inactive_cores, test_setup, test_teardown)
{
    // Setup: Initialize die 1, set some cores to C3, mark some as inactive
    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; i++)
    {
        if (i < NUMBER_OF_CORES_PER_DIE / 2)
        {
            core_rt[i].latest_cstate = CSTATE_C3;
            core_is_active[i] = true;
        }
        else
        {
            core_rt[i].latest_cstate = CSTATE_C0; // Different state
            core_is_active[i] = false;            // Inactive - should be ignored
        }
    }

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function
    data_smpl_process_package_cstates();

    // Verify: Should detect all ACTIVE cores in C3
    assert_int_equal(soc_rt.latest_pkg_cstate, ALL_CORES_IN_C3_state);
}

// Test for data_smpl_update_soc_package_cstate - both dies in C3
TEST_FUNCTION(test_data_smpl_update_soc_package_cstate_both_c3, test_setup, test_teardown)
{
    // Setup: Initialize die 0 (primary), set local die to C3, write C3 from die 1
    die_2_die_exch_init(0);
    soc_rt.latest_pkg_cstate = ALL_CORES_IN_C3_state;
    soc_rt.die1_pkg_mon.pkg_cstate = ALL_CORES_IN_C3_state;

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function
    data_smpl_update_soc_package_cstate();

    // Verify: PC3 residency should be incremented by DATA_AGGR_PKG_PERIOD_MS (10ms)
    assert_int_equal(computed_metrics_24_hrs.soc.pc3_residency_mS, DATA_AGGR_PKG_PERIOD_MS);
    assert_int_equal(computed_metrics_24_hrs.soc.pc4_residency_mS, 0);
}

TEST_FUNCTION(test_data_smpl_update_soc_package_cstate_both_C4, test_setup, test_teardown)
{
    // Setup: Initialize die 0 (primary), set local die to C3, write C3 from die 1
    die_2_die_exch_init(0);
    soc_rt.latest_pkg_cstate = ALL_CORES_IN_C4_state;
    soc_rt.die1_pkg_mon.pkg_cstate = ALL_CORES_IN_C4_state;

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function
    data_smpl_update_soc_package_cstate();

    assert_int_equal(computed_metrics_24_hrs.soc.pc4_residency_mS, DATA_AGGR_PKG_PERIOD_MS);
}

// Test for data_smpl_update_soc_package_cstate - local die mixed state
TEST_FUNCTION(test_data_smpl_update_soc_package_cstate_local_mixed, test_setup, test_teardown)
{
    // Reset metrics
    comp_metrics_reset_24_hrs_metrics();
    // Setup: Local die in mixed state
    die_2_die_exch_init(0);
    soc_rt.latest_pkg_cstate = ALL_CORES_MIXED_C3_C4_state;
    soc_rt.die1_pkg_mon.pkg_cstate = ALL_CORES_MIXED_C3_C4_state;

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function - should return early
    data_smpl_update_soc_package_cstate();

    // Verify: No residency accumulated
    assert_int_equal(computed_metrics_24_hrs.soc.pc3_residency_mS, 0);
    assert_int_equal(computed_metrics_24_hrs.soc.pc4_residency_mS, 0);
}
// Test for data_smpl_update_soc_package_cstate - local die mixed state
TEST_FUNCTION(test_data_smpl_update_soc_package_cstate_remote_mixed, test_setup, test_teardown)
{
    // Reset metrics
    comp_metrics_reset_24_hrs_metrics();
    // Setup: Local die in mixed state
    die_2_die_exch_init(0);
    soc_rt.latest_pkg_cstate = ALL_CORES_IN_C3_state;
    soc_rt.die1_pkg_mon.pkg_cstate = ALL_CORES_MIXED_C3_C4_state;

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function - should return early
    data_smpl_update_soc_package_cstate();

    // Verify: No residency accumulated
    assert_int_equal(computed_metrics_24_hrs.soc.pc3_residency_mS, 0);
    assert_int_equal(computed_metrics_24_hrs.soc.pc4_residency_mS, 0);
}

TEST_FUNCTION(test_data_smpl_get_secondary_die_package_cstate, test_setup, test_teardown)
{
    // Reset metrics
    comp_metrics_reset_24_hrs_metrics();
    // Setup: Local die in mixed state
    die_2_die_exch_init(0);
    data_smpl_get_secondary_die_package_cstate();
    // Verify: No residency accumulated
    assert_int_equal(soc_rt.die1_pkg_mon.pkg_cstate, 0);
}

// Test for data_smpl_update_soc_package_cstate - secondary die not primary
TEST_FUNCTION(test_data_smpl_update_soc_package_cstate_secondary_die, test_setup, test_teardown)
{
    // Setup: Initialize as secondary die (die 1)
    die_2_die_exch_init(1);
    soc_rt.latest_pkg_cstate = ALL_CORES_IN_C3_state;

    // Reset metrics
    comp_metrics_reset_24_hrs_metrics();

    will_return(__wrap_in_band_tlm_cmpnt_is_power_record_enabled, true);

    // Call the function - secondary die should return early
    data_smpl_update_soc_package_cstate();

    // Verify: No accumulation on secondary die
    assert_int_equal(computed_metrics_24_hrs.soc.pc3_residency_mS, 0);
}

TEST_FUNCTION(test_data_smpl_update_mpam_mem_power_primary_die, test_setup, test_teardown)
{
    // Test 1: Primary die behavior - reads from secondary die and local memory power
    // Setup: Initialize as primary die (die 0)
    die_2_die_exch_init(0);

    // Reset computed_metrics_2_mins to ensure clean state
    comp_metrics_reset_local_2_min_metrics();

    // Set up secondary die (die 1) memory power
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(1500); // Secondary die memory power = 1500 mW

    // Switch back to primary die
    die_2_die_exch_init(0);

    // Set up local memory power by adding samples
    data_util_running_avg_u32_add_sample(&computed_metrics_2_mins.soc.memory_avg_pwr_mW, 800);
    data_util_running_avg_u32_add_sample(&computed_metrics_2_mins.soc.memory_avg_pwr_mW, 1200);
    // Local memory power average = (800 + 1200) / 2 = 1000 mW

    // Call the function under test
    // Note: Currently this is a dummy implementation that doesn't store the result
    // It calculates: total = 1500 (die1) + 1000 (local) + 0 (fixed) = 2500 mW
    data_smpl_update_mpam_mem_power();

    // Test 2: Verify function doesn't crash and completes successfully
    // Since the implementation is currently a TODO, we're mainly testing that:
    // 1. The function executes without error
    // 2. It correctly identifies primary die and executes the primary die path
    // 3. It reads from die-to-die exchange without issues
    // 4. It accesses computed metrics without issues

    // Once the TODO is implemented, this test should be expanded to verify
    // that the calculated total_memory_power_mW is stored in the appropriate location
}

TEST_FUNCTION(test_data_smpl_update_mpam_mem_power_secondary_die, test_setup, test_teardown)
{
    // Test: Secondary die behavior - should return early without calculations
    // Setup: Initialize as secondary die (die 1)
    die_2_die_exch_init(1);

    // Reset computed_metrics_2_mins
    comp_metrics_reset_local_2_min_metrics();

    // Set up some local memory power
    data_util_running_avg_u32_add_sample(&computed_metrics_2_mins.soc.memory_avg_pwr_mW, 500);

    // Call the function under test
    // Should return early since this is not the primary die
    data_smpl_update_mpam_mem_power();

    // Verify function completes without error
    // On secondary die, the function should exit early without performing calculations
    // This test verifies the die ID check works correctly
}

TEST_FUNCTION(test_data_smpl_read_and_populate_core_vmin, test_setup, test_teardown)
{
    // Test: Verify that data_smpl_read_and_populate_core_vmin correctly reads Vmin values
    // from core exchange and populates core_rt[] structure

    // Setup: Create test Vmin data (600-667 mV for cores 0-67)
    uint16_t test_vmin_data[NUMBER_OF_CORES_PER_DIE];
    for (unsigned int i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        test_vmin_data[i] = (uint16_t)(600 + i);
    }

    // Clear core_rt to ensure clean state
    memset(core_rt, 0, sizeof(core_rt));

    // Setup the mock to return the test data
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_vmin, test_vmin_data);
    will_return(__wrap_pwr_tlm_core_exch_mcp_read_vmin, 1); // sequence number

    // Call the function under test
    data_smpl_read_and_populate_core_vmin();

    // Verify: Check that all core Vmin values were populated correctly
    for (unsigned int core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; ++core_id)
    {
        assert_int_equal(core_rt[core_id].latest_vmin_mV, (600 + core_id));
    }
}
