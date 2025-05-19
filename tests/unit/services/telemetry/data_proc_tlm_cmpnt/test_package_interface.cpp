//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pack_get_apis.cpp
 * This tests the api's used to retrieve data for in band and out of band telemetry reporting.
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
#include <power_tlm_fuse.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
#include <string.h>              // for memcmp
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_info;
extern dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE];
}

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_SNSR_ID_0       (0)
#define TEST_HNF_CHANN_ID_1  (1)
#define TEST_RAIL_ID_2       (2)
#define TEST_DIMM_CHANN_ID_3 (3)
#define TEST_MPAM_ID_4       (4)
#define TEST_CORE_ID_5       (5)

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

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(test_get_pwr_core_pstate_data, test_setup, test_teardown)
{
    pwr_core_element_pstate_t pstate_array[NUMBER_OF_PSTATES] = {{0}};
    uint8_t avg_power = 30;
    pwr_pstate_t temp_pstate[NUMBER_OF_PSTATES] = {{0}};
    for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
    {
        core[TEST_CORE_ID_5].pstate[pstate_index].pstate_id = pstate_index;
        core[TEST_CORE_ID_5].pstate[pstate_index].avg_power_mW = avg_power;
        core[TEST_CORE_ID_5].pstate[pstate_index].max_power_mW = 40;
        core[TEST_CORE_ID_5].pstate[pstate_index].min_power_mW = 10;
        core[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz = 150;
        core[TEST_CORE_ID_5].pstate[pstate_index].residency_uS = 10000;
        core[TEST_CORE_ID_5].pstate[pstate_index].entry_count = 10;
        // increment avg power for every pstate by 1
        avg_power++;
        // cache data to correctness check after the record collection because some data
        // need to be reset for next collection window.
        memcpy(&temp_pstate[pstate_index],
               &core[TEST_CORE_ID_5].pstate[pstate_index],
               sizeof(core[TEST_CORE_ID_5].pstate[pstate_index]));
    }

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(TEST_CORE_ID_5, &pstate_array);
    // check for valid data into full pstate_array .
    // Every core will have NUMBER_OF_PSTATES, and each pstate elemenet have , it's own data
    for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
    {
        assert_int_equal(pstate_array[pstate_index].pstate_id, temp_pstate[pstate_index].pstate_id);
        assert_int_equal(pstate_array[pstate_index].avg_power_mW, temp_pstate[pstate_index].avg_power_mW);
        assert_int_equal(pstate_array[pstate_index].min_power_mW, temp_pstate[pstate_index].min_power_mW);
        assert_int_equal(pstate_array[pstate_index].max_power_mW, temp_pstate[pstate_index].max_power_mW);
        assert_int_equal(pstate_array[pstate_index].frequency_Mhz, temp_pstate[pstate_index].frequency_Mhz);
        assert_int_equal(pstate_array[pstate_index].residency_mS, temp_pstate[pstate_index].residency_uS / 1000);
        assert_int_equal(pstate_array[pstate_index].entry_count, temp_pstate[pstate_index].entry_count);
    }
    // setup for failure case, change pstate 0 element data.
    uint8_t index = 0;
    core[TEST_CORE_ID_5].pstate[index].pstate_id = 0;
    core[TEST_CORE_ID_5].pstate[index].avg_power_mW = 41;
    core[TEST_CORE_ID_5].pstate[index].max_power_mW = 50;
    core[TEST_CORE_ID_5].pstate[index].min_power_mW = 15;
    core[TEST_CORE_ID_5].pstate[index].frequency_Mhz = 155;
    core[TEST_CORE_ID_5].pstate[index].residency_uS = 15;
    core[TEST_CORE_ID_5].pstate[index].entry_count = 20;
    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(NUMBER_OF_CORES_PER_DIE, &pstate_array);

    // verify for pstate 0
    assert_int_equal(pstate_array[index].pstate_id, core[TEST_CORE_ID_5].pstate[index].pstate_id);
    assert_int_not_equal(pstate_array[index].avg_power_mW, core[TEST_CORE_ID_5].pstate[index].avg_power_mW);
    assert_int_not_equal(pstate_array[index].min_power_mW, core[TEST_CORE_ID_5].pstate[index].min_power_mW);
    assert_int_not_equal(pstate_array[index].max_power_mW, core[TEST_CORE_ID_5].pstate[index].max_power_mW);
    assert_int_not_equal(pstate_array[index].frequency_Mhz, core[TEST_CORE_ID_5].pstate[index].frequency_Mhz);
    assert_int_not_equal(pstate_array[index].residency_mS, core[TEST_CORE_ID_5].pstate[index].residency_uS);
    assert_int_not_equal(pstate_array[index].entry_count, core[TEST_CORE_ID_5].pstate[index].entry_count);
}

TEST_FUNCTION(test_get_pwr_core_cstate_data, test_setup, test_teardown)
{
    pwr_core_element_cstate_t cstate_array[NUMBER_OF_CSTATES] = {{0}};
    pwr_cstate_t temp_cstate[NUMBER_OF_CSTATES] = {{0}};
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        core[TEST_CORE_ID_5].cstate[cstate_index].cstate_id = cstate_index;
        core[TEST_CORE_ID_5].cstate[cstate_index].residency_uS = 10000;
        core[TEST_CORE_ID_5].cstate[cstate_index].entry_count = 10;

        memcpy(&temp_cstate[cstate_index],
               &core[TEST_CORE_ID_5].cstate[cstate_index],
               sizeof(core[TEST_CORE_ID_5].pstate[cstate_index]));
    }
    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(TEST_CORE_ID_5, &cstate_array);
    // check for valid data into full cstate_array .
    // Every core will have NUMBER_OF_CSTATES, and each cstate elemenet have , it's own data
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        assert_int_equal(cstate_array[cstate_index].cstate_id, temp_cstate[cstate_index].cstate_id);
        assert_int_equal(cstate_array[cstate_index].residency_mS, temp_cstate[cstate_index].residency_uS / 1000);
        assert_int_equal(cstate_array[cstate_index].entry_count, temp_cstate[cstate_index].entry_count);
    }
}

