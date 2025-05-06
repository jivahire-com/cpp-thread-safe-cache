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
#include <dvfs.h>
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
extern uint16_t core_pwr_samples_accumulation_mW[NUMBER_OF_CORES_PER_DIE];
extern uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES]; // /* used only for MPAM*/
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
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &status_expected);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &status_expected);

    data_proc_tlm_cmpnt_aggregate_pwr_tlm_data();
}
// Test for tlm_logger to init dts coefficient structures
TEST_FUNCTION(test_tlm_logger_init_fuse_dts_coeff_data, test_setup, test_teardown)
{
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_SUCCESS);
    tlm_logger_init_fuse_dts_coeff_data();
}

TEST_FUNCTION(test_tlm_logger_init_fuse_dts_coeff_data_fail, test_setup, test_teardown)
{
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile, dts_coeff, tileDtsCoefficients);
    expect_value(__wrap_platform_power_fuses_get_dts_coeff_tile,
                 count,
                 sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));

    will_return(__wrap_platform_power_fuses_get_dts_coeff_tile, FPFW_STATUS_UNEXPECTED);
    tlm_logger_init_fuse_dts_coeff_data();
}

// Test that the constant data is initialized correctly
TEST_FUNCTION(test_tlm_logger_init_constant_data, test_setup, test_teardown)
{
    tlm_logger_init_constant_data();

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        for (uint8_t cstate_id = 0; cstate_id < NUMBER_OF_CSTATES; cstate_id++)
        {
            assert_int_equal(core[core_id].cstate[cstate_id].cstate_id, cstate_id);
        }

        for (uint8_t pstate_id = 0; pstate_id < NUMBER_OF_PSTATES; pstate_id++)
        {
            assert_int_equal(core[core_id].pstate[pstate_id].pstate_id, pstate_id);
            assert_int_equal(core[core_id].pstate[pstate_id].frequency_Mhz, dvfs_get_freq_from_plimit(pstate_id));
        }
    }
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
    assert_int_equal(core[index].pstate_from_current_pkt, (current_data.data.pstate));
    assert_int_equal(core[index].active_sample_mpam_id, (current_data.data.mpam_id_low));

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

// Test for tlm_logger_log_core_pstate
TEST_FUNCTION(test_tlm_logger_log_core_cstate, test_setup, test_teardown)
{
    fpfw_status_t status;
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
    tlm_logger_log_core_cstate(&pstate_data);
    assert_int_equal(core[0].cstate[1].residency_uS, 0);
    assert_int_equal(core[0].cstate[1].entry_count, 1);

    pstate_data.timestamp = 20;

    pstate_data.data.cstate = 2;
    tlm_logger_log_core_cstate(&pstate_data);
    assert_int_equal(core[0].cstate[1].residency_uS, 10);
    assert_int_equal(core[0].cstate[2].entry_count, 1);

    pstate_data.timestamp = 30;
    tlm_logger_log_core_cstate(&pstate_data);

    pstate_data.timestamp = 40;
    tlm_logger_log_core_cstate(&pstate_data);

    pstate_data.timestamp = 50;
    tlm_logger_log_core_cstate(&pstate_data);

    pstate_data.data.cstate = 1;

    pstate_data.timestamp = 60;
    tlm_logger_log_core_cstate(&pstate_data);

    assert_int_equal(core[0].cstate[2].residency_uS, 40);
    assert_int_equal(core[0].cstate[1].entry_count, 2);

    // test :invalid cstate change.
    pstate_data.timestamp = 70;
    pstate_data.data.cstate = 4;
    status = tlm_logger_log_core_cstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_core_pstate
TEST_FUNCTION(test_tlm_logger_log_core_cstate_invalid_timestamp, test_setup, test_teardown)
{
    fpfw_status_t status;

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
                .core = 1,
                .mpam_low = 0,
                .mpam_high = 0,
                .boost_priority = 0,
            },
    };
    // test : first time core timestamp is 0 .
    status = tlm_logger_log_core_cstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(core[1].cstate_timestamp_uS, 10);

    assert_int_equal(core[1].cstate[1].residency_uS, 0);
    assert_int_equal(core[1].cstate[1].entry_count, 1);

    // test :timestamp < core[core_id].cstate_timestamp_uS

    pstate_data.timestamp = 5;
    pstate_data.data.cstate = 2;
    status = tlm_logger_log_core_cstate(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(core[1].cstate_timestamp_uS, 10);
}

