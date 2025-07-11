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
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_info;
extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
}

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
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

TEST_FUNCTION(test_data_proc_tlm_cmpnt_clear_pwr_tlm_data, test_setup, test_teardown)
{
    core_runtime_info_t zero_core[NUMBER_OF_CORES_PER_DIE] = {{{0}}};
    tile_runtime_info_t zero_tile[NUMBER_OF_TILES_PER_DIE] = {{0}};
    soc_runtime_info_t zero_soc_info = {0};

    memset(core, 0xFF, sizeof(core));
    memset(tile, 0xFF, sizeof(tile));
    memset(&soc_info, 0xFF, sizeof(soc_info));

    data_proc_tlm_cmpnt_clear_pwr_tlm_data();

    assert_memory_equal(&core, &zero_core, sizeof(zero_core));
    assert_memory_equal(&tile, &zero_tile, sizeof(zero_tile));
    assert_memory_equal(&soc_info, &zero_soc_info, sizeof(zero_soc_info));
}

// Test for data_proc_tlm_cmpnt_process_input_data
TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data, test_setup, test_teardown)
{
    //  runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected);

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_tile_voltage_entry_true, test_setup, test_teardown)
{
    //  runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_tile_voltage = {.curr_data_is_valid = true, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected_tile_voltage);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected);

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_current_entry_true, test_setup, test_teardown)
{
    //  runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_current = {.curr_data_is_valid = true, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected_current);

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_pstate_entry_test, test_setup, test_teardown)
{
    pstate_telem_t mock_pstate_data = {0};
    mock_pstate_data.timestamp = 1000;
    mock_pstate_data.data.pstate = 10;
    mock_pstate_data.data.plimit = 5;
    mock_pstate_data.data.core = 0;
    mock_pstate_data.data.throttle_status = 0;

    // runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_current = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t status_expected_dimm = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected_current);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected_dimm);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_pstate_entry_with_rack_throttling, test_setup, test_teardown)
{
    pstate_telem_t mock_pstate_data = {0};
    mock_pstate_data.timestamp = 1000;
    mock_pstate_data.data.pstate = 10;
    mock_pstate_data.data.plimit = 5;
    mock_pstate_data.data.core = 0;
    mock_pstate_data.data.throttle_status = RACK_THROTTLE_START;
    mock_pstate_data.data.vm_throttle_pri = 0;
    core[0].throttling_status = 1;
    // runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_current = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t status_expected_dimm = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected_current);
    // will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    //  will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &status_expected);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected_dimm);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
    assert_int_equal(core[0].throttling_status, RACK_THROTTLE_START);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_pstate_entry_with_overrun_counter, test_setup, test_teardown)
{
    pstate_telem_t mock_pstate_data = {0};
    mock_pstate_data.timestamp = 1000;
    mock_pstate_data.data.pstate = 10;
    mock_pstate_data.data.plimit = 5;
    mock_pstate_data.data.core = 0;
    mock_pstate_data.data.throttle_status = CURRENT_THROTTLE_OVERRUN;
    mock_pstate_data.data.vm_throttle_pri = 0;
    core[0].throttling_status = CURRENT_THROTTLE_START;
    // runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_current = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t status_expected_dimm = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected_current);
    // will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    //  will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &status_expected);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected_dimm);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
    assert_int_equal(core[0].latest_throttle_type, THROTTLE_SOURCE_CURRENT_OVERRUN);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_process_input_data_pstate_entry_with_throttle_state_change, test_setup, test_teardown)
{
    pstate_telem_t mock_pstate_data = {0};
    mock_pstate_data.timestamp = 1000;
    mock_pstate_data.data.pstate = 10;
    mock_pstate_data.data.plimit = 5;
    mock_pstate_data.data.core = 0;
    mock_pstate_data.data.throttle_status = CURRENT_THROTTLE_START;
    mock_pstate_data.data.vm_throttle_pri = 0;
    core[0].throttling_status = TEMP_THROTTLE_START;
    // runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_current = {.curr_data_is_valid = true, .more_entries = false};
    sensor_ram_poll_status_t status_expected_dimm = {.curr_data_is_valid = true, .more_entries = false};

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected_current);
    // will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    //  will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &status_expected);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected_dimm);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_process_input_data();
    assert_int_equal(core[0].throttling_status, CURRENT_THROTTLE_START);
}

