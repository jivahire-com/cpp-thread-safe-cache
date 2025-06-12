//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_compute_metrics.cpp
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
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
}

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

//
// Tests
//

// Unit test for  comp_metrics_for_single_core_current
TEST_FUNCTION(test_comp_metrics_for_single_core_current, test_setup, test_teardown)
{

    // Feed a few values into the core current computation and validate the results
    uint8_t core_id = 0;
    uint16_t test_values_mA[5] = {18, 20, 30, 40, 50};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_current(core_id, test_values_mA[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.min, 18);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.max, 50);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.running_avg.average, 32);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.running_avg.num_samples, 5);
}

// Unit test for  comp_metrics_for_single_core_voltage
TEST_FUNCTION(test_comp_metrics_for_single_core_voltage, test_setup, test_teardown)
{
    // Feed a few values into the core power computation and validate the results
    uint8_t core_id = 0;
    uint16_t test_values_mV[5] = {180, 200, 300, 400, 500};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_voltage(core_id, test_values_mV[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.min, 180);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.max, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.average, 316);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.num_samples, 5);
}

// Unit test for  comp_metrics_for_single_core_temperature
TEST_FUNCTION(test_comp_metrics_for_single_core_temperature, test_setup, test_teardown)
{
    uint8_t core_id = 0;

    // Test case: Initial values
    computed_metrics_2_mins.cores[core_id].temperature_dC.min = 0;
    computed_metrics_2_mins.cores[core_id].temperature_dC.max = 0;
    computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg.average = 0;

    comp_metrics_for_single_core_temperature(core_id, 500);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg.average, 500);

    // Test case: Update with new latest value
    comp_metrics_for_single_core_temperature(core_id, 600);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 600);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg.average, 550);

    // Test case: Update with lower latest value
    comp_metrics_for_single_core_temperature(core_id, 400);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 400);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 600);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg.average, 500);
}

// Unit test for  comp_metrics_for_single_tile_vcpu
TEST_FUNCTION(test_comp_metrics_for_single_tile_vcpu, test_setup, test_teardown)
{
    uint8_t tile_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    tile[tile_id].vcpu.min_mV = 0;
    tile[tile_id].vcpu.max_mV = 0;
    tile[tile_id].vcpu.average_mV = 0;
    tile[tile_id].vcpu.latest_value_mV = 1200;
    comp_metrics_for_single_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1200); // first value will be 1200

    // Test case: Update with new latest value
    tile[tile_id].vcpu.latest_value_mV = 1300;
    comp_metrics_for_single_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1200);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1300);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    tile[tile_id].vcpu.latest_value_mV = 1100;
    comp_metrics_for_single_tile_vcpu(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vcpu.min_mV, 1100);
    assert_int_equal(tile[tile_id].vcpu.max_mV, 1300);
    assert_int_equal(tile[tile_id].vcpu.average_mV, 1175); // (1250 + 1100) / 2
}

// Unit test for comp_metrics_for_single_tile_vsys
TEST_FUNCTION(test_comp_metrics_for_single_tile_vsys, test_setup, test_teardown)
{
    uint8_t tile_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    tile[tile_id].vsys.min_mV = 0;
    tile[tile_id].vsys.max_mV = 0;
    tile[tile_id].vsys.average_mV = 0;
    tile[tile_id].vsys.latest_value_mV = 1200;
    comp_metrics_for_single_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1200); // Init value to 1200

    // Test case: Update with new latest value
    tile[tile_id].vsys.latest_value_mV = 1300;
    comp_metrics_for_single_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1200);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1300);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    tile[tile_id].vsys.latest_value_mV = 1100;
    comp_metrics_for_single_tile_vsys(tile_id, time_diff_uS, residency_uS);
    assert_int_equal(tile[tile_id].vsys.min_mV, 1100);
    assert_int_equal(tile[tile_id].vsys.max_mV, 1300);
    assert_int_equal(tile[tile_id].vsys.average_mV, 1175); // (1250 + 1100) / 2
}

TEST_FUNCTION(test_comp_metrics_for_soc_rail_voltage, test_setup, test_teardown)
{
    uint8_t vr_index = 2;

    computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.min = 1000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.max = 2000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.running_avg.average = 1500;
    computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.running_avg.num_samples = 3;

    soc_info.latest_rail_voltage_mV[vr_index] = 3000;
    // Call the function to be tested
    comp_metrics_for_soc_rail_voltage(&soc_info.latest_rail_voltage_mV);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.min, 1000);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.max, 3000);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.running_avg.average, 1875);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].voltage_mV.running_avg.num_samples, 4);
}