// Test for tlm_logger_calculate_throttle_index_pass
TEST_FUNCTION(test_tlm_logger_calculate_throttle_index_pass, test_setup, test_teardown)
{

    pstate_throttle_status_t th_status = CURRENT_THROTTLING_START;
    int throttle_index = tlm_logger_calculate_throttle_index(th_status);
    assert_int_equal(throttle_index, 0);
}

// Test for tlm_logger_calculate_throttle_index_fail
TEST_FUNCTION(test_tlm_logger_calculate_throttle_index_fail, test_setup, test_teardown)
{
    pstate_throttle_status_t th_status = NO_THROTTLING;
    int throttle_index = tlm_logger_calculate_throttle_index(th_status);
    assert_int_equal(throttle_index, -1);
}
// Test for tlm_logger_log_core_states
TEST_FUNCTION(test_tlm_logger_log_core_states_no_throttling, test_setup, test_teardown)
{
    fpfw_status_t status;
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

    // Baseline log
    status = tlm_logger_log_core_states(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(core[0].throttle_info[1].entry_count, 0);
}

// Test for tlm_logger_log_core_throttling
TEST_FUNCTION(test_tlm_logger_log_core_states_in_throttle_start, test_setup, test_teardown)
{
    fpfw_status_t status;
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

    status = tlm_logger_log_core_states(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(core[0].throttle_info[0].entry_count, 1);
}

// Test for tlm_logger_log_core_throttling
TEST_FUNCTION(test_tlm_logger_log_core_states_in_throttle_end, test_setup, test_teardown)
{
    fpfw_status_t status;
    core[0].throttling_status = CURRENT_THROTTLING_START;

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

    status = tlm_logger_log_core_states(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

// Test for tlm_logger_log_core_throttling
TEST_FUNCTION(test_tlm_logger_log_core_throttling_start, test_setup, test_teardown)
{
    fpfw_status_t status;
    core[0].throttling_status = 0;
    core[0].throttle_previous_timestamp_uS[1] = 1000;

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

    status = tlm_logger_log_core_throttling(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(core[0].throttle_info[1].entry_count, 1);
    assert_int_equal(core[0].throttle_info[1].residency_mS, 0);
}

// Test for tlm_logger_log_core_throttling
TEST_FUNCTION(test_tlm_logger_log_core_throttling_end, test_setup, test_teardown)
{
    fpfw_status_t status;
    core[0].throttling_status = TEMP_THROTTLE_START;

    pstate_telem_t pstate_data = {
        .timestamp = 3000,
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

    status = tlm_logger_log_core_throttling(&pstate_data);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(core[0].throttle_info[1].residency_mS, 1);
}

// Test for tlm_logger_log_vr_temp
TEST_FUNCTION(test_tlm_logger_log_vr_temp, test_setup, test_teardown)
{
    // runtime information manager test
    vr_temp_t vr_temperature = {
        .timestamp = 0,
        .vr_temp_dC = {15, 25, 35, 45, 55, 65, 75, 85},
    };

    // Baseline log
    tlm_logger_log_vr_temp(&vr_temperature);

    // Check VR Temp
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.rail[index].temperature.latest_value_dC, (vr_temperature.vr_temp_dC[index]));
    }
}

// Test for tlm_logger_log_pvt_soc_temp
TEST_FUNCTION(test_tlm_logger_log_pvt_soc_temp, test_setup, test_teardown)
{
    soc_pvt_temp_t pvt_temperature = {
        .timestamp = 0,
        .sensor_temp_dC = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},

    };
    tlm_logger_log_soc_pvt_temp(&pvt_temperature);

    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        assert_int_equal(soc_info.sensor_temp[pvt_index].latest_value_dC, (pvt_temperature.sensor_temp_dC[pvt_index]));
    }
}

// Test for tlm_logger_log_dimm_infromation
TEST_FUNCTION(test_tlm_logger_log_dimm_information, test_setup, test_teardown)
{
    fpfw_status_t status;
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
    status = telmain_log_dimm_info(&dimm_info);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    // Check DIMM information
    assert_int_equal(soc_info.dimm[dimm_info.dimm_id].s0.latest_value_dC, (dimm_info.dimm_temp_s0_dC));
    assert_int_equal(soc_info.dimm[dimm_info.dimm_id].s1.latest_value_dC, (dimm_info.dimm_temp_s1_dC));

    // invalid DIMM id.
    dimm_info.dimm_id = 17;
    status = telmain_log_dimm_info(&dimm_info);
    assert_int_equal(status, FPFW_STATUS_INVALID_ARGS);
}

// Test for tlm_logger_log_vr_current
TEST_FUNCTION(test_tlm_logger_log_vr_current, test_setup, test_teardown)
{
    // runtime information manager test
    vr_current_t data = {
        .timestamp = 0,
        .vr_current_mA = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage_mV = {1000, 900, 800, 700, 600, 700, 800, 900},
    };

    // Baseline log
    tlm_logger_log_vr_current(&data);
    // Check VR Current and voltage
    for (uint8_t index = 0; index < MAX_NUM_OF_VR_RAILS; index++)
    {
        assert_int_equal(soc_info.rail[index].current.latest_value_mA, (data.vr_current_mA[index]));
        assert_int_equal(soc_info.rail[index].voltage.latest_value_mV, (data.vr_voltage_mV[index]));
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

// Unit test for tlm_calculate_mma_res
TEST_FUNCTION(test_tlm_calculate_mma_res, test_setup, test_teardown)
{

    uint16_t mma_min = 0;
    uint16_t mma_max = 0;
    uint16_t mma_average = 0;
    uint16_t mma_latest_value = 50;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Valid parameters
    tlm_calculate_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 50);
    assert_int_equal(mma_max, 50);
    assert_int_equal(mma_average, 50);

    // Test case: Update with new latest value
    mma_latest_value = 100;
    tlm_calculate_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 50);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 75); // Weighted average calculation

    // Test case: Update with lower latest value
    mma_latest_value = 30;
    tlm_calculate_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52); // Weighted average calculation

    // Test case: Zero latest value
    mma_latest_value = 0;
    tlm_calculate_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52); // No change in average

    // Test case: Invalid parameters (time_diff_uS = 0)
    time_diff_uS = 0;
    tlm_calculate_mma_res(&mma_min, &mma_max, &mma_average, &mma_latest_value, time_diff_uS, residency_uS);
    // No change in values as the function should handle invalid parameters gracefully
    assert_int_equal(mma_min, 30);
    assert_int_equal(mma_max, 100);
    assert_int_equal(mma_average, 52);
}

