
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

TEST_FUNCTION(test_comp_metrics_for_soc_for_sampling_period, test_setup, test_teardown)
{
    // Initialize soc_info
    soc_info.time_counter_uS = 0;
    // Set up mock return values
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    // Call the function to be tested
    comp_metrics_for_soc_for_sampling_period();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 0);

    // here soc_info.time_counter_uS will be 5 ;
    // Test :updated previous timestamp on next update.

    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 10);
    comp_metrics_for_soc_for_sampling_period();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 5); // 10-5 =5

    // Test :update again.
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 15);
    comp_metrics_for_soc_for_sampling_period();
    //  Add assertions to verify the expected behavior
    assert_int_equal(soc_info.time_counter_uS, 10); // 15-5 =5
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
    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].voltage.min_mV = 0;
    core[core_id].voltage.max_mV = 0;
    core[core_id].voltage.average_mV = 0;
    core[core_id].voltage.latest_value_mV = 1200;
    comp_metrics_for_single_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1200);
    assert_int_equal(core[core_id].voltage.max_mV, 1200);
    assert_int_equal(core[core_id].voltage.average_mV, 1200); // First time no average value to we assign 1200.

    // Test case: Update with new latest value
    core[core_id].voltage.latest_value_mV = 1300;
    comp_metrics_for_single_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1200);
    assert_int_equal(core[core_id].voltage.max_mV, 1300);
    assert_int_equal(core[core_id].voltage.average_mV, 1250); // (1200 + 1300) / 2

    // Test case: Update with lower latest value
    core[core_id].voltage.latest_value_mV = 1100;
    comp_metrics_for_single_core_voltage(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].voltage.min_mV, 1100);
    assert_int_equal(core[core_id].voltage.max_mV, 1300);
    assert_int_equal(core[core_id].voltage.average_mV, 1175); // (1250 + 1100) / 2
}

// Unit test for  comp_metrics_for_single_core_temperature
TEST_FUNCTION(test_comp_metrics_for_single_core_temperature, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Test case: Initial values
    core[core_id].temperature.min_dC = 0;
    core[core_id].temperature.max_dC = 0;
    core[core_id].temperature.average_dC = 0;
    core[core_id].temperature.latest_value_dC = 500;

    comp_metrics_for_single_core_temperature(core_id, time_diff_uS, residency_uS);

    assert_int_equal(core[core_id].temperature.min_dC, 500);
    assert_int_equal(core[core_id].temperature.max_dC, 500);
    assert_int_equal(core[core_id].temperature.average_dC, 500); // first time we keep it 500

    // Test case: Update with new latest value
    core[core_id].temperature.latest_value_dC = 600;
    comp_metrics_for_single_core_temperature(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].temperature.min_dC, 500);
    assert_int_equal(core[core_id].temperature.max_dC, 600);
    assert_int_equal(core[core_id].temperature.average_dC, 550); // (500 + 600) / 2

    // Test case: Update with lower latest value
    core[core_id].temperature.latest_value_dC = 400;
    comp_metrics_for_single_core_temperature(core_id, time_diff_uS, residency_uS);
    assert_int_equal(core[core_id].temperature.min_dC, 400);
    assert_int_equal(core[core_id].temperature.max_dC, 600);
    assert_int_equal(core[core_id].temperature.average_dC, 475); // (550 + 400) / 2
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
    uint32_t time_diff_uS = 100;
    uint32_t residency_uS = 200;

    // Initialize soc_info with some values
    soc_info.rail[vr_index].voltage.min_mV = 1000;
    soc_info.rail[vr_index].voltage.max_mV = 2000;
    soc_info.rail[vr_index].voltage.average_mV = 1500;
    soc_info.rail[vr_index].voltage.latest_value_mV = 1800;

    // Call the function to be tested
    comp_metrics_for_soc_rail_voltage(vr_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.rail[vr_index].voltage.min_mV, 1000);
    assert_int_equal(soc_info.rail[vr_index].voltage.max_mV, 2000);
    assert_int_equal(soc_info.rail[vr_index].voltage.average_mV, 1650);
    assert_int_equal(soc_info.rail[vr_index].voltage.latest_value_mV, 1800);
}