TEST_FUNCTION(test_get_pwr_core_throttle_data, test_setup, test_teardown)
{
    pwr_core_element_throttle_t throttle_array[NUMBER_OF_THROTTLE_TYPES] = {{0}};
    uint8_t throttle_index = 0;
    uint8_t avg_pstate = 5;
    uint32_t entry_count = 10;
    uint32_t residency_mS = 100;
    uint8_t max_pstate = 5;
    uint8_t type_id = THROTTLE_SOURCE_TEMPERATURE;

    for (throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
    {
        core[TEST_CORE_ID_5].throttle_info[throttle_index].avg_pstate = avg_pstate;
        core[TEST_CORE_ID_5].throttle_info[throttle_index].entry_count = entry_count;
        core[TEST_CORE_ID_5].throttle_info[throttle_index].max_pstate = max_pstate;
        core[TEST_CORE_ID_5].throttle_info[throttle_index].residency_mS = residency_mS;
        core[TEST_CORE_ID_5].throttle_info[throttle_index].type_id = type_id;
    }
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(TEST_CORE_ID_5, &throttle_array);

    for (throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
    {
        assert_int_equal(throttle_array[throttle_index].avg_pstate, avg_pstate);
        assert_int_equal(throttle_array[throttle_index].entry_count, entry_count);
        assert_int_equal(throttle_array[throttle_index].max_pstate, max_pstate);
        assert_int_equal(throttle_array[throttle_index].residency_mS, residency_mS);
        assert_int_equal(throttle_array[throttle_index].type_id, type_id);
    }

    // setup for failure case.
    uint8_t index = 0; // test for one of the throttle type.
    core[TEST_CORE_ID_5].throttle_info[index].avg_pstate = 6;
    core[TEST_CORE_ID_5].throttle_info[index].entry_count = 12;
    core[TEST_CORE_ID_5].throttle_info[index].max_pstate = 28;
    core[TEST_CORE_ID_5].throttle_info[index].residency_mS = 110;
    core[TEST_CORE_ID_5].throttle_info[index].type_id = THROTTLE_SOURCE_TEMPERATURE;
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(NUMBER_OF_CORES_PER_DIE, &throttle_array);

    throttle_index = 0;

    assert_int_not_equal(throttle_array[throttle_index].avg_pstate,
                         core[TEST_CORE_ID_5].throttle_info[throttle_index].avg_pstate);
    assert_int_not_equal(throttle_array[throttle_index].entry_count,
                         core[TEST_CORE_ID_5].throttle_info[throttle_index].entry_count);
    assert_int_not_equal(throttle_array[throttle_index].max_pstate,
                         core[TEST_CORE_ID_5].throttle_info[throttle_index].max_pstate);
    assert_int_equal(throttle_array[throttle_index].reserved,
                     core[TEST_CORE_ID_5].throttle_info[throttle_index].reserved);
    assert_int_not_equal(throttle_array[throttle_index].residency_mS,
                         core[TEST_CORE_ID_5].throttle_info[throttle_index].residency_mS);
    assert_int_equal(throttle_array[throttle_index].type_id,
                     core[TEST_CORE_ID_5].throttle_info[throttle_index].type_id);
}

TEST_FUNCTION(test_get_pwr_core_rack_priority_data, test_setup, test_teardown)
{
    pwr_core_element_rack_priorities_t rack_priority_array[NUMBER_OF_RACK_PRIORITIES] = {{0}};
    uint8_t rack_index = 0;
    // test setup
    // Throttling Priorities are only for the Rack Throttling type and Rack Priorities are
    // only populated when the throttling type is the Rack Throttling.
    for (rack_index = 0; rack_index < NUMBER_OF_RACK_PRIORITIES; rack_index++)
    {
        // ID representing the Priority, there are 8 Priority Levels (0 to 7)
        core[TEST_CORE_ID_5].priorities[rack_index].priority_id = rack_index;
        core[TEST_CORE_ID_5].priorities[rack_index].entry_count = 1;
        core[TEST_CORE_ID_5].priorities[rack_index].residency_mS = 100;
    }

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(TEST_CORE_ID_5, &rack_priority_array);

    for (rack_index = 0; rack_index < NUMBER_OF_RACK_PRIORITIES; rack_index++)
    {
        assert_int_equal(rack_priority_array[rack_index].priority_id,
                         core[TEST_CORE_ID_5].priorities[rack_index].priority_id);
        assert_int_equal(rack_priority_array[rack_index].entry_count,
                         core[TEST_CORE_ID_5].priorities[rack_index].entry_count);
        assert_int_equal(rack_priority_array[rack_index].residency_mS,
                         core[TEST_CORE_ID_5].priorities[rack_index].residency_mS);
    }
    rack_index = 0;
    // test for failure case .
    // change data in core data structure for rack priority 0 only.
    core[TEST_CORE_ID_5].priorities[rack_index].entry_count = 2;
    core[TEST_CORE_ID_5].priorities[rack_index].residency_mS = 110;

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(NUMBER_OF_CORES_PER_DIE, &rack_priority_array);

    assert_int_equal(rack_priority_array[rack_index].priority_id, core[TEST_CORE_ID_5].priorities[rack_index].priority_id);
    assert_int_not_equal(rack_priority_array[rack_index].entry_count,
                         core[TEST_CORE_ID_5].priorities[rack_index].entry_count);
    assert_int_not_equal(rack_priority_array[rack_index].residency_mS,
                         core[TEST_CORE_ID_5].priorities[rack_index].residency_mS);
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
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
    pwr_core_element_voltage_t voltage_get_data = {0};
    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(index, &voltage_get_data);
    assert_int_equal(voltage_get_data.latest_value_mV, voltage_data.data.vcore0 * 1000);

    // test index out of range

    voltage_data.data.vcore0 = 6; // update voltage
    data_smpl_parse_tile_voltage_entry(&voltage_data, index);

    // updated index to out of range: enter in fail case.
    index = NUMBER_OF_CORES_PER_DIE;
    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(index, &voltage_get_data);
    assert_int_not_equal(voltage_get_data.latest_value_mV, voltage_data.data.vcore0 * 1000);
}

TEST_FUNCTION(test_get_pwr_core_current_data, test_setup, test_teardown)
{
    pwr_core_element_current_t current_get_data = {0};
    uint8_t index = TEST_CORE_ID_5;
    current_t test_current = {0};
    core[index].current.latest_value_mA = 30;
    core[index].current.average_mA = 30;
    core[index].current.max_mA = 40;
    core[index].current.min_mA = 20;
    memcpy(&test_current, &core[TEST_CORE_ID_5].current, sizeof(core[TEST_CORE_ID_5].current));
    data_proc_tlm_cmpnt_get_pwr_core_current_data(index, &current_get_data);
    // Check core 0 current
    // check for valid data into full current  array.
    assert_int_equal(memcmp(&current_get_data, &test_current, sizeof(core[TEST_CORE_ID_5].current)), 0);
    // setup for fail case .
    core[index].current.latest_value_mA = 0;
    core[index].current.average_mA = 0;
    core[index].current.max_mA = 0;
    core[index].current.min_mA = 0;
    data_proc_tlm_cmpnt_get_pwr_core_current_data(NUMBER_OF_CORES_PER_DIE, &current_get_data);
    // check for valid data into full current array.

    assert_int_not_equal(memcmp(&current_get_data, &core[TEST_CORE_ID_5].current, sizeof(core[TEST_CORE_ID_5].current)), 0);
}

TEST_FUNCTION(test_get_pwr_core_temperature_data, test_setup, test_teardown)
{
    pwr_core_element_temperature_t temp_data = {0};
    temperature_t test_temperature = {0};

    core[TEST_CORE_ID_5].temperature.latest_value_dC = 30;
    core[TEST_CORE_ID_5].temperature.average_dC = 30;
    core[TEST_CORE_ID_5].temperature.max_dC = 40;
    core[TEST_CORE_ID_5].temperature.min_dC = 20;
    memcpy(&test_temperature, &core[TEST_CORE_ID_5].temperature, sizeof(core[TEST_CORE_ID_5].temperature));
    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, &temp_data);
    // check for valid data into full temperaure array.
    assert_int_equal(memcmp(&temp_data, &test_temperature, sizeof(test_temperature)), 0);

    // setup for fail case .
    core[TEST_CORE_ID_5].temperature.latest_value_dC = 0;
    core[TEST_CORE_ID_5].temperature.average_dC = 0;
    core[TEST_CORE_ID_5].temperature.max_dC = 0;
    core[TEST_CORE_ID_5].temperature.min_dC = 0;

    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(NUMBER_OF_CORES_PER_DIE, &temp_data);
    // check for valid data into full temperaure array.
    assert_int_not_equal(memcmp(&temp_data, &core[TEST_CORE_ID_5].temperature, sizeof(core[TEST_CORE_ID_5].temperature)), 0);
}

TEST_FUNCTION(test_get_pwr_core_histogram_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_element_histogram_t histogram_data[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES] = {{{0}}};
    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, &histogram_data);
}