// Unit test for tlm_update_core_current
TEST_FUNCTION(test_tlm_update_core_current, test_setup, test_teardown)
{

    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].current.min_mA = 0;
    core[core_id].current.max_mA = 0;
    core[core_id].current.average_mA = 0;
    core[core_id].current.latest_value_mA = 50;

    tlm_update_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 50);
    assert_int_equal(core[core_id].current.max_mA, 50);
    assert_int_equal(core[core_id].current.average_mA, 50); // init avg is 0 so avg will be assigned to 50

    // Test case: Update with new latest value
    core[core_id].current.latest_value_mA = 100;
    tlm_update_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 50);
    assert_int_equal(core[core_id].current.max_mA, 100);
    assert_int_equal(core[core_id].current.average_mA, 75); // (50 + 100) / 2

    // Test case: Update with lower latest value
    core[core_id].current.latest_value_mA = 30;
    tlm_update_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 30);
    assert_int_equal(core[core_id].current.max_mA, 100);
    assert_int_equal(core[core_id].current.average_mA, 52); // (75 + 30) / 2
}

// Unit test for tlm_update_core_voltage
TEST_FUNCTION(test_tlm_update_core_voltage, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].voltage.min_mV = 0;
    core[core_id].voltage.max_mV = 0;
    core[core_id].voltage.average_mV = 0;
    core[core_id].voltage.latest_value_mV = 1200;
    tlm_update_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1200);
    assert_int_equal(core[core_id].voltage.max_mV, 1200);
    assert_int_equal(core[core_id].voltage.average_mV, 1200); // First time no average value to we assign 1200.

    // Test case: Update with new latest value
    core[core_id].voltage.latest_value_mV = 1300;
    tlm_update_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1200);
    assert_int_equal(core[core_id].voltage.max_mV, 1300);
    assert_int_equal(core[core_id].voltage.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    core[core_id].voltage.latest_value_mV = 1100;
    tlm_update_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1100);
    assert_int_equal(core[core_id].voltage.max_mV, 1300);
    assert_int_equal(core[core_id].voltage.average_mV, 1175); // (1250 + 1100) / 2
}