TEST_FUNCTION(test_comp_metrics_for_soc_rail_current, test_setup, test_teardown)
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
    comp_metrics_for_soc_rail_current(vr_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.rail[vr_index].current.min_mA, 500);
    assert_int_equal(soc_info.rail[vr_index].current.max_mA, 1000);
    assert_int_equal(soc_info.rail[vr_index].current.average_mA, 825);
    assert_int_equal(soc_info.rail[vr_index].current.latest_value_mA, 900);
}

TEST_FUNCTION(test_comp_metrics_for_single_hnf_channel, test_setup, test_teardown)
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
    comp_metrics_for_single_hnf_channel(hnf_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.hnf[hnf_index].min_dC, 30);
    assert_int_equal(soc_info.hnf[hnf_index].max_dC, 70);
    assert_int_equal(soc_info.hnf[hnf_index].average_dC, 55);
    assert_int_equal(soc_info.hnf[hnf_index].latest_value_dC, 60);
}

TEST_FUNCTION(test_comp_metrics_for_single_soc_temp_sensor, test_setup, test_teardown)
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
    comp_metrics_for_single_soc_temp_sensor(pvt_index, time_diff_uS, residency_uS);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_info.sensor_temp[pvt_index].min_dC, 25);
    assert_int_equal(soc_info.sensor_temp[pvt_index].max_dC, 75);
    assert_int_equal(soc_info.sensor_temp[pvt_index].average_dC, 57);
    assert_int_equal(soc_info.sensor_temp[pvt_index].latest_value_dC, 65);
}

TEST_FUNCTION(test_comp_metrics_for_single_soc_dimm, test_setup, test_teardown)
{

    sensor_ram_dimm_info_t dimm_info = {
        .timestamp = 2000,
        .dimm_temp_s0_dC = 260,
        .dimm_temp_s1_dC = 280,
        .dimm_power_mW = 100,
        .dimm_id = 0,
        .dimm_throttling = 0,
        .dimm_memory_frequency_id = 0,
    };
    soc_dimm.previous_soc_dimm_timestamp_uS = 1000;
    soc_dimm.residency_uS = 5000;

    uint8_t dimm_module_index = dimm_info.dimm_id;

    // Initialize soc_info with some test values
    soc_dimm.dimm_temp[dimm_module_index].s0.min_dC = 80;
    soc_dimm.dimm_temp[dimm_module_index].s0.max_dC = 90;
    soc_dimm.dimm_temp[dimm_module_index].s0.average_dC = 0;
    soc_dimm.dimm_temp[dimm_module_index].s0.latest_value_dC = 100;

    soc_dimm.dimm_temp[dimm_module_index].s1.min_dC = 90;
    soc_dimm.dimm_temp[dimm_module_index].s1.max_dC = 100;
    soc_dimm.dimm_temp[dimm_module_index].s1.average_dC = 0;
    soc_dimm.dimm_temp[dimm_module_index].s1.latest_value_dC = 20;

    // Baseline log
    data_smpl_parse_dimm_entry(&dimm_info);

    // Call the function to be tested
    comp_metrics_for_single_soc_dimm(&dimm_info);

    // Add assertions to verify the expected behavior
    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s0.latest_value_dC, 260);
    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s1.latest_value_dC, 280);

    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s0.min_dC, 80);
    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s1.min_dC, 90);

    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s0.max_dC, 260);
    assert_int_equal(soc_dimm.dimm_temp[dimm_module_index].s1.max_dC, 280);
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
        tile[tile_id].active_sample_max_temperature_dC = 50;
        tile[tile_id].max_tile_temperature_dC = 40;
        tile[tile_id].active_sample_max_id = 1;
        tile[tile_id].max_tile_id = 0;
    }
    // Set up mock return values for tlm_get_timestamp_microseconds
    will_return(__wrap_exec_tlm_cmpnt_get_timestamp_microseconds, 5);

    comp_metrics_for_tiles_for_sampling_period();

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

    comp_metrics_for_tiles_for_sampling_period();
    temp_active_sample_max_temperature_dC = 60;
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        assert_int_equal(tile[tile_id].time_counter_uS, 5);
        assert_int_equal(tile[tile_id].max_tile_temperature_dC, temp_active_sample_max_temperature_dC);
        temp_active_sample_max_temperature_dC++;
        assert_int_equal(tile[tile_id].max_tile_id, 1);
    }
}