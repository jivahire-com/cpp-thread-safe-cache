//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_package_interface.cpp
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
#include <dvfs.h>
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
#define TEST_SNSR_ID_0      (0)
#define TEST_HNF_CHANN_ID_1 (1)
#define TEST_RAIL_ID_2      (2)
#define TEST_DIMM_MOD_ID_3  (3)
#define TEST_MPAM_ID_4      (4)
#define TEST_CORE_ID_5      (5)

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

    for (uint16_t pstate_id = 0; pstate_id < NUMBER_OF_PSTATES; pstate_id++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.average = 100;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.min = 50;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.max = 150;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].entry_count = 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].residency_uS = 1000;
    }

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(TEST_CORE_ID_5, &pstate_array);
    // check for valid data into full pstate_array .
    // Every core will have NUMBER_OF_PSTATES, and each pstate elemenet have , it's own data
    for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
    {

        assert_int_equal(
            pstate_array[pstate_index].avg_power_mW,
            computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].power_mW.running_avg.average);
        assert_int_equal(pstate_array[pstate_index].min_power_mW,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].power_mW.min);
        assert_int_equal(pstate_array[pstate_index].max_power_mW,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].power_mW.max);

        assert_int_equal(pstate_array[pstate_index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].residency_uS / 1000);
        assert_int_equal(pstate_array[pstate_index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].entry_count);
    }
    // setup for failure case, change pstate 0 element data.
    uint8_t index = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.running_avg.average = 105;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.min = 55;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.max = 155;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].entry_count = 2;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].residency_uS = 2000;

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(NUMBER_OF_CORES_PER_DIE, &pstate_array);

    // verify for pstate 0
    assert_int_not_equal(pstate_array[index].avg_power_mW,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.running_avg.average);
    assert_int_not_equal(pstate_array[index].min_power_mW,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.min);
    assert_int_not_equal(pstate_array[index].max_power_mW,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.max);
    assert_int_not_equal(pstate_array[index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].residency_uS / 1000);
    assert_int_not_equal(pstate_array[index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].entry_count);
}

TEST_FUNCTION(test_get_pwr_core_cstate_data, test_setup, test_teardown)
{
    pwr_core_element_cstate_t cstate_array[NUMBER_OF_CSTATES] = {{0}};
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].entry_count = 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].residency_uS = 1000;
    }
    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(TEST_CORE_ID_5, &cstate_array);
    // check for valid data into full cstate_array .
    // Every core will have NUMBER_OF_CSTATES, and each cstate elemenet have , it's own data
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        assert_int_equal(cstate_array[cstate_index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].residency_uS / 1000);
        assert_int_equal(cstate_array[cstate_index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].entry_count);
    }

    // setup for failure case, change pstate 0 element data.
    uint8_t index = 0;

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[index].entry_count = 3;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[index].residency_uS = 2000;

    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(NUMBER_OF_CORES_PER_DIE, &cstate_array);

    // verify for cstate 0
    assert_int_not_equal(cstate_array[index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[index].residency_uS / 1000);
    assert_int_not_equal(cstate_array[index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[index].entry_count);
}

TEST_FUNCTION(test_get_pwr_core_throttle_data, test_setup, test_teardown)
{
    pwr_core_element_throttle_t throttle_array[NUMBER_OF_THROTTLE_TYPES] = {{0}};
    uint8_t throttle_index = 0;
    uint8_t avg_pstate = 5;
    uint32_t entry_count = 10;
    uint32_t residency_mS = 100;
    uint8_t max_pstate = 5;

    for (throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].avg_pstate = avg_pstate;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].entry_count = entry_count;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].max_pstate = max_pstate;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].residency_mS = residency_mS;
    }
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(TEST_CORE_ID_5, &throttle_array);

    for (throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
    {
        assert_int_equal(throttle_array[throttle_index].avg_pstate, avg_pstate);
        assert_int_equal(throttle_array[throttle_index].entry_count, entry_count);
        assert_int_equal(throttle_array[throttle_index].max_pstate, max_pstate);
        assert_int_equal(throttle_array[throttle_index].residency_mS, residency_mS);
        assert_int_equal(throttle_array[throttle_index].type_id, throttle_index);
    }

    // setup for failure case.
    uint8_t index = 0; // test for one of the throttle type.
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].avg_pstate = 6;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].entry_count = 12;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].max_pstate = 28;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].residency_mS = 110;
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(NUMBER_OF_CORES_PER_DIE, &throttle_array);

    throttle_index = 0;

    assert_int_not_equal(throttle_array[throttle_index].avg_pstate,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].avg_pstate);
    assert_int_not_equal(throttle_array[throttle_index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].entry_count);
    assert_int_not_equal(throttle_array[throttle_index].max_pstate,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].max_pstate);
    assert_int_not_equal(throttle_array[throttle_index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_index].residency_mS);
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
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].priority_id = rack_index;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].entry_count = 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].residency_mS = 100;
    }

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(TEST_CORE_ID_5, &rack_priority_array);

    for (rack_index = 0; rack_index < NUMBER_OF_RACK_PRIORITIES; rack_index++)
    {
        assert_int_equal(rack_priority_array[rack_index].priority_id,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].priority_id);
        assert_int_equal(rack_priority_array[rack_index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].entry_count);
        assert_int_equal(rack_priority_array[rack_index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].residency_mS);
    }
    rack_index = 0;
    // test for failure case .
    // change data in core data structure for rack priority 0 only.
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].entry_count = 2;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].residency_mS = 110;

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(NUMBER_OF_CORES_PER_DIE, &rack_priority_array);

    assert_int_equal(rack_priority_array[rack_index].priority_id,
                     computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].priority_id);
    assert_int_not_equal(rack_priority_array[rack_index].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].entry_count);
    assert_int_not_equal(rack_priority_array[rack_index].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_index].residency_mS);
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
{
    pwr_core_element_voltage_t voltage_data = {0};

    uint8_t core_id = 0;
    computed_metrics_2_mins.cores[core_id].voltage_mV.min = 1200;
    computed_metrics_2_mins.cores[core_id].voltage_mV.max = 1800;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.average = 1500;

    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(core_id, &voltage_data);

    assert_int_equal(voltage_data.min_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.min);
    assert_int_equal(voltage_data.max_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.max);
    assert_int_equal(voltage_data.average_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.average);

    core_id = NUMBER_OF_CORES_PER_DIE;
    // Invalid case
    computed_metrics_2_mins.cores[core_id].voltage_mV.min = 2200;
    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(core_id, &voltage_data);
    assert_int_not_equal(voltage_data.min_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.min);
}