// Unit test for tlm_update_core_temperature
TEST_FUNCTION(test_tlm_update_core_temperature, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].temperature.min_dC = 0;
    core[core_id].temperature.max_dC = 0;
    core[core_id].temperature.average_dC = 0;
    core[core_id].temperature.latest_value_dC = 500;

    tlm_update_core_temperature(core_id, time_diff_uS, residency_uS);

    assert_int_equal(core[core_id].temperature.min_dC, 500);
    assert_int_equal(core[core_id].temperature.max_dC, 500);
    assert_int_equal(core[core_id].temperature.average_dC, 500); // first time we keep it 500

    // Test case: Update with new latest value
    core[core_id].temperature.latest_value_dC = 600;
    tlm_update_core_temperature(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].temperature.min_dC, 500);
    assert_int_equal(core[core_id].temperature.max_dC, 600);
    assert_int_equal(core[core_id].temperature.average_dC, 550); // (500 + 600) / 2

    // Test case: Update with lower latest value
    core[core_id].temperature.latest_value_dC = 400;
    tlm_update_core_temperature(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].temperature.min_dC, 400);
    assert_int_equal(core[core_id].temperature.max_dC, 600);
    assert_int_equal(core[core_id].temperature.average_dC, 475); // (550 + 400) / 2
}
// Unit test for tlm_update_tile_vcpu
TEST_FUNCTION(test_tlm_update_tile_vcpu, test_setup, test_teardown)
{
    uint8_t tile_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    tile[tile_id].vcpu.min_mV = 0;
    tile[tile_id].vcpu.max_mV = 0;
    tile[tile_id].vcpu.average_mV = 0;
    tile[tile_id].vcpu.latest_value_mV = 1200;
    tlm_update_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1200); // first value will be 1200

    // Test case: Update with new latest value
    tile[tile_id].vcpu.latest_value_mV = 1300;
    tlm_update_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1300);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    tile[tile_id].vcpu.latest_value_mV = 1100;
    tlm_update_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1100);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1300);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1175); // (1250 + 1100) / 2
}
// Unit test for tlm_update_tile_vsys
TEST_FUNCTION(test_tlm_update_tile_vsys, test_setup, test_teardown)
{
    uint8_t tile_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    tile[tile_id].vsys.min_mV = 0;
    tile[tile_id].vsys.max_mV = 0;
    tile[tile_id].vsys.average_mV = 0;
    tile[tile_id].vsys.latest_value_mV = 1200;
    tlm_update_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1200); // Init value to 1200

    // Test case: Update with new latest value
    tile[tile_id].vsys.latest_value_mV = 1300;
    tlm_update_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1300);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    tile[tile_id].vsys.latest_value_mV = 1100;
    tlm_update_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1100);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1300);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1175); // (1250 + 1100) / 2
}

