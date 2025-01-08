//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_telemetry.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_proc_tlm_cmpnt.h>
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
#include <tlm_logger_i.h>
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

// Test for data_proc_tlm_cmpnt_aggregate_pwr_tlm_data
TEST_FUNCTION(test_data_proc_tlm_cmpnt_aggregate_pwr_tlm_data, test_setup, test_teardown)
{
    // runtime information manager test
    sensor_ram_poll_status_t status_expected = {.curr_data_is_valid = false, .more_entries = false};
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &status_expected);
    data_proc_tlm_cmpnt_aggregate_pwr_tlm_data();
}
// Test for tlm_logger to init dts coefficient structures
TEST_FUNCTION(test_tlm_logger_init_fuse_dts_coeff_data, test_setup, test_teardown)
{
    fpfw_status_t status;
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);
    status = tlm_logger_init_fuse_dts_coeff_data();
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_logger_init_fuse_dts_coeff_data_fail, test_setup, test_teardown)
{
    fpfw_status_t status;
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_UNEXPECTED);
    status = tlm_logger_init_fuse_dts_coeff_data();
    assert_int_equal(status, FPFW_STATUS_UNEXPECTED);
}

// Test for tlm_logger_log_tile_temperature
TEST_FUNCTION(test_tlm_logger_log_tile_temperature, test_setup, test_teardown)
{
    fpfw_status_t status;

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
    status = tlm_logger_log_tile_temperature(&temperature_data, index);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    index = 1;
    temperature_data.temp1.temp1 = 60;
    status = tlm_logger_log_tile_temperature(&temperature_data, index);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // test index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    status = tlm_logger_log_tile_temperature(&temperature_data, index);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_tile_voltage
TEST_FUNCTION(test_tlm_logger_log_tile_voltage, test_setup, test_teardown)
{
    fpfw_status_t status;

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
    status = tlm_logger_log_tile_voltage(&voltage_data, index);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Check core 0 and core 1 voltage
    assert_int_equal(core[index].voltage.latest_value_mV, (uint16_t)(voltage_data.data.vcore0 * 1000));
    assert_int_equal(core[index + 1].voltage.latest_value_mV, (uint16_t)(voltage_data.data.vcore1 * 1000));
    assert_int_equal(tile[index].vcpu.latest_value_mV, (uint16_t)(voltage_data.data.vcpu * 1000));
    assert_int_equal(tile[index].vsys.latest_value_mV, (uint16_t)(voltage_data.data.vsys * 1000));

    // test index out of range
    index = NUMBER_OF_TILES_PER_DIE;
    status = tlm_logger_log_tile_voltage(&voltage_data, index);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_core_current
TEST_FUNCTION(test_tlm_logger_log_core_current, test_setup, test_teardown)
{
    fpfw_status_t status;
    // runtime information manager test
    core_current_t current_data = {
        .timestamp = 0,
        .data =
            {
                .avg = 100,
                .min = 10,
                .max = 150,
                .volt = 100,
                .pwr = 50,
                .pstate = 12,
                .change = 1,
                .mpam_id_low = 0,
                .mpam_id_high = 0,
                .cstate = 0,
            },
    };

    uint8_t index = 0;
    status = tlm_logger_log_core_current(&current_data, index);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    current_data.timestamp = 100;
    current_data.data.pstate = 13;
    current_data.data.change = 0;
    status = tlm_logger_log_core_current(&current_data, index);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Check core 0 current
    assert_int_equal(core[index].current_pkt_timestamp, (current_data.timestamp));
    assert_int_equal(core[index].current.latest_value_mA,
                     (uint16_t)(current_data.data.avg * CORE_CURRENT_CONVERSION_FACTOR));
    assert_int_equal(core[index].ldo_voltage, (uint16_t)(current_data.data.volt));
    assert_int_equal(core[index].current_tel_pstate, (current_data.data.pstate));
    assert_int_equal(core[index].current_mpam_id, (current_data.data.mpam_id_low));

    // test index out of range
    index = NUMBER_OF_CORES_PER_DIE;
    status = tlm_logger_log_core_current(&current_data, index);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_core_pstate
TEST_FUNCTION(test_tlm_logger_log_core_pstate, test_setup, test_teardown)
{
    fpfw_status_t status;
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
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Do Pstate change
    pstate_data.timestamp = 100;
    pstate_data.data.pstate = 5;
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Do Throttling start
    pstate_data.timestamp = 120;
    pstate_data.data.throttle_status = RACK_THROTTLE_START;
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Do Throttling start
    pstate_data.timestamp = 500;
    pstate_data.data.throttle_status = RACK_THROTTLE_END;
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Do Cstate change
    pstate_data.timestamp = 110;
    pstate_data.data.pstate = 31;
    pstate_data.data.cstate = 3;
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // test index out of range
    pstate_data.data.core = NUMBER_OF_CORES_PER_DIE;
    status = tlm_logger_log_core_pstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_vr_temp
TEST_FUNCTION(test_tlm_logger_log_vr_temp, test_setup, test_teardown)
{
    // runtime information manager test
    vr_temp_t vr_temperature = {
        .timestamp = 0,
        .vr_temp = {15, 25, 35, 45, 55, 65, 75, 85},
    };

    // Baseline log
    tlm_logger_log_vr_temp(&vr_temperature);

    // Check VR Temp
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.rail[index].temperature.latest_value_dC, (vr_temperature.vr_temp[index]));
    }
}

// Test for tlm_logger_log_vr_current
TEST_FUNCTION(test_tlm_logger_log_vr_current, test_setup, test_teardown)
{
    // runtime information manager test
    vr_current_t data = {
        .timestamp = 0,
        .vr_current = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage = {1000, 900, 800, 700, 600, 700, 800, 900},
    };

    // Baseline log
    tlm_logger_log_vr_current(&data);
    // Check VR Current and voltage
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.rail[index].current.latest_value_mA, (data.vr_current[index]));
        assert_int_equal(soc_info.rail[index].voltage.latest_value_mV, (data.vr_voltage[index]));
    }
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_aggregate_inst_tlm_data, test_setup, test_teardown)
{
    // expand unit test once implementation is available
    data_proc_tlm_cmpnt_aggregate_inst_tlm_data();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_aggregate_24hr_tlm_data, test_setup, test_teardown)
{
    // expand unit test once implementation is available
    data_proc_tlm_cmpnt_aggregate_24hr_tlm_data();
}