TEST_FUNCTION(test_get_pwr_soc_pkg_mon_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_element_pkg_monitor_t pc3_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pc3_data);
}

TEST_FUNCTION(test_get_pwr_soc_vr_rail_data, test_setup, test_teardown)
{
    pwr_soc_element_vr_rail_t vr_rail_data = {{0}};
    vr_current_t data = {
        .timestamp = 0,
        .vr_current_mA = {10, 20, 30, 40, 50, 60, 70, 80},
        .vr_voltage_mV = {1000, 900, 800, 700, 600, 700, 800, 900},
    };
    vr_temp_t vr_temperature = {
        .timestamp = 0,
        .vr_temp_dC = {15, 25, 35, 45, 55, 65, 75, 85},
    };
    // Baseline log for vr current and voltage
    data_smpl_parse_vr_current_entry(&data);
    // Baseline log for vr temperature
    data_smpl_parse_vr_temperature_entry(&vr_temperature);

    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(TEST_RAIL_ID_2, &vr_rail_data);
    // Check VR Current and voltage for packaging api.
    assert_int_equal(vr_rail_data.current.latest_value_mA, (data.vr_current_mA[TEST_RAIL_ID_2]));
    assert_int_equal(vr_rail_data.voltage.latest_value_mV, (data.vr_voltage_mV[TEST_RAIL_ID_2]));
    assert_int_equal(vr_rail_data.temperature.latest_value_dC, (vr_temperature.vr_temp_dC[TEST_RAIL_ID_2]));

    // fail case for current, voltage and temperature.
    data.vr_current_mA[TEST_RAIL_ID_2] = 35;
    data.vr_voltage_mV[TEST_RAIL_ID_2] = 810;
    // Log again with new values and compare new valuse.
    data_smpl_parse_vr_current_entry(&data);
    vr_temperature.vr_temp_dC[TEST_RAIL_ID_2] = 45;
    data_smpl_parse_vr_temperature_entry(&vr_temperature);

    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(MAX_NUM_OF_VR_RAILS, &vr_rail_data);
    assert_int_not_equal(vr_rail_data.current.latest_value_mA, (data.vr_current_mA[TEST_RAIL_ID_2]));
    assert_int_not_equal(vr_rail_data.voltage.latest_value_mV, (data.vr_voltage_mV[TEST_RAIL_ID_2]));
    assert_int_not_equal(vr_rail_data.temperature.latest_value_dC, (vr_temperature.vr_temp_dC[TEST_RAIL_ID_2]));
}