TEST_FUNCTION(test_tlm_update_soc_rails_voltage, test_setup, test_teardown)
{

    uint8_t vr_index = 2;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some values
    soc_info.rail[vr_index].voltage.min_mV = 1000;
    soc_info.rail[vr_index].voltage.max_mV = 2000;
    soc_info.rail[vr_index].voltage.average_mV = 1500;
    soc_info.rail[vr_index].voltage.latest_value_mV = 1800;

    // Call the function to be tested
    tlm_update_soc_rails_voltage(vr_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.rail[vr_index].voltage.min_mV, 1000);
    assert_int_equal(soc_info.rail[vr_index].voltage.max_mV, 2000);
    assert_int_equal(soc_info.rail[vr_index].voltage.average_mV, 1650);
    assert_int_equal(soc_info.rail[vr_index].voltage.latest_value_mV, 1800);
}

TEST_FUNCTION(test_tlm_update_soc_rails_current, test_setup, test_teardown)
{
    uint8_t vr_index = 2;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some values
    soc_info.rail[vr_index].current.min_mA = 500;
    soc_info.rail[vr_index].current.max_mA = 1000;
    soc_info.rail[vr_index].current.average_mA = 750;
    soc_info.rail[vr_index].current.latest_value_mA = 900;

    // Call the function to be tested
    tlm_update_soc_rails_current(vr_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.rail[vr_index].current.min_mA, 500);
    assert_int_equal(soc_info.rail[vr_index].current.max_mA, 1000);
    assert_int_equal(soc_info.rail[vr_index].current.average_mA, 825);
    assert_int_equal(soc_info.rail[vr_index].current.latest_value_mA, 900);
}

TEST_FUNCTION(test_tlm_update_soc_hnf_temperature, test_setup, test_teardown)
{
    uint8_t hnf_index = 2;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some values
    soc_info.hnf[hnf_index].min_dC = 30;
    soc_info.hnf[hnf_index].max_dC = 70;
    soc_info.hnf[hnf_index].average_dC = 50;
    soc_info.hnf[hnf_index].latest_value_dC = 60;

    // Call the function to be tested
    tlm_update_soc_hnf_temperature(hnf_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.hnf[hnf_index].min_dC, 30);
    assert_int_equal(soc_info.hnf[hnf_index].max_dC, 70);
    assert_int_equal(soc_info.hnf[hnf_index].average_dC, 55);
    assert_int_equal(soc_info.hnf[hnf_index].latest_value_dC, 60);
}
TEST_FUNCTION(test_tlm_update_soc_pvt_temperature, test_setup, test_teardown)
{
    uint8_t pvt_index = 2;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some values
    soc_info.sensor_temp[pvt_index].min_dC = 25;
    soc_info.sensor_temp[pvt_index].max_dC = 75;
    soc_info.sensor_temp[pvt_index].average_dC = 50;
    soc_info.sensor_temp[pvt_index].latest_value_dC = 65;

    // Call the function to be tested
    tlm_update_soc_pvt_temperature(pvt_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.sensor_temp[pvt_index].min_dC, 25);
    assert_int_equal(soc_info.sensor_temp[pvt_index].max_dC, 75);
    assert_int_equal(soc_info.sensor_temp[pvt_index].average_dC, 57);
    assert_int_equal(soc_info.sensor_temp[pvt_index].latest_value_dC, 65);
}
TEST_FUNCTION(test_tlm_update_soc_dimm_info, test_setup, test_teardown)
{
    uint8_t dimm_module_index = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some test values
    soc_info.dimm[dimm_module_index].s0.min_dC = 0;
    soc_info.dimm[dimm_module_index].s0.max_dC = 0;
    soc_info.dimm[dimm_module_index].s0.average_dC = 0;
    soc_info.dimm[dimm_module_index].s0.latest_value_dC = 10;

    soc_info.dimm[dimm_module_index].s1.min_dC = 0;
    soc_info.dimm[dimm_module_index].s1.max_dC = 0;
    soc_info.dimm[dimm_module_index].s1.average_dC = 0;
    soc_info.dimm[dimm_module_index].s1.latest_value_dC = 20;

    // Call the function to be tested
    tlm_update_soc_dimm_info(dimm_module_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.dimm[dimm_module_index].s0.min_dC, 10);
    assert_int_equal(soc_info.dimm[dimm_module_index].s0.max_dC, 10);
    assert_int_equal(soc_info.dimm[dimm_module_index].s0.average_dC, 10);
    assert_int_equal(soc_info.dimm[dimm_module_index].s0.latest_value_dC, 10);
    assert_int_equal(soc_info.dimm[dimm_module_index].s1.min_dC, 20);
    assert_int_equal(soc_info.dimm[dimm_module_index].s1.max_dC, 20);
    assert_int_equal(soc_info.dimm[dimm_module_index].s1.average_dC, 20);
    assert_int_equal(soc_info.dimm[dimm_module_index].s1.latest_value_dC, 20);
}

