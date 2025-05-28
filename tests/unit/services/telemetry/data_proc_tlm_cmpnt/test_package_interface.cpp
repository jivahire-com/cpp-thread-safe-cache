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
#include <die_2_die_exchange_i.h>
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
    comp_metrics_reset_2_mins_metrics();
    comp_metrics_reset_24_hrs_metrics();
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
    pwr_core_element_power_t power_data = {0};

    uint8_t core_id = 0;
    computed_metrics_2_mins.cores[core_id].voltage_mV.min = 1200;
    computed_metrics_2_mins.cores[core_id].voltage_mV.max = 1800;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.average = 1500;

    data_proc_tlm_cmpnt_get_pwr_core_power_data(core_id, &power_data);

    assert_int_equal(power_data.min_mW, computed_metrics_2_mins.cores[core_id].power_mW.min);
    assert_int_equal(power_data.max_mW, computed_metrics_2_mins.cores[core_id].power_mW.max);
    assert_int_equal(power_data.average_mW, computed_metrics_2_mins.cores[core_id].power_mW.running_avg.average);
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

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.average = 30;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max = 40;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min = 20;

    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, &temp_data);

    assert_int_equal(temp_data.min_dC, computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min);
    assert_int_equal(temp_data.max_dC, computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max);
    assert_int_equal(temp_data.average_dC,
                     computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.average);

    // setup for fail case .

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.average = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min = 0;

    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(NUMBER_OF_CORES_PER_DIE, &temp_data);
    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, nullptr);
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
    pwr_soc_element_vr_rail_t rail_data = {{0}};
    uint16_t rail_id = TEST_RAIL_ID_2;

    // Set up test values in computed_metrics_2_mins for this rail
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.running_avg.average = 123;
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.max = 150;
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.min = 100;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.running_avg.average = 1100;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.max = 1200;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.min = 1000;
    computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.running_avg.average = 55;
    computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.max = 60;
    computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.min = 50;

    // Call the API
    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(rail_id, &rail_data);

    // Check current
    assert_int_equal(rail_data.current.average_mA, 123);
    assert_int_equal(rail_data.current.max_mA, 150);
    assert_int_equal(rail_data.current.min_mA, 100);
    // Check voltage
    assert_int_equal(rail_data.voltage.average_mV, 1100);
    assert_int_equal(rail_data.voltage.max_mV, 1200);
    assert_int_equal(rail_data.voltage.min_mV, 1000);
    // Check temperature
    assert_int_equal(rail_data.temperature.average_dC, 55);
    assert_int_equal(rail_data.temperature.max_dC, 60);
    assert_int_equal(rail_data.temperature.min_dC, 50);

    // Negative test: invalid rail_id
    memset(&rail_data, 0, sizeof(rail_data));
    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(MAX_NUM_OF_VR_RAILS, &rail_data);
    // Should not match the values set above
    assert_int_not_equal(rail_data.current.average_mA, 123);
    assert_int_not_equal(rail_data.voltage.average_mV, 1100);
    assert_int_not_equal(rail_data.temperature.average_dC, 55);

    // Negative test: null pointer
    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(rail_id, NULL);
}

TEST_FUNCTION(test_get_pwr_soc_hnf_data, test_setup, test_teardown)
{
    pwr_soc_element_hnf_t hnf_data = {0};
    uint8_t hnf_channel = 4;

    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.average = 100;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max = 200;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min = 40;

    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel, &hnf_data);
    assert_int_equal(hnf_data.average_dC, 100);
    assert_int_equal(hnf_data.max_dC, 200);
    assert_int_equal(hnf_data.min_dC, 40);

    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(NUMBER_OF_HNF_CHANNELS_PER_DIE, &hnf_data);
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(hnf_channel, nullptr);
}

TEST_FUNCTION(test_get_pwr_soc_dimm_data, test_setup, test_teardown)
{
    pwr_soc_element_dimm_temp_t dimm_data = {{0}};

    // Check DIMM information

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.running_avg.average = 250;

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.running_avg.average = 250;

    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(TEST_DIMM_CHANN_ID_3, &dimm_data);

    assert_int_equal(dimm_data.s0.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.max);
    assert_int_equal(dimm_data.s0.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.min);
    assert_int_equal(dimm_data.s0.average_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s0_dC.running_avg.average);

    assert_int_equal(dimm_data.s1.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.max);
    assert_int_equal(dimm_data.s1.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.min);
    assert_int_equal(dimm_data.s1.average_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_CHANN_ID_3].temperature_s1_dC.running_avg.average);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_sensor_temp_t sensor_temp_data = {0};
    uint16_t sensor_id = 1;

    // Set up test values in computed_metrics_2_mins for this sensor
    computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].running_avg.average = 77;
    computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].max = 88;
    computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].min = 66;

    // Call the API
    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(sensor_id, &sensor_temp_data);

    // Check values
    assert_int_equal(sensor_temp_data.average_dC, 77);
    assert_int_equal(sensor_temp_data.max_dC, 88);
    assert_int_equal(sensor_temp_data.min_dC, 66);

    // Negative test: invalid sensor_id
    memset(&sensor_temp_data, 0, sizeof(sensor_temp_data));
    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(NUMBER_OF_SOC_TEMP_SENSORS, &sensor_temp_data);
    assert_int_not_equal(sensor_temp_data.average_dC, 77);
    assert_int_not_equal(sensor_temp_data.max_dC, 88);
    assert_int_not_equal(sensor_temp_data.min_dC, 66);

    // Negative test: null pointer
    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(sensor_id, NULL);
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
    core[TEST_CORE_ID_5].latest_voltage_mV = 3200;
    // core temperature and current,plimit.
    core[TEST_CORE_ID_5].current.latest_value_mA = 30;
    core[TEST_CORE_ID_5].latest_max_value_dC = 400;
    // plimit
    core[TEST_CORE_ID_5].active_sample_plimit = 1;

    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);
    assert_int_equal(core_summary_data.pstate, core[TEST_CORE_ID_5].pstate[pstate_index].pstate_id);
    assert_int_equal(core_summary_data.cstate, core[TEST_CORE_ID_5].cstate[cstate_index].cstate_id);

    assert_int_equal(core_summary_data.plimit, core[TEST_CORE_ID_5].active_sample_plimit);
    assert_int_equal(core_summary_data.power_mW, core[TEST_CORE_ID_5].latest_power_mW);
    assert_int_equal(core_summary_data.frequency_Mhz, core[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz);

    assert_int_equal(core_summary_data.voltage_mV, core[TEST_CORE_ID_5].latest_voltage_mV);
    assert_int_equal(core_summary_data.current_mA, core[TEST_CORE_ID_5].current.latest_value_mA);
    assert_int_equal(core_summary_data.temperature_dC, core[TEST_CORE_ID_5].latest_max_value_dC);
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

TEST_FUNCTION(test_get_inst_soc_sensor_temp_data, test_setup, test_teardown)
{
    inst_soc_element_max_temp_t read_data = {0};

    soc_info.latest_max_die_temp_dC = 200;
    die_2_die_exchange_write_max_die_temp(400);
    data_proc_tlm_cmpnt_get_inst_soc_max_temp_data(&read_data);

    assert_int_equal(read_data.die0_max_temperature_dC, 200);
    assert_int_equal(read_data.die1_max_temperature_dC, 400);
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
