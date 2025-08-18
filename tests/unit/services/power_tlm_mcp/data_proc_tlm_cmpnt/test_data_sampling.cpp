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
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <dvfs.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
#include <telemetry_package_defs.h>
#include <tlm_fuses.h>
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;
extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
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

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_all_empty, test_setup, test_teardown)
{
    for (uint32_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        test_snsr_fifo_is_empty[i] = true;
    }

    will_return(__wrap_sensor_fifo_svc_is_empty, test_snsr_fifo_is_empty);

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
    comp_metrics_init();

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

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify current, power, and MPAM ID were updated
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(120 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, (uint16_t)(75 * CORE_POWER_MW_PER_BIT));
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

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify current, power, and MPAM ID were updated
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(80 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, (uint16_t)(40 * CORE_POWER_MW_PER_BIT));
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

    valid_entry = data_smpl_parse_core_current_entry(&current_data, core_id);
    assert_true(valid_entry);

    // Verify it works with maximum valid core ID
    assert_int_equal(core_rt[core_id].latest_current_mA, (uint16_t)(200 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id].latest_power_mW, (uint16_t)(100 * CORE_POWER_MW_PER_BIT));

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

    // Call the function under test
    data_smpl_process_core_current_sensor_fifo();

    // Verify first entry was processed correctly (throttling active path)
    assert_int_equal(core_rt[core_id_1].latest_current_mA, (uint16_t)(100 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id_1].latest_power_mW, (uint16_t)(50 * CORE_POWER_MW_PER_BIT));
    assert_int_equal(core_rt[core_id_1].latest_mpam_id, 5);
    assert_int_equal(core_rt[core_id_1].latest_pstate, 12); // Should be updated from current packet

    // Verify second entry was processed correctly (no throttling path)
    assert_int_equal(core_rt[core_id_2].latest_current_mA, (uint16_t)(80 * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core_rt[core_id_2].latest_power_mW, (uint16_t)(40 * CORE_POWER_MW_PER_BIT));
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
        .vr_current_mA = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage_mV = {1000, 900, 800, 700, 600, 700, 800, 900},
    };

    data_smpl_parse_vr_current_entry(&vr_data);

    // Verify all VR current and voltage values were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], vr_data.vr_current_mA[index]);
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data.vr_voltage_mV[index]);
    }

    // Test case 2: Zero/minimum values
    vr_current_t vr_data_zeros = {
        .timestamp = 2000,
        .vr_current_mA = {0, 0, 0, 0, 0, 0, 0, 0},
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
        .vr_current_mA = {5000, 4500, 4000, 3500, 3000, 2500, 2000, 1500},
        .vr_voltage_mV = {1200, 1100, 1050, 1000, 950, 900, 850, 800},
    };

    data_smpl_parse_vr_current_entry(&vr_data_high);

    // Verify high values were updated correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], vr_data_high.vr_current_mA[index]);
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data_high.vr_voltage_mV[index]);
    }

    // Test case 4: Mixed patterns
    vr_current_t vr_data_mixed = {
        .timestamp = 4000,
        .vr_current_mA = {100, 200, 150, 250, 175, 225, 125, 275},
        .vr_voltage_mV = {1100, 950, 1050, 900, 1000, 850, 950, 800},
    };

    data_smpl_parse_vr_current_entry(&vr_data_mixed);

    // Verify mixed pattern values were updated correctly
    uint16_t expected_current[] = {100, 200, 150, 250, 175, 225, 125, 275};
    uint16_t expected_voltage[] = {1100, 950, 1050, 900, 1000, 850, 950, 800};

    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], expected_current[index]);
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], expected_voltage[index]);
    }

    // Test case 5: Verify overwrite behavior (new values replace old ones)
    vr_current_t vr_data_overwrite = {
        .timestamp = 5000,
        .vr_current_mA = {999, 888, 777, 666, 555, 444, 333, 222},
        .vr_voltage_mV = {1111, 1000, 889, 778, 667, 556, 445, 334},
    };

    data_smpl_parse_vr_current_entry(&vr_data_overwrite);

    // Verify overwrite occurred correctly
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_rt.latest_rail_current_mA[index], vr_data_overwrite.vr_current_mA[index]);
        assert_int_equal(soc_rt.latest_rail_voltage_mV[index], vr_data_overwrite.vr_voltage_mV[index]);
    }
}