TEST_FUNCTION(test_tlm_update_pstate, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t time_stamp_uS = 1000;

    // Test case: No throttling, no id_change_bit, no pstate_change
    core[core_id].throttling_status = NO_THROTTLE;
    core[core_id].pstate_timestamp_uS = 500;

    core[core_id].pstate_from_pstate_pkt = 1;
    int pstate_index = core[core_id].pstate_from_pstate_pkt;
    core[core_id].num_pwr_samples = 5;
    core[core_id].flags.id_change_bit = 0;
    core[core_id].flags.pstate_change = 0;
    core_pwr_samples_accumulation_mW[core_id] = 50;
    core[core_id].pstate[pstate_index].residency_uS = 200;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 10);
    // Test case: Throttling, no id_change_bit, no pstate_change
    core[core_id].throttling_status = TEMP_THROTTLE_START;
    core[core_id].pstate_from_current_pkt = 2;
    pstate_index = core[core_id].pstate_from_current_pkt;
    core[core_id].flags.id_change_bit = 0;
    core[core_id].flags.pstate_change = 0;
    core[core_id].num_pwr_samples = 12;
    core_pwr_samples_accumulation_mW[core_id] = 60;
    core[core_id].pstate[pstate_index].residency_uS = 300;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 5);

    // Test case: id_change_bit set
    core[core_id].flags.id_change_bit = 1;
    core[core_id].flags.pstate_change = 0;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].flags.id_change_bit, 0);
    assert_int_equal(core[core_id].flags.pstate_change, 0);
    assert_int_equal(pstate_accum_uS[core_id][pstate_index], 0);

    // Test case: pstate_change set
    core[core_id].flags.id_change_bit = 0;
    core[core_id].flags.pstate_change = 1;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].flags.id_change_bit, 0);
    assert_int_equal(core[core_id].flags.pstate_change, 0);
    assert_int_equal(pstate_accum_uS[core_id][pstate_index], 0);
}