TEST_FUNCTION(test_max_dimm_temp, test_setup, test_teardown)
{
    comp_metrics_init();
    //  runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    sensor_ram_poll_status_t status_expected_dimm_info = {.curr_data_is_valid = true, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, 0);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected);

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected_dimm_info);

    sensor_ram_dimm_info_t dimm_info = {0};
    dimm_info.dimm_temp_s0_dC = 500;
    dimm_info.dimm_temp_s1_dC = 600;

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &dimm_info);
    data_proc_tlm_cmpnt_process_input_data();

    uint16_t max_temp = data_util_mov_avg_u16_get(&computed_metrics_oob.max_dimm_temp_mov_avg_dC);
    assert_int_equal(max_temp, 600);
}

// Test for data sampling to init dts coefficient structures
TEST_FUNCTION(test_data_smpl_init_dts_coefficients, test_setup, test_teardown)
{
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);
    data_smpl_init_dts_coefficients();
}

TEST_FUNCTION(test_data_smpl_init_dts_coefficients_fail, test_setup, test_teardown)
{
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_UNEXPECTED);
    data_smpl_init_dts_coefficients();
}

// Test for data_smpl_parse_tile_temperature_entry
TEST_FUNCTION(test_data_smpl_parse_tile_temperature_entry, test_setup, test_teardown)
{
    // runtime information manager test
    tile_temp_t temperature_data = {
        .timestamp = 0,
        .temp0 =
            {
                .temp_valid = 1,
                .max_id = 7,
                .max_temp = 80,
                .core0 = 40,
                .core1 = 40,
            },
        .temp1 =
            {
                .temp0 = 20,
                .temp1 = 30,
                .temp2 = 40,
                .temp3 = 40,
            },
        .temp2 =
            {
                .temp4 = 30,
                .temp5 = 20,
                .temp6 = 40,
                .temp7 = 80,
            },
    };

    uint8_t index = 0;
    data_smpl_parse_tile_temperature_entry(&temperature_data, index);

    index = 1;
    temperature_data.temp1.temp1 = 60;
    temperature_data.temp0.max_temp = 16384;
    tileDtsCoefficients[index].k_val = 281;
    tileDtsCoefficients[index].y_val = 648;
    soc_info.latest_max_tile_temp_dC = 0;

    data_smpl_parse_tile_temperature_entry(&temperature_data, index);
    assert_int_equal(soc_info.latest_max_tile_temp_dC, 3670);

    // test index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    data_smpl_parse_tile_temperature_entry(&temperature_data, index);
}

// Test for data_smpl_parse_tile_voltage_entry
TEST_FUNCTION(test_data_smpl_parse_tile_voltage_entry, test_setup, test_teardown)
{

    // runtime information manager test
    tile_voltage_t voltage_data = {
        .timestamp = 0,
        .data =
            {
                .vcore0 = 5,
                .vcore1 = 6,
                .vcpu = 10,
                .vsys = 80,
            },
    };

    uint8_t index = 0;
    data_smpl_parse_tile_voltage_entry(&voltage_data, index);

    // Check core 0 and core 1 voltage
    assert_int_equal(core[index].latest_voltage_mV, (uint16_t)(voltage_data.data.vcore0 * 1000));
    assert_int_equal(core[index + 1].latest_voltage_mV, (uint16_t)(voltage_data.data.vcore1 * 1000));
    assert_int_equal(tile[index].vcpu.latest_value_mV, (uint16_t)(voltage_data.data.vcpu * 1000));
    assert_int_equal(tile[index].vsys.latest_value_mV, (uint16_t)(voltage_data.data.vsys * 1000));

    // test index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    data_smpl_parse_tile_voltage_entry(&voltage_data, index);
}