TEST_FUNCTION(test_get_pwr_soc_hnf_data, test_setup, test_teardown)
{
    pwr_soc_element_hnf_t hnf_data = {0};
    uint8_t hnf_channel_id = 0;
    // to log the data .
    tile_temp_t temperature_data = {
        .timestamp = 7485463, // non -zero to run the calculation in logger.
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

    // for hnf channel id 0
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel_id, &hnf_data);
    assert_int_equal(hnf_data.latest_value_dC, (temperature_data.temp2.temp6));
    hnf_channel_id = TEST_HNF_CHANN_ID_1;
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel_id, &hnf_data);
    assert_int_equal(hnf_data.latest_value_dC, (temperature_data.temp2.temp7));

    // Fail case : update hnf data from logger on previously loggged data for HNF and test via packaging api.
    hnf_channel_id = 0;
    temperature_data.temp2.temp6 = 60; // First HNF on the tile.
    temperature_data.temp2.temp7 = 70; // Second HNF on the tile.
    data_smpl_parse_tile_temperature_entry(&temperature_data, hnf_channel_id);

    hnf_channel_id = NUMBER_OF_HNF_CHANNELS_PER_DIE; // max range should fail to get updated data.
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel_id, &hnf_data);
    // last read value in hnf_data was temp7(second sensor on the tile)so compare with that.
    assert_int_not_equal(hnf_data.latest_value_dC, (temperature_data.temp2.temp7));

    // Check fail case for first sensor on tile :temp6.
    hnf_data = {0};
    hnf_channel_id = NUMBER_OF_HNF_CHANNELS_PER_DIE; // max range should fail to get updated data.
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel_id, &hnf_data);
    assert_int_not_equal(hnf_data.latest_value_dC, (temperature_data.temp2.temp7));
}