TEST_FUNCTION(test_get_pwr_core_current_data, test_setup, test_teardown)
{
    pwr_core_element_current_t current_get_data = {0};
    uint8_t index = TEST_CORE_ID_5;

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.average = 30;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max = 40;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min = 20;

    data_proc_tlm_cmpnt_get_pwr_core_current_data(index, &current_get_data);

    assert_int_equal(current_get_data.min_mA, computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min);
    assert_int_equal(current_get_data.max_mA, computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max);
    assert_int_equal(current_get_data.average_mA,
                     computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.average);

    // setup for fail case .
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.average = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min = 0;

    data_proc_tlm_cmpnt_get_pwr_core_current_data(NUMBER_OF_CORES_PER_DIE, &current_get_data);
    assert_int_not_equal(current_get_data.average_mA, 0);
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

TEST_FUNCTION(test_get_pwr_soc_dimm_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_dimm_temp_t dimm_data = {{0}};

    // Check DIMM information

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.running_avg.average = 250;

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.running_avg.average = 250;

    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(TEST_DIMM_MOD_ID_3, &dimm_data);

    assert_int_equal(dimm_data.s0.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.max);
    assert_int_equal(dimm_data.s0.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min);
    assert_int_equal(dimm_data.s0.average_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.running_avg.average);

    assert_int_equal(dimm_data.s1.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.max);
    assert_int_equal(dimm_data.s1.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.min);
    assert_int_equal(dimm_data.s1.average_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.running_avg.average);

    // Invalid case
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min = 400;
    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(NUMBER_OF_DIMM_MODULES_PER_DIE, &dimm_data);
    assert_int_not_equal(dimm_data.s0.min_dC,
                         computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min);
}

TEST_FUNCTION(test_get_pwr_soc_dimm_power_data, test_setup, test_teardown)
{

    pwr_soc_element_dimm_power_t dimm_data = {{0}};
    // Check DIMM information

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.running_avg.average = 250;

    // Check DIMM information
    data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(TEST_DIMM_MOD_ID_3, &dimm_data);

    assert_int_equal(dimm_data.power_mW.average_mW,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.running_avg.average);

    assert_int_equal(dimm_data.power_mW.min_mW, computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min);

    assert_int_equal(dimm_data.power_mW.max_mW, computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.max);

    // Invalid case
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min = 300;
    data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(NUMBER_OF_DIMM_MODULES_PER_DIE, &dimm_data);
    assert_int_not_equal(dimm_data.power_mW.min_mW,
                         computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min);
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

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_max_soc_temp_t max_temp_data = {0};

    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(500, 10, 600);

    computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.average = 300;
    computed_metrics_d2d_2mins.max_soc_temp_dC.max = 400;
    computed_metrics_d2d_2mins.max_soc_temp_dC.min = 200;
    computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples = 5;

    data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(&max_temp_data);

    assert_int_equal(max_temp_data.average_max_dC, 433);
    assert_int_equal(max_temp_data.peak_max_dC, 600);

    computed_metrics_d2d_2mins.max_soc_temp_dC.max = 800;
    data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(&max_temp_data);
    assert_int_equal(max_temp_data.peak_max_dC, 800);
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
    uint8_t pstate_index = core[TEST_CORE_ID_5].latest_pstate;

    // core[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz = 150;
    //  cstate
    core[TEST_CORE_ID_5].latest_cstate = 2;
    uint16_t cstate_index = core[TEST_CORE_ID_5].latest_cstate;

    // core voltage
    core[TEST_CORE_ID_5].latest_voltage_mV = 3200;
    // core temperature and current,plimit.
    core[TEST_CORE_ID_5].latest_current_mA = 30;
    core[TEST_CORE_ID_5].latest_max_value_dC = 400;
    // plimit
    core[TEST_CORE_ID_5].active_sample_plimit = 1;

    uint8_t core_id = TEST_CORE_ID_5;
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    { // make all active
        core[core_id].core_throttling_tracker[i] = 1;
    }
    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);
    assert_int_equal(core_summary_data.pstate, pstate_index);
    assert_int_equal(core_summary_data.cstate, cstate_index);

    assert_int_equal(core_summary_data.plimit, core[TEST_CORE_ID_5].active_sample_plimit);
    assert_int_equal(core_summary_data.power_mW, core[TEST_CORE_ID_5].latest_power_mW);
    assert_int_equal(core_summary_data.frequency_Mhz, dvfs_get_freq_from_plimit(pstate_index));

    assert_int_equal(core_summary_data.voltage_mV, core[TEST_CORE_ID_5].latest_voltage_mV);
    assert_int_equal(core_summary_data.current_mA, core[TEST_CORE_ID_5].latest_current_mA);
    assert_int_equal(core_summary_data.temperature_dC, core[TEST_CORE_ID_5].latest_max_value_dC);
    assert_int_equal(core_summary_data.throttling_type, 0x7f);
    assert_int_equal(core_summary_data.throttling_rack_priority, core[TEST_CORE_ID_5].latest_rack_throttling_priority_id);
}