// Test for data_smpl_parse_core_current_entry
TEST_FUNCTION(test_data_smpl_parse_core_current_entry, test_setup, test_teardown)
{
    // runtime information manager test
    core_current_t current_data = {
        .timestamp = 0,
        .data =
            {
                .avg = 100,
                .min = 10,
                .max = 150,
                .volt = 1,
                .pwr = 50,
                .pstate = 12,
                .change = 1,
                .mpam_id_low = 0,
                .mpam_id_high = 0,
                .cstate = 0,
            },
    };
    core[0].throttling_status = TEMP_THROTTLING_START;
    uint8_t index = 0;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    data_smpl_parse_core_current_entry(&current_data, index);

    current_data.timestamp = 100;
    current_data.data.pstate = 13;
    current_data.data.change = 0;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    data_smpl_parse_core_current_entry(&current_data, index);

    // Check core 0 current
    assert_int_equal(core[index].current_pkt_timestamp_uS, (current_data.timestamp));
    assert_int_equal(core[index].latest_current_mA, (uint16_t)(current_data.data.avg * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core[index].pstate_from_current_pkt, (current_data.data.pstate));
    assert_int_equal(core[index].active_sample_mpam_id, (current_data.data.mpam_id_low));
    assert_int_equal(core[0].throttling_status, TEMP_THROTTLING_START);
    // Check core 0 power
    assert_int_equal(core[index].latest_power_mW, (uint16_t)(current_data.data.pwr * CORE_POWER_MW_PER_BIT));

    // test index out of range
    index = NUMBER_OF_CORES_PER_DIE;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    data_smpl_parse_core_current_entry(&current_data, index);
}

// Test for data_smpl_parse_pstate_no_throttling
TEST_FUNCTION(test_data_smpl_parse_pstate_no_throttling, test_setup, test_teardown)
{
    // runtime information manager test
    pstate_telem_t pstate_data = {
        .timestamp = 0,
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

    // Baseline log

    data_smpl_parse_pstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].active_sample_plimit, 5);
    assert_int_equal(core[0].latest_pstate, 12);
    // Do Pstate change
    pstate_data.timestamp = 100;
    pstate_data.data.pstate = 5;

    data_smpl_parse_pstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].latest_pstate, 5);
    // pstate not changed
    pstate_data.timestamp = 150;
    pstate_data.data.pstate = 6;

    data_smpl_parse_pstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].latest_pstate, 6);

    // Do Throttling start
    pstate_data.timestamp = 120;
    pstate_data.data.throttle_status = RACK_THROTTLE_START;

    data_smpl_parse_pstate_no_throttling(&pstate_data, pstate_data.timestamp);
}

// Test for data_smpl_parse_pstate_no_throttling
TEST_FUNCTION(test_data_smpl_parse_cstate_no_throttling, test_setup, test_teardown)
{
    core[0].cstate_timestamp_uS = 0;
    // pstate telemetry packet provide both p state/c state
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 1,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    data_smpl_parse_cstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].latest_cstate, 1);

    pstate_data.timestamp = 20;
    pstate_data.data.cstate = 2;

    data_smpl_parse_cstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].latest_cstate, 2);

    pstate_data.data.cstate = 1;
    pstate_data.timestamp = 60;

    data_smpl_parse_cstate_no_throttling(&pstate_data, pstate_data.timestamp);
    assert_int_equal(core[0].latest_cstate, 1);
    // test :invalid cstate change.
    pstate_data.timestamp = 70;
    pstate_data.data.cstate = 4;

    data_smpl_parse_cstate_no_throttling(&pstate_data, pstate_data.timestamp);
}