TEST_FUNCTION(test_comp_metrics_for_soc_rail_current, test_setup, test_teardown)
{
    uint8_t vr_index = 2;

    computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.min = 500;
    computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.max = 1000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.running_avg.average = 750;
    computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.running_avg.num_samples = 3;

    soc_info.latest_rail_current_mA[vr_index] = 2000;
    // Call the function to be tested
    comp_metrics_for_soc_rail_current(&soc_info.latest_rail_current_mA);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.min, 500);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.max, 2000);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.running_avg.average, 1063);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].current_mA.running_avg.num_samples, 4);
}

TEST_FUNCTION(test_comp_metrics_for_soc_rail_TEMP, test_setup, test_teardown)
{
    uint8_t vr_index = 4;

    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min = 500;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max = 1000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.average = 750;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples = 3;

    soc_info.latest_rail_temperature_dC[vr_index] = 2000;
    // Call the function to be tested
    comp_metrics_for_soc_rail_temperature(&soc_info.latest_rail_temperature_dC);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max, 2000);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.average, 1063);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples, 4);
}

TEST_FUNCTION(test_comp_metrics_for_single_hnf_channel, test_setup, test_teardown)
{
    uint8_t hnf_channel = 2;

    // Initialize soc_info with some values
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min = 30;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max = 70;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.average = 50;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.num_samples = 1;

    // Call the function to be tested
    comp_metrics_for_single_hnf_channel(hnf_channel, 60);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min, 30);
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max, 70);
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.average, 55);
}

TEST_FUNCTION(test_comp_metrics_for_soc_top_temp_sensor, test_setup, test_teardown)
{
    // Arrange
    uint16_t test_values[NUMBER_OF_SOC_TEMP_SENSORS];
    for (uint16_t i = 0; i < NUMBER_OF_SOC_TEMP_SENSORS; ++i)
    {
        test_values[i] = 20 + i; // Assign unique values for each sensor
    }

    // Act
    comp_metrics_for_soc_top_temp_sensor(&test_values);

    // Assert
    for (uint16_t i = 0; i < NUMBER_OF_SOC_TEMP_SENSORS; ++i)
    {
        assert_int_equal(computed_metrics_2_mins.soc.top_sensor_temp_dC[i].min, test_values[i]);
        assert_int_equal(computed_metrics_2_mins.soc.top_sensor_temp_dC[i].max, test_values[i]);
        assert_int_equal(computed_metrics_2_mins.soc.top_sensor_temp_dC[i].running_avg.average, test_values[i]);
    }
}

TEST_FUNCTION(test_comp_metrics_for_single_soc_dimm_temp, test_setup, test_teardown)
{

    uint8_t dimm_id = 0;
    uint16_t test_values_dC[5][2] = {
        {180, 300},
        {190, 310},
        {200, 320},
        {210, 330},
        {220, 340},
    };

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_soc_dimm_temp(dimm_id, test_values_dC[i][0], test_values_dC[i][1]);
    }

    // Check the results

    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.min, 180);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.max, 220);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.running_avg.average, 200);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_power_per_pstate, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    int pstate_index = 0;
    // Feed a few values into the core power computation and validate the results
    uint16_t test_values_mW[5] = {100, 200, 300, 400, 500};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power_per_pstate(core_id, pstate_index, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.min, 100);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.max, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.running_avg.average, 300);
}

// comp_metrics_for_single_core_throttling_pstate(core_id, i, time_diff_uS, core[core_id].throttle_info[i].residency_mS);
TEST_FUNCTION(test_comp_metrics_for_single_core_throttling_pstate, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = 1;
    uint64_t time_stamp_uS = 5000;

    core[core_id].core_throttling_tracker[throttle_index] = 1;
    core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index] = 1000;
    core[core_id].latest_pstate = core[core_id].pstate_from_current_pkt = 20;

    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS = 10;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate = 10;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate = 5;
    uint32_t time_diff_uS = time_stamp_uS - core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index];
    comp_metrics_for_single_core_throttling_pstate(
        core_id,
        throttle_index,
        time_diff_uS,
        computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS,
        core[core_id].latest_pstate);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].max_pstate, 20);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].avg_pstate, 14);
}

// Unit test function
TEST_FUNCTION(test_comp_metrics_for_tiles_for_sampling_period, test_setup, test_teardown)
{
    // Initialize tile data for testing
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        tile[tile_id].time_counter_uS = 0;
        tile[tile_id].latest_max_temp_dC = 50;
        tile[tile_id].latest_max_temp_sensor_index = 1;
    }
    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    comp_metrics_for_tiles_for_sampling_period();

    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        assert_int_equal(tile[tile_id].time_counter_uS, 0);
        assert_int_equal(tile[tile_id].latest_max_temp_dC, 50);
    }

    uint8_t mock_max_tile_temp_dC = 60;
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {

        tile[tile_id].latest_max_temp_dC = mock_max_tile_temp_dC;
        mock_max_tile_temp_dC++;
        tile[tile_id].latest_max_temp_sensor_index = 1;
    }

    //  Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);

    comp_metrics_for_tiles_for_sampling_period();
    mock_max_tile_temp_dC = 60;
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        assert_int_equal(tile[tile_id].time_counter_uS, 5);
        assert_int_equal(tile[tile_id].latest_max_temp_dC, mock_max_tile_temp_dC);
        mock_max_tile_temp_dC++;
    }
}