TEST_FUNCTION(test_get_inst_soc_rail_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    inst_soc_element_rail_t rail_data = {0};
    data_proc_tlm_cmpnt_get_inst_soc_rail_data(TEST_RAIL_ID_2, &rail_data);
    data_proc_tlm_cmpnt_get_inst_soc_rail_data(MAX_NUM_OF_VR_RAILS, &rail_data);
}

TEST_FUNCTION(test_get_inst_soc_dimm_runtime_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    inst_soc_element_dimm_runtime_t dimm_runtime_data = {0};
    data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(TEST_DIMM_MOD_ID_3, &dimm_runtime_data);
    data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(NUMBER_OF_DIMM_MODULES_PER_DIE, &dimm_runtime_data);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data, test_setup, test_teardown)
{
    inst_soc_element_die_temp_t sensor_temp_data = {0};

    soc_info.latest_soc_top_temp_dC[7] = 0x23;
    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(7, &sensor_temp_data);

    assert_int_equal(sensor_temp_data.temperature_dC, 0x23);

    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(NUMBER_OF_SOC_TEMP_SENSORS, &sensor_temp_data);
    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(7, nullptr);
}

TEST_FUNCTION(test_get_inst_soc_sensor_temp_data, test_setup, test_teardown)
{
    inst_soc_element_max_temp_t read_data = {0};

    soc_info.latest_max_die_temp_dC = 200;
    die_2_die_exch_ib_write_inst_max_die_temp(400);
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
    // invalid case
    computed_metrics_2_mins.cores[core_id].power_mW.min = 20;
    data_proc_tlm_cmpnt_get_pwr_core_power_data(NUMBER_OF_CORES_PER_DIE, &power_data);
    assert_int_not_equal(power_data.min_mW, computed_metrics_2_mins.cores[core_id].power_mW.min);
}