TEST_FUNCTION(test_tlm_update_pstate_pwr, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t time_stamp_uS = 1000;
    // Test case: No throttling, no id_change_bit, no pstate_change
    core[core_id].throttling_status = NO_THROTTLE;
    core[core_id].pstate_timestamp_uS = 500;

    core[core_id].pstate_from_pstate_pkt = 1;
    int pstate_index = core[core_id].pstate_from_pstate_pkt;
    core[core_id].num_pwr_samples = 5;
    core[core_id].flags.id_change_bit = 0;
    core[core_id].flags.pstate_change = 0;
    core_pwr_samples_accumulation_mW[core_id] = 50;
    core[core_id].pstate[pstate_index].residency_uS = 200;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 10);

    // update pstate index and
    core[core_id].num_pwr_samples++;
    core_pwr_samples_accumulation_mW[core_id] += 20;
    core[core_id].pstate[pstate_index].residency_uS += 100;

    tlm_update_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 11); // 22 * 5
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 11);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 11);

    uint16_t test_latest_pwr_mW = 0;

    uint32_t test_weighted_previous_average = 0;
    uint32_t test_weighted_latest_value = 0;
    uint32_t test_new_avg_power_mW = 0;

    for (uint8_t i = 0; i < 10; i++)
    {
        core[core_id].num_pwr_samples++;
        core_pwr_samples_accumulation_mW[core_id] += 10;
        core[core_id].pstate[pstate_index].residency_uS += 100;
        test_latest_pwr_mW = (core_pwr_samples_accumulation_mW[core_id] / core[core_id].num_pwr_samples);
        time_stamp_uS += 10;

        test_weighted_previous_average =
            core[core_id].pstate[pstate_index].avg_power_mW * (core[core_id].num_pwr_samples - 1);
        test_weighted_latest_value = test_latest_pwr_mW;
        test_new_avg_power_mW = (test_weighted_previous_average + test_weighted_latest_value) / core[core_id].num_pwr_samples;

        tlm_update_pstate(core_id, time_stamp_uS);

        assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, test_latest_pwr_mW);

        assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, test_new_avg_power_mW);
    }
}

TEST_FUNCTION(test_tlm_update_rack_throttling, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = 4;

    pstate_telem_t pstate_data = {
        .timestamp = 10,
        .data =
            {
                .pstate = 12,
                .throttle_status = 0,
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
    // Call the function to be tested//TODO: update once implemented
    tlm_update_rack_throttling(&pstate_data, throttle_index, core_id);
}

// Unit test function
TEST_FUNCTION(test_tlm_update_throttling, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = 4;
    uint64_t time_stamp_uS = 5000;
    core[core_id].core_throttling_tracker[throttle_index] = 1;
    core[core_id].throttle_previous_timestamp_uS[throttle_index] = 1000;
    core[core_id].pstate_from_current_pkt = 20;

    core[core_id].throttle_info[throttle_index].residency_mS = 10;
    core[core_id].throttle_info[throttle_index].avg_pstate = 10;
    core[core_id].throttle_info[throttle_index].max_pstate = 5;

    tlm_update_throttling(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].throttle_info[throttle_index].residency_mS, 14);
    assert_int_equal(core[core_id].throttle_previous_timestamp_uS[throttle_index], 5000);
    assert_int_equal(core[core_id].throttle_info[throttle_index].max_pstate, 20);

    assert_int_equal(core[core_id].throttle_info[throttle_index].avg_pstate, 12);

    // inactive throttling
    core[core_id].core_throttling_tracker[throttle_index] = 0;
    time_stamp_uS = 10000;
    tlm_update_throttling(core_id, time_stamp_uS);

    assert_int_not_equal(core[core_id].throttle_info[throttle_index].residency_mS, 19);
    assert_int_equal(core[core_id].throttle_previous_timestamp_uS[throttle_index], 10000);
}
// tlm_update_throttling_pstate(core_id, i, time_diff_uS, core[core_id].throttle_info[i].residency_mS);

TEST_FUNCTION(test_tlm_update_throttling_pstate, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = 1;
    uint64_t time_stamp_uS = 5000;

    core[core_id].core_throttling_tracker[throttle_index] = 1;
    core[core_id].throttle_previous_timestamp_uS[throttle_index] = 1000;
    core[core_id].pstate_from_current_pkt = 20;

    core[core_id].throttle_info[throttle_index].residency_mS = 10;
    core[core_id].throttle_info[throttle_index].avg_pstate = 10;
    core[core_id].throttle_info[throttle_index].max_pstate = 5;
    uint32_t time_diff_uS = time_stamp_uS - core[core_id].throttle_previous_timestamp_uS[throttle_index];
    tlm_update_throttling_pstate(core_id,
                                 throttle_index,
                                 time_diff_uS,
                                 core[core_id].throttle_info[throttle_index].residency_mS);

    assert_int_equal(core[core_id].throttle_info[throttle_index].max_pstate, 20);
    assert_int_equal(core[core_id].throttle_info[throttle_index].avg_pstate, 14);
}

TEST_FUNCTION(test_tlm_update_core_histogram, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    // Call the function to be tested TODO:: update when implemented
    tlm_update_core_histogram(core_id);
    // Add assertions to verify the expected behavior
    // Since the function does nothing for now, there is nothing to assert
}

TEST_FUNCTION(test_tlm_soc_component_update, test_setup, test_teardown)
{
    // Initialize soc_info
    soc_info.time_counter_uS = 0;
    // Set up mock return values
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    // Call the function to be tested
    tlm_soc_component_update();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 0);

    // here soc_info.time_counter_uS will be 5 ;
    // Test :updated previous timestamp on next update.

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
    tlm_soc_component_update();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 5); // 10-5 =5

    // Test :update again.
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 15);
    tlm_soc_component_update();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 10); // 15-5 =5
}