TEST_FUNCTION(test_comp_metrics_for_single_core_power, test_setup, test_teardown)
{

    // Feed a few values into the core power computation and validate the results
    uint8_t core_id = 0;
    uint16_t test_values_mW[5] = {100, 200, 300, 400, 500};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power(core_id, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.min, 100);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.max, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.running_avg.average, 300);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.running_avg.num_samples, 5);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_throttle_overrun_count_update, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = THROTTLE_SOURCE_CURRENT;

    comp_metrics_for_single_core_single_throttle_overrun_count_update(core_id, throttle_index);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].overrun_count, 1);
    throttle_index = THROTTLE_SOURCE_ADAPTIVE_CLK;
    comp_metrics_for_single_core_single_throttle_overrun_count_update(core_id, throttle_index);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].overrun_count, 1);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_throttle_update, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t throttle_index = THROTTLE_SOURCE_CURRENT;
    uint64_t timestamp_diff_uS = 1000;
    bool throttle_start = true;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].entry_count = 0;
    computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS = 10;

    comp_metrics_for_single_core_single_throttle_update(core_id, throttle_index, timestamp_diff_uS, throttle_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS, 10);

    throttle_start = false;
    comp_metrics_for_single_core_single_throttle_update(core_id, throttle_index, timestamp_diff_uS, throttle_start);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_index].residency_mS, 11);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_rack_throttle_update, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint8_t priority_id = 0;
    uint64_t timestamp_diff_uS = 1000;
    computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].residency_mS = 10;
    bool rack_throttle_priority_start = true;

    comp_metrics_for_single_core_single_rack_throttle_update(core_id, priority_id, timestamp_diff_uS, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].residency_mS, 10);

    rack_throttle_priority_start = false;
    comp_metrics_for_single_core_single_rack_throttle_update(core_id, priority_id, timestamp_diff_uS, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[priority_id].residency_mS, 11);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_pstate, test_setup, test_teardown)
{
    uint8_t core_id = 5;
    uint8_t pstate_id = 0;
    uint64_t timestamp_diff_uS = 1000;
    uint8_t update_pstate_entry = 1;
    comp_metrics_for_single_core_single_pstate(core_id, pstate_id, timestamp_diff_uS, update_pstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_id].residency_uS, 1000);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_id].entry_count, 1);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_cstate, test_setup, test_teardown)
{
    uint8_t core_id = 5;
    uint8_t cstate_id = 0;
    uint64_t timestamp_diff_uS = 1000;
    uint8_t update_cstate_entry = 1;
    comp_metrics_for_single_core_single_cstate(core_id, cstate_id, timestamp_diff_uS, update_cstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[cstate_id].residency_uS, 1000);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[cstate_id].entry_count, 1);
}

TEST_FUNCTION(test_comp_metrics_reset_2_mins_metrics, test_setup, test_teardown)
{
    memset(&computed_metrics_2_mins, 0xFF, sizeof(computed_metrics_2_mins));
    comp_metrics_reset_2_mins_metrics();
    for (uint32_t byte = 0; byte < sizeof(computed_metrics_2_mins); byte++)
    {
        assert_int_equal(((uint8_t*)&computed_metrics_2_mins)[byte], 0);
    }
}

TEST_FUNCTION(test_comp_metrics_reset_24_hrs_metrics, test_setup, test_teardown)
{
    memset(&computed_metrics_24_hrs, 0xFF, sizeof(computed_metrics_24_hrs));
    comp_metrics_reset_24_hrs_metrics();
    for (uint32_t byte = 0; byte < sizeof(computed_metrics_24_hrs); byte++)
    {
        assert_int_equal(((uint8_t*)&computed_metrics_24_hrs)[byte], 0);
    }
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core, test_setup, test_teardown)
{
    memset(&computed_metrics_d2d_2mins, 0xFF, sizeof(computed_metrics_d2d_2mins));
    data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core();
    for (uint32_t byte = 0; byte < sizeof(computed_metrics_d2d_2mins); byte++)
    {
        assert_int_equal(((uint8_t*)&computed_metrics_d2d_2mins)[byte], 0);
    }
}
