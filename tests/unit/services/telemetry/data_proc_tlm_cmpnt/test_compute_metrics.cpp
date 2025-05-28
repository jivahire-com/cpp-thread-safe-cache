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

extern uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES]; // /* used only for MPAM*/
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

TEST_FUNCTION(test_comp_metrics_for_cores_for_sampling_period, test_setup, test_teardown)
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
    comp_metrics_for_cores_for_sampling_period();

    // Add assertions to verify the expected behavior
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core[core_id].time_counter_uS, 0);
    }

    // Test to updae : core[core_id].time_counter_uS in all core by time diff
    //  Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);

    // Call the function to be tested
    comp_metrics_for_cores_for_sampling_period();

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(core[core_id].time_counter_uS, 5);
    }
}

// Unit test for  comp_metrics_for_single_core_current
TEST_FUNCTION(test_comp_metrics_for_single_core_current, test_setup, test_teardown)
{

    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].current.min_mA = 0;
    core[core_id].current.max_mA = 0;
    core[core_id].current.average_mA = 0;
    core[core_id].current.latest_value_mA = 50;

    comp_metrics_for_single_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 50);
    assert_int_equal(core[core_id].current.max_mA, 50);
    assert_int_equal(core[core_id].current.average_mA, 50); // init avg is 0 so avg will be assigned to 50

    // Test case: Update with new latest value
    core[core_id].current.latest_value_mA = 100;
    comp_metrics_for_single_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 50);
    assert_int_equal(core[core_id].current.max_mA, 100);
    assert_int_equal(core[core_id].current.average_mA, 75); // (50 + 100) / 2

    // Test case: Update with lower latest value
    core[core_id].current.latest_value_mA = 30;
    comp_metrics_for_single_core_current(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].current.min_mA, 30);
    assert_int_equal(core[core_id].current.max_mA, 100);
    assert_int_equal(core[core_id].current.average_mA, 52); // (75 + 30) / 2
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

TEST_FUNCTION(test_comp_metrics_for_single_core_pstate, test_setup, test_teardown)
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

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

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

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 5);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 5);

    // Test case: id_change_bit set
    core[core_id].flags.id_change_bit = 1;
    core[core_id].flags.pstate_change = 0;

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].flags.id_change_bit, 0);
    assert_int_equal(core[core_id].flags.pstate_change, 0);
    assert_int_equal(pstate_accum_uS[core_id][pstate_index], 0);

    // Test case: pstate_change set
    core[core_id].flags.id_change_bit = 0;
    core[core_id].flags.pstate_change = 1;

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].flags.id_change_bit, 0);
    assert_int_equal(core[core_id].flags.pstate_change, 0);
    assert_int_equal(pstate_accum_uS[core_id][pstate_index], 0);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_pstate_pwr, test_setup, test_teardown)
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

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].min_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].max_power_mW, 10);
    assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, 10);

    // update pstate index and
    core[core_id].num_pwr_samples++;
    core_pwr_samples_accumulation_mW[core_id] += 20;
    core[core_id].pstate[pstate_index].residency_uS += 100;

    comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

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

        comp_metrics_for_single_core_pstate(core_id, time_stamp_uS);

        assert_int_equal(core[core_id].pstate[pstate_index].latest_value_mW, test_latest_pwr_mW);

        assert_int_equal(core[core_id].pstate[pstate_index].avg_power_mW, test_new_avg_power_mW);
    }
}

// Unit test function
TEST_FUNCTION(test_comp_metrics_for_single_core_throttling, test_setup, test_teardown)
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

    comp_metrics_for_single_core_throttling(core_id, time_stamp_uS);

    assert_int_equal(core[core_id].throttle_info[throttle_index].residency_mS, 14);
    assert_int_equal(core[core_id].throttle_previous_timestamp_uS[throttle_index], 5000);
    assert_int_equal(core[core_id].throttle_info[throttle_index].max_pstate, 20);

    assert_int_equal(core[core_id].throttle_info[throttle_index].avg_pstate, 12);

    // inactive throttling
    core[core_id].core_throttling_tracker[throttle_index] = 0;
    time_stamp_uS = 10000;
    comp_metrics_for_single_core_throttling(core_id, time_stamp_uS);

    assert_int_not_equal(core[core_id].throttle_info[throttle_index].residency_mS, 19);
    assert_int_equal(core[core_id].throttle_previous_timestamp_uS[throttle_index], 10000);
}

// comp_metrics_for_single_core_throttling_pstate(core_id, i, time_diff_uS, core[core_id].throttle_info[i].residency_mS);
TEST_FUNCTION(test_comp_metrics_for_single_core_throttling_pstate, test_setup, test_teardown)
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
    comp_metrics_for_single_core_throttling_pstate(core_id,
                                                   throttle_index,
                                                   time_diff_uS,
                                                   core[core_id].throttle_info[throttle_index].residency_mS);

    assert_int_equal(core[core_id].throttle_info[throttle_index].max_pstate, 20);
    assert_int_equal(core[core_id].throttle_info[throttle_index].avg_pstate, 14);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_histogram, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    // Call the function to be tested TODO:: update when implemented
    comp_metrics_for_single_core_histogram(core_id);
    // Add assertions to verify the expected behavior
    // Since the function does nothing for now, there is nothing to assert
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