// Test for data_smpl_parse_pstate_no_throttling
TEST_FUNCTION(test_data_smpl_parse_cstate_no_throttling_invalid_timestamp, test_setup, test_teardown)
{
    core[1].cstate_timestamp_uS = 20;
    core[1].latest_cstate = 1;
    core[1].flags.cstate_change = 0;
    // pstate telemetry packet provide both p state/c state
    pstate_telem_t pstate_data = {
        .timestamp = 10, // This will a invald data set
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 1,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    // test : first time core timestamp is 0 .

    data_smpl_parse_cstate_no_throttling(&pstate_data, pstate_data.timestamp);
    // will not upfate the change bit
    assert_int_equal(core[1].flags.cstate_change, 0);
}

// Test for data_smpl_parse_throttling_state_change_get_index_from_status_pass
TEST_FUNCTION(test_data_smpl_parse_throttling_state_change_get_index_from_status_pass, test_setup, test_teardown)
{

    pstate_throttle_status_t th_status = CURRENT_THROTTLING_START;
    int throttle_index = data_smpl_parse_throttling_state_change_get_index_from_status(th_status);
    assert_int_equal(throttle_index, 0);
}

// Test for data_smpl_parse_throttling_state_change_get_index_from_status_fail
TEST_FUNCTION(test_data_smpl_parse_throttling_state_change_get_index_from_status_fail, test_setup, test_teardown)
{
    pstate_throttle_status_t th_status = NO_THROTTLING;
    int throttle_index = data_smpl_parse_throttling_state_change_get_index_from_status(th_status);
    assert_int_equal(throttle_index, -1);
}
// Invalid id's bound check for core id
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_no_throttling_invalid_core_id, test_setup, test_teardown)
{
    core_state_metrics_flags_t metrics = {0};
    core[0].pstate_timestamp_uS = 5;
    // Invalid id's bound check for core id ,
    // pstate and cstate will get compile time error if exceed limit
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = NO_THROTTLING,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 70, // Invalid value
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    // Baseline log
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);

    metrics = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_not_equal(metrics.pstate_time_diff_uS, 5); // because we return early on bound check
}

TEST_FUNCTION(test_data_smpl_parse_core_states_entry_no_throttling_cstate_update, test_setup, test_teardown)
{
    core_state_metrics_flags_t metrics = {0};
    core[0].pstate_timestamp_uS = 5;
    core[0].cstate_timestamp_uS = 5;
    // Invalid id's bound check for core id ,
    // pstate and cstate will get compile time error if exceed limit
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = NO_THROTTLING,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0, // Invalid value
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    // Baseline log
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);

    metrics = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(metrics.cstate_time_diff_uS, 5); // because we return early on bound check
    assert_int_equal(metrics.valid_entry_cstate, true);
    assert_int_equal(metrics.new_ctstate, true); // by default will be false to switch to true.
}

// Test for data_smpl_parse_core_states_entry
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_no_throttling, test_setup, test_teardown)
{
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = NO_THROTTLING,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    core[0].pstate_timestamp_uS = 5;
    core[0].throttling_status = CURRENT_THROTTLE_START;
    // Baseline log
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.valid_entry_pstate, true);
    assert_int_equal(flags.throttling_state_change, false);
}

// Test for data_smpl_parse_core_states_entry
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_in_throttle_start, test_setup, test_teardown)
{
    core[0].throttling_status = 0; // NO_THROTTLING;

    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = CURRENT_THROTTLING_START,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.valid_entry_pstate, false);
    assert_int_equal(flags.throttling_state_change, true);
}

// Test for data_smpl_parse_core_states_entry //overrun counter
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_for_overrun_count, test_setup, test_teardown)
{
    core[0].throttling_status = CURRENT_THROTTLING_START; // NO_THROTTLING;

    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = CURRENT_THROTTLING_OVERRUN,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.valid_entry_pstate, false);
    assert_int_equal(flags.throttling_state_change, false);
    assert_int_equal(flags.overrun_count_change, true);
    assert_int_equal(core[0].latest_throttle_type, THROTTLE_SOURCE_CURRENT_OVERRUN);

    // For adaptive clocking
    core[0].throttling_status = ADPT_CLK_THROTTLING_START;
    pstate_data.data.throttle_status = ADPT_CLK_THROTTLING_OVERRUN;

    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.overrun_count_change, true);
    assert_int_equal(core[0].latest_throttle_type, THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN);
}

// Test for data_smpl_parse_core_states_entry
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_in_throttle_end, test_setup, test_teardown)
{

    core[0].throttling_status = CURRENT_THROTTLING_START;
    core[0].pstate_timestamp_uS = 5;
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = CURRENT_THROTTLING_END,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);

    core[0].latest_throttle_type_previous_timestamp_uS[THROTTLE_SOURCE_CURRENT] = 5;
    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.valid_entry_pstate, false);
    assert_int_equal(flags.throttling_state_change, true);
    assert_int_equal(flags.throttle_start, false);
}