TEST_FUNCTION(test_tlm_core_component_update, test_setup, test_teardown)
{
    // Initialize core data for testing
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        core[core_id].time_counter_uS = 0;
        core[core_id].current_pkt_timestamp = 1000; // Non-zero to trigger updates
    }

    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    // Call the function to be tested
    tlm_core_component_update();

    // Add assertions to verify the expected behavior
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core[core_id].time_counter_uS, 0);
    }

    // Test to updae : core[core_id].time_counter_uS in all core by time diff
    //  Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);

    // Call the function to be tested
    tlm_core_component_update();

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core[core_id].time_counter_uS, 5);
    }
}
// Unit test function
TEST_FUNCTION(test_tlm_tile_component_update, test_setup, test_teardown)
{
    // Initialize tile data for testing
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        tile[tile_id].time_counter_uS = 0;
        tile[tile_id].active_sample_max_temperature_dC = 50;
        tile[tile_id].max_tile_temperature_dC = 40;
        tile[tile_id].active_sample_max_id = 1;
        tile[tile_id].max_tile_id = 0;
    }
    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    tlm_tile_component_update();

    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        assert_int_equal(tile[tile_id].time_counter_uS, 0);
        assert_int_equal(tile[tile_id].max_tile_temperature_dC, 50);
        assert_int_equal(tile[tile_id].max_tile_id, 1);
    }

    uint8_t temp_active_sample_max_temperature_dC = 60;
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {

        tile[tile_id].active_sample_max_temperature_dC = temp_active_sample_max_temperature_dC;
        temp_active_sample_max_temperature_dC++;
        tile[tile_id].max_tile_temperature_dC = 40;
        tile[tile_id].active_sample_max_id = 1;
        tile[tile_id].max_tile_id = 0;
    }

    //  Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);

    tlm_tile_component_update();
    temp_active_sample_max_temperature_dC = 60;
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        assert_int_equal(tile[tile_id].time_counter_uS, 5);
        assert_int_equal(tile[tile_id].max_tile_temperature_dC, temp_active_sample_max_temperature_dC);
        temp_active_sample_max_temperature_dC++;
        assert_int_equal(tile[tile_id].max_tile_id, 1);
    }
}