TEST_FUNCTION(test_data_smpl_process_vr_current_sensor_fifo, test_setup, test_teardown)
{
    // Test case 1: Process multiple FIFO entries
    sensor_ram_poll_status_t more_entries = {.curr_data_is_valid = true, .more_entries = true};
    sensor_ram_poll_status_t last_entry = {.curr_data_is_valid = true, .more_entries = false};

    vr_current_t vr_current_entry_1 = {
        .timestamp = 1000,
        .vr_current_mA = {100, 0, 0, 0, 0, 0, 0, 0}, // Simple data for FIFO testing
        .vr_voltage_mV = {1000, 0, 0, 0, 0, 0, 0, 0},
    };

    vr_current_t vr_current_entry_2 = {
        .timestamp = 2000,
        .vr_current_mA = {200, 0, 0, 0, 0, 0, 0, 0}, // Simple data for FIFO testing
        .vr_voltage_mV = {1100, 0, 0, 0, 0, 0, 0, 0},
    };

    // Mock sequence for multiple entries
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_entries);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &vr_current_entry_1);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &last_entry);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &vr_current_entry_2);

    // Call the function under test
    data_smpl_process_vr_current_sensor_fifo();

    // Basic verification that processing occurred (last entry processed)
    assert_int_equal(soc_rt.latest_rail_current_mA[0], 200);
    assert_int_equal(soc_rt.latest_rail_voltage_mV[0], 1100);

    // Test case 2: No entries to process (curr_data_is_valid = false)
    sensor_ram_poll_status_t invalid_entry = {.curr_data_is_valid = false, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &invalid_entry);

    // Store previous value to verify no change
    uint16_t prev_current = soc_rt.latest_rail_current_mA[0];

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
        .dimm_mr4_throttle_count = 2,
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
    assert_int_equal(dimm_rt.latest_dimm[0].throttling_flags, 1);

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
        .dimm_mr4_throttle_count = 0,
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

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Verify valid C0 to C2 transition
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[0].latest_cstate, 2); // C2

    // Test case 6: Valid same cstate (C2 to C2) - should be allowed
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 10000;
    cstate_data.data.cstate = 2; // Same C2 state

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

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[1].latest_cstate, 0);

    // C0 to C1
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000;
    cstate_data.data.cstate = 1;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_rt[1].latest_cstate, 1);

    // Test case 10: Edge case - same timestamp
    memset(&core_entry_data, 0, sizeof(core_entry_data));
    core_entry_data.packet_timestamp_uS = 3000; // Same timestamp
    cstate_data.data.cstate = 0;

    data_smpl_parse_cstate(&cstate_data, &core_entry_data);

    // Should process normally (timestamp >= is allowed)
    assert_true(core_entry_data.valid_entry_cstate);
    assert_true(core_entry_data.cstate_change);
    assert_int_equal(core_entry_data.cstate_time_diff_uS, 0); // Same timestamp
    assert_int_equal(core_rt[1].latest_cstate, 0);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_24hr_pkg_completed, test_setup, test_teardown)
{
    // TODO - complete with the records will be collected with 24hr window.
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

    // Test Case 14: Default case in switch statement (should do nothing special)
    memset(&core_rt[core_id], 0, sizeof(core_runtime_info_t));
    memset(&entry_data, 0, sizeof(entry_data));

    pstate_entry.data.throttle_status = SYS_FRC_PMIN_THROTTLING_START; // Valid enum but not handled in switch
    core_rt[core_id].latest_cstate = CSTATE_C0;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // 1MHz for easy conversion

    data_smpl_parse_core_states_entry(&pstate_entry, &entry_data);

    // Should reach the end without error (default case does nothing)
    // Should have called data_smpl_parse_cstate
    assert_true(entry_data.valid_entry_cstate);
    // No other state changes for default case
    assert_false(entry_data.valid_entry_pstate);
    assert_false(entry_data.throttling_state_change);
    assert_false(entry_data.rack_throttling_state_change);
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

    data_smpl_process_pstate_sensor_fifo();

    // Should have processed both entries
    assert_int_equal(core_rt[core_id].latest_pstate, 3);
}