// Test for data_smpl_parse_core_states_entry_with_rack_throttling
TEST_FUNCTION(test_data_smpl_parse_core_states_entry_in_rack_throttle, test_setup, test_teardown)
{
    core[0].throttling_status = 0; // NO_THROTTLING;

    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = RACK_THROTTLE_START,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    core[0].latest_rack_priority_previous_timestamp_uS[0] = 5;
    // changing both rack priority to test rack start and new rack throttle
    core[0].latest_rack_throttling_priority_id = 0;
    pstate_data.data.vm_throttle_pri = 2;
    core[0].latest_rack_priority_previous_timestamp_uS[pstate_data.data.vm_throttle_pri] = 5;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000); // need only one wrap call in this case.
    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.rack_throttling_state_change, true);
    assert_int_equal(flags.rack_priority_start, true);
}

// Test for data_smpl_update_metrics_for_single_core_during_rack_throttling
TEST_FUNCTION(test_data_smpl_update_metrics_for_single_core_during_rack_throttling, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t time_stamp_uS = 3000;
    uint8_t rack_priority_id = 0;
    computed_metrics_2_mins.cores[core_id].rack_priorities[0].residency_mS = 0;
    core[0].throttling_status = RACK_THROTTLE_START;

    core[core_id].latest_rack_priority_previous_timestamp_uS[rack_priority_id] =
        core[0].latest_rack_priority_previous_timestamp_uS[0] = 5;
    // changing both rack priority to test rack start and new rack throttle
    core[0].latest_rack_throttling_priority_id = 0;
    core[0].latest_rack_priority_previous_timestamp_uS[0] = 1000;

    data_smpl_update_metrics_for_single_core_during_rack_throttling(core_id, time_stamp_uS);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[0].residency_mS, 2);
}

// Test for data_smpl_parse_throttling_state_change
TEST_FUNCTION(test_data_smpl_parse_throttling_state_change, test_setup, test_teardown)
{
    core[0].throttling_status = 0;
    core[0].latest_throttle_type_previous_timestamp_uS[1] = 1000;

    pstate_telem_t pstate_data = {
        .timestamp = 2000,
        .data =
            {
                .pstate = 12,
                .throttle_status = TEMP_THROTTLE_START,
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };

    pstate_data.data.throttle_status = 13; // Invalid value for test.
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    bool status = data_smpl_parse_throttling_state_change(&pstate_data);
    assert_int_equal(status, false);

    // same throttle status
    core[0].throttling_status = 10;
    pstate_data.data.throttle_status = 10;
    core[0].latest_throttle_type = 0;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    status = data_smpl_parse_throttling_state_change(&pstate_data);
    assert_int_equal(status, true);
    assert_int_equal(core[0].latest_throttle_type, THROTTLE_SOURCE_ADAPTIVE_CLK);
    assert_int_equal(core[0].throttling_status, ADPT_CLK_THROTTLE_END);

    // different thottle status
    core[0].latest_throttle_type_previous_timestamp_uS[THROTTLE_SOURCE_VR_HOT] = 5;
    pstate_data.data.throttle_status = SYS_FRC_PMIN_THROTTLE_START;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    status = data_smpl_parse_throttling_state_change(&pstate_data);
    assert_int_equal(status, true);
    assert_int_equal(core[0].latest_throttle_type, THROTTLE_SOURCE_VR_HOT);

    // Invalid timestamp
    core[0].latest_throttle_type_previous_timestamp_uS[THROTTLE_SOURCE_VR_HOT] = 100;
    pstate_data.timestamp = 50;
    pstate_data.data.throttle_status = SYS_FRC_PMIN_THROTTLE_END;
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    status = data_smpl_parse_throttling_state_change(&pstate_data);
    assert_int_equal(status, false);
}

// Test for data_smpl_parse_throttling_state_change
TEST_FUNCTION(test_data_smpl_parse_throttling_state_change_end, test_setup, test_teardown)
{
    core[0].throttling_status = TEMP_THROTTLE_START;
    core[0].latest_throttle_type_previous_timestamp_uS[1] = 1000;

    pstate_telem_t pstate_data = {
        .timestamp = 2000,
        .data =
            {
                .pstate = 12,
                .throttle_status = TEMP_THROTTLE_END, // TEMP_THROTTLE_END
                .vm_throttle_pri = 0,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);
    will_return(__wrap_gtimer_prodfw_get_frequency, 1000000);

    core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(&pstate_data);
    assert_int_equal(flags.valid_entry_pstate, false);
    assert_int_equal(flags.throttle_start, false);
}

TEST_FUNCTION(test_data_smpl_parse_throttling_state_change_exit, test_setup, test_teardown)
{
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        core[5].core_throttling_tracker[i] = 1;
    }
    // Call the function to be tested
    data_smpl_parse_throttling_state_change_exit_transition(5, 1000000);

    // Check that all throttling trackers are reset
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        if (i != THROTTLE_SOURCE_RACK_LIMIT)
        {
            assert_int_equal(core[5].core_throttling_tracker[i], 0);
        }
    }
    // Rack still not changed
    assert_int_equal(core[5].core_throttling_tracker[THROTTLE_SOURCE_RACK_LIMIT], 1);
    // additional test
    assert_int_equal(core[5].core_throttling_tracker[0], 0);
}