TEST_FUNCTION(test_get_pwr_soc_dimm_data, test_setup, test_teardown)
{
    pwr_soc_element_dimm_temp_t dimm_data = {{0}};
    sensor_ram_dimm_info_t dimm_info = {
        .timestamp = 0,
        .dimm_temp_s0_dC = 26,
        .dimm_temp_s1_dC = 28,
        .dimm_power_mW = 100,
        .dimm_id = TEST_DIMM_CHANN_ID_3,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 0,
    };
    // Baseline log
    data_smpl_parse_dimm_entry(&dimm_info);

    // Check DIMM information
    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(TEST_DIMM_CHANN_ID_3, &dimm_data);
    assert_int_equal(dimm_data.s0.latest_value_dC, dimm_info.dimm_temp_s0_dC);
    assert_int_equal(dimm_data.s1.latest_value_dC, (dimm_info.dimm_temp_s1_dC));

    // setup and test for fail case.
    dimm_info.dimm_temp_s0_dC = 27;
    dimm_info.dimm_temp_s1_dC = 40;
    dimm_info.dimm_power_mW = 200;
    dimm_info.dimm_throttling = 1;
    dimm_info.dimm_memory_frequency_id = 100;

    data_smpl_parse_dimm_entry(&dimm_info);
    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(NUMBER_OF_DIMM_MODULES, &dimm_data);
    assert_int_not_equal(dimm_data.s0.latest_value_dC, (dimm_info.dimm_temp_s0_dC));
    assert_int_not_equal(dimm_data.s1.latest_value_dC, (dimm_info.dimm_temp_s1_dC));
}

TEST_FUNCTION(test_get_pwr_soc_snsr_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_sensor_temp_t snsr_temp_data = {0};

    soc_pvt_temp_t pvt_temperature = {
        .timestamp = 0,
        .sensor_temp_dC = {20, 30, 40, 50, 60, 26, 27, 21, 12, 21, 31, 13, 41, 14},
    };
    data_smpl_parse_pvt_temperature_entry(&pvt_temperature);

    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(TEST_SNSR_ID_0, &snsr_temp_data);
    // assert_int_equal(pvt_temperature.sensor_temp_dC[TEST_SNSR_ID_0], snsr_temp_data.latest_value_dC);
    assert_int_equal(memcmp(&snsr_temp_data.latest_value_dC,
                            &pvt_temperature.sensor_temp_dC[TEST_SNSR_ID_0],
                            sizeof(pvt_temperature.sensor_temp_dC[TEST_SNSR_ID_0])),
                     0);

    // Fail case, fill data with zero
    snsr_temp_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(NUMBER_OF_SOC_TEMP_SENSORS, &snsr_temp_data);
    assert_int_not_equal(pvt_temperature.sensor_temp_dC[TEST_SNSR_ID_0], snsr_temp_data.latest_value_dC);
}