// Test for data_smpl_parse_vr_temperature_entry
TEST_FUNCTION(test_data_smpl_parse_vr_temperature_entry, test_setup, test_teardown)
{
    // runtime information manager test
    vr_temp_t vr_temperature = {
        .timestamp = 0,
        .vr_temp_dC = {15, 25, 35, 45, 55, 65, 75, 85},
    };

    // Baseline log
    data_smpl_parse_vr_temperature_entry(&vr_temperature);

    // Check VR Temp
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.latest_rail_temperature_dC[index], (vr_temperature.vr_temp_dC[index]));
    }
}

// Test for tlm_logger_log_pvt_soc_temp
TEST_FUNCTION(test_data_smpl_parse_pvt_temperature_entry, test_setup, test_teardown)
{
    soc_pvt_temp_t pvt_temperature = {
        .timestamp = 0,
        .sensor_temp_dC = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},

    };
    data_smpl_parse_pvt_temperature_entry(&pvt_temperature);

    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        assert_int_equal(soc_info.latest_soc_top_temp_dC[pvt_index], (pvt_temperature.sensor_temp_dC[pvt_index]));
    }
}

// Test for tlm_logger_log_dimm_infromation
TEST_FUNCTION(test_tlm_logger_log_dimm_information, test_setup, test_teardown)
{
    dimm_rt.latest_max_dimm_temp_dC = 42;
    sensor_ram_dimm_info_t dimm_info = {
        .timestamp = 0,
        .dimm_temp_s0_dC = 26,
        .dimm_temp_s1_dC = 28,
        .dimm_power_mW = 100,
        .dimm_id = 0,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 0,
    };
    // Baseline log
    data_smpl_parse_dimm_entry(&dimm_info);

    // Check DIMM information
    assert_int_equal(dimm_rt.latest_dimm[dimm_info.dimm_id].power_mW, (dimm_info.dimm_power_mW));
    assert_int_equal(dimm_rt.latest_dimm[dimm_info.dimm_id].temperature_dC, (dimm_info.dimm_temp_s1_dC));
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 42);

    dimm_rt.latest_max_dimm_temp_dC = 16;
    data_smpl_parse_dimm_entry(&dimm_info);
    assert_int_equal(dimm_rt.latest_max_dimm_temp_dC, 28);

    // invalid DIMM id.
    dimm_info.dimm_id = 17;
    data_smpl_parse_dimm_entry(&dimm_info);
}

// Test for data_smpl_parse_vr_current_entry
TEST_FUNCTION(test_data_smpl_parse_vr_current_entry, test_setup, test_teardown)
{
    // runtime information manager test
    vr_current_t data = {
        .timestamp = 0,
        .vr_current_mA = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage_mV = {1000, 900, 800, 700, 600, 700, 800, 900},
    };

    // Baseline log
    data_smpl_parse_vr_current_entry(&data);
    // Check VR Current and voltage
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.latest_rail_current_mA[index], (data.vr_current_mA[index]));
        assert_int_equal(soc_info.latest_rail_voltage_mV[index], (data.vr_voltage_mV[index]));
    }
}

TEST_FUNCTION(test_data_smpl_parse_rack_throttling, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = RACK_THROTTLING_END,
                .vm_throttle_pri = NUMBER_OF_RACK_PRIORITIES,
                .max_pstate = 0,
                .cstate = 2,
                .plimit = 5,
                .core = 0,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    // Call the function to be tested//TODO: update once implemented
    // invalid Priority id
    bool status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(status, false);

    // valid priority id
    pstate_data.data.vm_throttle_pri = 2;
    status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(status, true);
    // change to rack thorttle start from stop .
    pstate_data.data.throttle_status = RACK_THROTTLING_START;
    status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(status, true);
    // chnage rack priority
    core[core_id].latest_rack_throttling_priority_id = 3;
    status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(core[core_id].flags.rack_priority_change, 1);

    // rack priority same .
    core[core_id].latest_rack_throttling_priority_id = 3;
    pstate_data.data.vm_throttle_pri = 3;
    status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(core[core_id].flags.rack_priority_change, 0);
    // Invalid throttle status
    pstate_data.data.throttle_status = 13;
    status = data_smpl_parse_rack_throttling(&pstate_data);
    assert_int_equal(status, false);
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
    soc_info.latest_max_die_temp_dC = 0;
    soc_info.latest_max_tile_temp_dC = 400;
    soc_info.latest_max_soc_top_temp_dC = 300;
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, false);
    data_smpl_update_max_die_temp();

    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, true);
    data_smpl_update_max_die_temp();

    soc_info.latest_max_tile_temp_dC = 200;
    soc_info.latest_max_soc_top_temp_dC = 300;
    die_2_die_exch_init(1);
    will_return(__wrap_in_band_tlm_cmpnt_is_inst_record_enabled, true);
    data_smpl_update_max_die_temp();
}

TEST_FUNCTION(test_data_smpl_update_comp_metrics_cores_states_for_no_pstate_entry, test_setup, test_teardown)
{
    soc_info.latest_core_states_proc_timestamp_uS = 2;
    // Initialize core data for testing
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core[core_id].time_counter_uS = 0;
        core[core_id].latest_pstate = 2;
        core[core_id].current_pkt_timestamp_uS = 1000; // Non-zero to trigger updates
        core[core_id].throttling_status = NO_THROTTLING;
    }

    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    // Call the function to be tested
    data_smpl_update_comp_metrics_cores_states_for_no_pstate_entry();

    // Add assertions to verify the expected behavior
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core[core_id].time_counter_uS, 0);
    }

    assert_int_equal(soc_info.latest_core_states_proc_timestamp_uS, 5);

    // test throttling path.

    // Initialize core data for testing
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core[core_id].time_counter_uS = 0;
        core[core_id].latest_pstate = 3;
        core[core_id].current_pkt_timestamp_uS = 1000; // Non-zero to trigger updates
        core[core_id].throttling_status = TEMP_THROTTLING_START;
    }

    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
    data_smpl_update_comp_metrics_cores_states_for_no_pstate_entry();

    assert_int_equal(soc_info.latest_core_states_proc_timestamp_uS, 10);
}

// Unit test function
TEST_FUNCTION(test_data_smpl_update_metrics_for_single_core_during_throttling, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = 4;
    uint64_t time_stamp_uS = 5000;
    core[core_id].core_throttling_tracker[throttle_index] = 1;
    core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index] = 1000;
    core[core_id].latest_pstate = core[core_id].pstate_from_current_pkt = 20;

    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS = 10;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate = 10;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate = 5;

    data_smpl_update_metrics_for_single_core_during_throttling(core_id, time_stamp_uS);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS, 14);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate, 20);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate, 12);
}

// Unit test function
TEST_FUNCTION(test_data_smpl_get_active_throttling_for_single_core, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    { // make all active
        core[core_id].core_throttling_tracker[i] = 1;
    }
    // Call the function to be tested
    uint8_t active_throttlings = data_smpl_get_active_throttling_for_single_core(core_id);
    assert_int_equal(active_throttlings, 0x7F);

    // make all inactive
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        core[core_id].core_throttling_tracker[i] = 0;
    }
    active_throttlings = data_smpl_get_active_throttling_for_single_core(core_id);

    assert_int_equal(active_throttlings, 0);
}