TEST_FUNCTION(test_get_pwr_mpam_pstate_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_element_mpam_pstate_t mpam_pstate_array[NUMBER_OF_PSTATES] = {{{0}}};
    data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(TEST_MPAM_ID_4, &mpam_pstate_array);
}

TEST_FUNCTION(test_get_pwr_soc_mpam_throttle_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_element_mpam_throttle_t mpam_throttle_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(TEST_MPAM_ID_4, &mpam_throttle_data);
}

TEST_FUNCTION(test_get_inst_soc_core_summary_data, test_setup, test_teardown)
{
    inst_core_element_summary_t core_summary_data = {0};
    // pstate
    core[TEST_CORE_ID_5].throttling_status = NO_THROTTLE;
    core[TEST_CORE_ID_5].pstate_from_pstate_pkt = 10;
    uint8_t pstate_index = core[TEST_CORE_ID_5].pstate_from_pstate_pkt;
    core[TEST_CORE_ID_5].pstate[pstate_index].pstate_id = pstate_index;

    core[TEST_CORE_ID_5].average_pwr_mW = 10;
    core[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz = 150;
    // cstate
    core[TEST_CORE_ID_5].cstate_from_pstate_pkt = 2;
    uint16_t cstate_index = core[TEST_CORE_ID_5].cstate_from_pstate_pkt;
    core[TEST_CORE_ID_5].cstate[cstate_index].cstate_id = cstate_index;
    // core voltage
    core[TEST_CORE_ID_5].voltage.latest_value_mV = 3200;
    // core temperature and current,plimit.
    core[TEST_CORE_ID_5].current.latest_value_mA = 30;
    core[TEST_CORE_ID_5].temperature.latest_value_dC = 400;
    // plimit
    core[TEST_CORE_ID_5].active_sample_plimit = 1;

    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);
    assert_int_equal(core_summary_data.pstate, core[TEST_CORE_ID_5].pstate[pstate_index].pstate_id);
    assert_int_equal(core_summary_data.cstate, core[TEST_CORE_ID_5].cstate[cstate_index].cstate_id);

    assert_int_equal(core_summary_data.plimit, core[TEST_CORE_ID_5].active_sample_plimit);
    assert_int_equal(core_summary_data.power_mW, core[TEST_CORE_ID_5].latest_power_mW);
    assert_int_equal(core_summary_data.frequency_Mhz, core[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz);

    assert_int_equal(core_summary_data.voltage_mV, core[TEST_CORE_ID_5].voltage.latest_value_mV);
    assert_int_equal(core_summary_data.current_mA, core[TEST_CORE_ID_5].current.latest_value_mA);
    assert_int_equal(core_summary_data.temperature_dC, core[TEST_CORE_ID_5].temperature.latest_value_dC);
}

TEST_FUNCTION(test_get_inst_soc_rail_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    inst_soc_element_rail_t rail_data = {0};
    data_proc_tlm_cmpnt_get_inst_soc_rail_data(TEST_RAIL_ID_2, &rail_data);
}

TEST_FUNCTION(test_get_inst_soc_dimm_runtime_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    inst_soc_element_dimm_runtime_t dimm_runtime_data = {0};
    data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(TEST_DIMM_CHANN_ID_3, &dimm_runtime_data);
}

TEST_FUNCTION(test_get_inst_soc_snsr_temp_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    inst_soc_element_die_temp_t snsr_temp_data = {0};
    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(TEST_SNSR_ID_0, &snsr_temp_data);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_core_power_data, test_setup, test_teardown)
{
    // Test validates that the records published to consumers is using the correct data source

    pwr_core_element_power_t power_data = {0};

    uint8_t core_id = 0;
    computed_metrics_2_mins.cores[core_id].power_mW.min = 10;
    computed_metrics_2_mins.cores[core_id].power_mW.max = 20;
    computed_metrics_2_mins.cores[core_id].power_mW.running_avg.average = 15;

    data_proc_tlm_cmpnt_get_pwr_core_power_data(core_id, &power_data);

    assert_int_equal(power_data.min_mW, computed_metrics_2_mins.cores[core_id].power_mW.min);
    assert_int_equal(power_data.max_mW, computed_metrics_2_mins.cores[core_id].power_mW.max);
    assert_int_equal(power_data.average_mW, computed_metrics_2_mins.cores[core_id].power_mW.running_avg.average);
}
