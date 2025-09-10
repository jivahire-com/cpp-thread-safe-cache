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
#include <aging_counters_i.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <data_utilities_i.h>
#include <die_2_die_exchange_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern aging_counter_t core_aging[NUMBER_OF_CORES_PER_DIE];
}

#define TEST_CORE_ID (7)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    comp_metrics_reset_local_2_min_metrics();

    comp_metrics_reset_24_hrs_metrics();

    will_return_always(__wrap_core_info_get_enable_cores_result, 0x00);

    comp_metrics_init();

    in_band_publishing_active = true;

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
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;
    in_band_publishing_active = false;

    uint16_t test_values_mA[5] = {18, 20, 30, 40, 50};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_current(core_id, test_values_mA[i]);
    }

    // not updated because core is inactive
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].current_mA.running_avg), 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.running_avg.num_samples, 0);

    core_is_active[core_id] = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_current(core_id, test_values_mA[i]);
    }

    // not updated because publishing is not active
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].current_mA.running_avg), 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.running_avg.num_samples, 0);

    core_is_active[core_id] = true;
    in_band_publishing_active = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_current(core_id, test_values_mA[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.min, 18);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.max, 50);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].current_mA.running_avg), 32);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].current_mA.running_avg.num_samples, 5);
}

// Unit test for  comp_metrics_for_single_core_voltage
TEST_FUNCTION(test_comp_metrics_for_single_core_voltage, test_setup, test_teardown)
{
    // Feed a few values into the core power computation and validate the results
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;
    uint16_t test_values_mV[5] = {180, 200, 300, 400, 500};
    uint16_t test_vcpu_values_mV[5] = {180, 200, 300, 400, 500};

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_voltage(core_id, test_values_mV[i], test_vcpu_values_mV[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg), 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.num_samples, 0);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.max, 0);
    assert_int_equal(
        data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg),
        0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg.num_samples, 0);

    core_is_active[core_id] = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_voltage(core_id, test_values_mV[i], test_vcpu_values_mV[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.min, 180);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.max, 500);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg), 316);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.num_samples, 5);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.min, 180);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.max, 500);
    assert_int_equal(
        data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg),
        316);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg.num_samples, 5);
}

// Unit test for  comp_metrics_for_single_core_temperature
TEST_FUNCTION(test_comp_metrics_for_single_core_temperature, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    comp_metrics_for_single_core_temperature(core_id, 500);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 0);
    assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg), 0);

    core_is_active[core_id] = true;

    comp_metrics_for_single_core_temperature(core_id, 500);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 500);
    assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg), 500);

    // Test case: Update with new latest value
    comp_metrics_for_single_core_temperature(core_id, 600);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 600);
    assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg), 550);

    // Test case: Update with lower latest value
    comp_metrics_for_single_core_temperature(core_id, 400);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.min, 400);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].temperature_dC.max, 600);
    assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[core_id].temperature_dC.running_avg), 500);
}

TEST_FUNCTION(test_comp_metrics_for_soc_rails, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    computed_metrics_oob.soc_total_pwr_mov_avg_mW.total_sum = 0;
    computed_metrics_oob.soc_total_pwr_mov_avg_mW.sample_count = 0;

    // Arrange
    uint16_t voltage[MAX_NUM_OF_VR_RAILS];
    uint32_t current[MAX_NUM_OF_VR_RAILS];
    uint32_t expected_total_power = 0;

    for (uint16_t i = 0; i < NUM_DIE1_VR_RAILS; i++)
    {
        voltage[i] = 1000 + i * 10; // 1000mV, 1010mV, ...
        current[i] = 100 + i * 5;   // 100mA, 105mA, ...
        expected_total_power += (voltage[i] * current[i]) / 1000;
    }

    // Act
    comp_metrics_for_soc_rails(&voltage, &current);

    // Assert
    assert_int_equal(computed_metrics_oob.soc_total_pwr_mov_avg_mW.total_sum, expected_total_power);
    assert_int_equal(computed_metrics_oob.soc_total_pwr_mov_avg_mW.sample_count, 1);

    // Assert voltage and current values for each VR rail
    for (uint16_t i = 0; i < NUM_DIE1_VR_RAILS; i++)
    {
        assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[i].voltage_mV.running_avg),
                         voltage[i]);
        assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.soc.vr_rail[i].current_mA.running_avg),
                         current[i]);
    }
}

TEST_FUNCTION(test_comp_metrics_for_soc_rails_primary_die_inband_inactive, test_setup, test_teardown)
{
    die_2_die_exch_init(0); // PRIMARY_DIE_ID

    computed_metrics_oob.soc_total_pwr_mov_avg_mW.total_sum = 0;
    computed_metrics_oob.soc_total_pwr_mov_avg_mW.sample_count = 0;

    // Set in_band_publishing_active to false to test the case where voltage/current metrics are not updated
    in_band_publishing_active = false;

    // Arrange
    uint16_t voltage[MAX_NUM_OF_VR_RAILS];
    uint32_t current[MAX_NUM_OF_VR_RAILS];
    uint32_t expected_total_power = 0;

    for (uint16_t i = 0; i < NUM_DIE0_VR_RAILS; i++)
    {
        voltage[i] = 1200 + i * 15; // 1200mV, 1215mV, ...
        current[i] = 150 + i * 10;  // 150mA, 160mA, ...
        expected_total_power += (voltage[i] * current[i]) / 1000;
    }

    // Act
    comp_metrics_for_soc_rails(&voltage, &current);

    // Assert
    // Out-of-band power metrics should still be updated
    assert_int_equal(computed_metrics_oob.soc_total_pwr_mov_avg_mW.total_sum, expected_total_power);
    assert_int_equal(computed_metrics_oob.soc_total_pwr_mov_avg_mW.sample_count, 1);

    // In-band voltage and current metrics should NOT be updated (should remain at initial values)
    for (uint16_t i = 0; i < NUM_DIE0_VR_RAILS; i++)
    {
        assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[i].voltage_mV.running_avg), 0);
        assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_2_mins.soc.vr_rail[i].current_mA.running_avg), 0);
    }
}

TEST_FUNCTION(test_comp_metrics_for_soc_rail_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(0);

    uint8_t vr_index = 4;

    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min = 500;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max = 1000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.summation = 750 * 3;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples = 3;

    soc_rt.latest_rail_temperature_dC[vr_index] = 2000;
    // Call the function to be tested
    comp_metrics_for_soc_rail_temperature(&soc_rt.latest_rail_temperature_dC);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max, 2000);
    assert_int_equal(
        data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg),
        1063);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples, 4);

    will_return_always(__wrap_core_info_get_enable_cores_result, 0x00);

    comp_metrics_init();
    die_2_die_exch_init(1);

    vr_index = 1;

    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min = 500;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max = 1000;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.summation = 750 * 3;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples = 3;

    soc_rt.latest_rail_temperature_dC[vr_index] = 2000;
    // Call the function to be tested
    comp_metrics_for_soc_rail_temperature(&soc_rt.latest_rail_temperature_dC);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min, 500);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.max, 2000);
    assert_int_equal(
        data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg),
        1063);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.running_avg.num_samples, 4);

    assert_int_equal(computed_metrics_oob.max_vr_temp_mov_avg_dC.total_sum, 2000);
    assert_int_equal(computed_metrics_oob.max_vr_temp_mov_avg_dC.sample_count, 1);

    // Test case: in_band_publishing_active = false
    in_band_publishing_active = false;
    vr_index = 5;
    computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min = 600;
    soc_rt.latest_rail_temperature_dC[vr_index] = 1800;
    comp_metrics_for_soc_rail_temperature(&soc_rt.latest_rail_temperature_dC);
    assert_int_equal(computed_metrics_2_mins.soc.vr_rail[vr_index].temperature_dC.min, 600);
}

TEST_FUNCTION(test_comp_metrics_for_single_hnf_channel, test_setup, test_teardown)
{
    uint8_t hnf_channel = 2;

    // Initialize soc_rt with some values
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min = 30;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max = 70;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.summation = 50 * 1;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.num_samples = 1;

    // Call the function to be tested
    comp_metrics_for_single_hnf_channel(hnf_channel, 60);

    // Add assertions to verify the expected behavior
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min, 30);
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].max, 70);
    assert_int_equal(
        data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg),
        55);

    // Test case: in_band_publishing_active = false
    in_band_publishing_active = false;
    hnf_channel = 3;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min = 40;
    comp_metrics_for_single_hnf_channel(hnf_channel, 80);
    assert_int_equal(computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].min, 40);
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
        assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.top_sensor_temp_dC[i].running_avg),
                         test_values[i]);
    }

    // Test case: in_band_publishing_active = false
    in_band_publishing_active = false;
    computed_metrics_2_mins.soc.top_sensor_temp_dC[0].min = 100;
    test_values[0] = 200;
    comp_metrics_for_soc_top_temp_sensor(&test_values);
    assert_int_equal(computed_metrics_2_mins.soc.top_sensor_temp_dC[0].min, 100);
}

TEST_FUNCTION(test_comp_metrics_for_single_soc_dimm, test_setup, test_teardown)
{

    uint8_t dimm_id = 0;
    uint16_t test_values_dC[5][3] = {
        {180, 300, 400},
        {220, 340, 500},
        {200, 320, 420},
        {210, 330, 440},
        {190, 180, 490},
    };

    die_2_die_exch_init(0);

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_soc_dimm(dimm_id, test_values_dC[i][0], test_values_dC[i][1], test_values_dC[i][2]);
    }

    // Check the results

    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.min, 180);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.max, 220);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.running_avg), 200);

    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s1_dC.min, 180);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s1_dC.max, 340);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[0].temperature_s1_dC.running_avg), 294);

    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].power_mW.min, 400);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].power_mW.max, 500);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[0].power_mW.running_avg), 450);

    die_2_die_exch_init(1);
    comp_metrics_for_single_soc_dimm(dimm_id, 20, 10, 30);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s0_dC.min, 20);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].temperature_s1_dC.min, 10);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[0].power_mW.min, 30);

    // Test case: in_band_publishing_active = false
    in_band_publishing_active = false;
    dimm_id = 1;
    computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s0_dC.min = 150;
    comp_metrics_for_single_soc_dimm(dimm_id, 250, 300, 350);
    assert_int_equal(computed_metrics_2_mins.soc.dimm[dimm_id].temperature_s0_dC.min, 150);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_power_per_pstate, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    // Feed a few values into the core power computation and validate the results
    uint16_t test_values_mW[5] = {100, 200, 300, 400, 500};

    int pstate_index = 0;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power_per_pstate(core_id, pstate_index, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.min, 0);
    assert_int_equal(data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.running_avg),
                     0);

    pstate_index = 0;
    core_is_active[core_id] = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power_per_pstate(core_id, pstate_index, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.min, 100);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.max, 500);
    assert_int_equal(data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.cores[core_id].pstate[pstate_index].power_mW.running_avg),
                     300);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_throttling_pstate, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t throttle_source = 1;
    uint8_t test_pstates[5] = {10, 15, 20, 25, 30};

    // Test case: Core is inactive - should not update metrics
    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_throttling_pstate(core_id, throttle_source, test_pstates[i]);
    }

    // Check the results - should be 0 since core is inactive
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg),
                     0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg.num_samples, 0);

    // Test case: Core is active - should update metrics
    core_is_active[core_id] = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_throttling_pstate(core_id, throttle_source, test_pstates[i]);
    }

    // Check the results - should track min, max, and average
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.min, 10);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.max, 30);
    assert_int_equal(data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg),
                     20);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].pstate.running_avg.num_samples, 5);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_power, test_setup, test_teardown)
{
    // Feed a few values into the core power computation and validate the results
    uint8_t core_id = TEST_CORE_ID;

    uint16_t test_values_mW[5] = {100, 200, 300, 400, 500};
    core_is_active[core_id] = false;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power(core_id, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.min, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.max, 0);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].power_mW.running_avg), 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.running_avg.num_samples, 0);

    core_is_active[core_id] = true;

    for (uint8_t i = 0; i < 5; i++)
    {
        comp_metrics_for_single_core_power(core_id, test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.min, 100);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.max, 500);
    assert_int_equal(data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].power_mW.running_avg), 300);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].power_mW.running_avg.num_samples, 5);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_throttle_overrun, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t throttle_source = THROTTLE_SOURCE_CURRENT;

    comp_metrics_for_single_core_throttle_overrun(core_id, throttle_source);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count, 0);

    core_is_active[core_id] = true;

    comp_metrics_for_single_core_throttle_overrun(core_id, throttle_source);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count, 1);
    throttle_source = THROTTLE_SOURCE_ADAPTIVE_CLK;
    comp_metrics_for_single_core_throttle_overrun(core_id, throttle_source);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].overrun_count, 1);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_throttle_source, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t throttle_source = THROTTLE_SOURCE_CURRENT;
    uint64_t timestamp_diff_uS = 1000;
    bool throttle_start = true;

    comp_metrics_for_single_core_single_throttle_source(core_id, throttle_source, timestamp_diff_uS, throttle_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS, 0);

    core_is_active[core_id] = true;
    comp_metrics_for_single_core_single_throttle_source(core_id, throttle_source, timestamp_diff_uS, throttle_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS, 0);

    throttle_start = false;
    comp_metrics_for_single_core_single_throttle_source(core_id, throttle_source, timestamp_diff_uS, throttle_start);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].throttle_info[throttle_source].residency_uS, 1000);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_rack_priority, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t current_priority = 0;
    uint8_t new_priority = 6;
    uint64_t timestamp_diff_uS = 1000;
    uint64_t timestamp_diff2_uS = 500;
    bool rack_throttle_priority_start = true;

    // Test case: Core is inactive - should not update metrics
    comp_metrics_for_single_core_single_rack_priority(core_id, current_priority, timestamp_diff_uS, new_priority, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[new_priority].entry_count, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[current_priority].residency_uS, 0);

    // Test case: Core is active, throttle priority start
    core_is_active[core_id] = true;
    comp_metrics_for_single_core_single_rack_priority(core_id, current_priority, timestamp_diff_uS, new_priority, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[new_priority].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[current_priority].residency_uS, 0);

    // Test case: Core is active, throttle priority end (rack_throttle_priority_start = false)
    rack_throttle_priority_start = false;
    comp_metrics_for_single_core_single_rack_priority(core_id, current_priority, timestamp_diff_uS, new_priority, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[new_priority].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[current_priority].residency_uS, 1000);

    // Test case: Multiple residency updates
    comp_metrics_for_single_core_single_rack_priority(core_id, current_priority, timestamp_diff2_uS, new_priority, rack_throttle_priority_start);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[new_priority].entry_count, 1);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].rack_priorities[current_priority].residency_uS, 1500);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_pstate, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t current_pstate = 3;
    uint8_t new_pstate = 6;
    uint64_t timestamp_diff_uS = 1000;
    bool update_pstate_entry = true;
    comp_metrics_for_single_core_single_pstate(core_id, current_pstate, timestamp_diff_uS, new_pstate, update_pstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[current_pstate].residency_uS, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[new_pstate].entry_count, 0);

    core_is_active[core_id] = true;
    comp_metrics_for_single_core_single_pstate(core_id, current_pstate, timestamp_diff_uS, new_pstate, update_pstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[current_pstate].residency_uS, 1000);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].pstate[new_pstate].entry_count, 1);
}

TEST_FUNCTION(test_comp_metrics_for_soc_avg_pstate, test_setup, test_teardown)
{
    die_2_die_exch_init(0);

    // Setup: Mark some cores as active
    memset(core_is_active, 0, sizeof(core_is_active));
    core_is_active[0] = true;
    core_is_active[1] = true;
    core_is_active[2] = false;
    core_is_active[3] = true;

    // Setup: Provide pstate values for all cores
    uint8_t pstate[NUMBER_OF_CORES_PER_DIE] = {0};
    pstate[0] = 10; // Core 0
    pstate[1] = 20; // Core 1
    pstate[2] = 16; // Core 2 (inactive)
    pstate[3] = 30; // Core 3

    // Call function under test
    comp_metrics_for_soc_avg_pstate(&pstate);

    // Only active cores: 0, 1, 3 (values: 10, 20, 30)
    // total_pstate = 10 + 20 + 30 = 60
    // num_active_cores = 3
    // avg_pstate = (60 + 3/2) / 3 = (60 + 1) / 3 = 61 / 3 = 20 (integer division)

    assert_int_equal(computed_metrics_oob.pstate_mov_avg.total_sum, 20);
    assert_int_equal(computed_metrics_oob.pstate_mov_avg.sample_count, 1);

    die_2_die_exch_init(1);
    data_util_mov_avg_u16_clear(&computed_metrics_oob.pstate_mov_avg);
    comp_metrics_for_soc_avg_pstate(&pstate);

    assert_int_equal(computed_metrics_oob.pstate_mov_avg.total_sum, 20);
    assert_int_equal(computed_metrics_oob.pstate_mov_avg.sample_count, 1);

    // Test: No active cores
    data_util_mov_avg_u16_clear(&computed_metrics_oob.pstate_mov_avg);
    memset(core_is_active, 0, sizeof(core_is_active));
    memset(pstate, 0, sizeof(pstate));
    comp_metrics_for_soc_avg_pstate(&pstate);
    assert_int_equal(computed_metrics_oob.pstate_mov_avg.sample_count, 0);
}

TEST_FUNCTION(test_comp_metrics_for_single_core_single_cstate, test_setup, test_teardown)
{
    uint8_t core_id = TEST_CORE_ID;
    core_is_active[core_id] = false;

    uint8_t current_cstate = 7;
    uint8_t new_cstate = 11;
    uint64_t timestamp_diff_uS = 1000;
    uint8_t update_cstate_entry = 1;
    comp_metrics_for_single_core_single_cstate(core_id, current_cstate, timestamp_diff_uS, new_cstate, update_cstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[current_cstate].residency_uS, 0);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[new_cstate].entry_count, 0);

    core_is_active[core_id] = true;
    comp_metrics_for_single_core_single_cstate(core_id, current_cstate, timestamp_diff_uS, new_cstate, update_cstate_entry);

    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[current_cstate].residency_uS, 1000);
    assert_int_equal(computed_metrics_2_mins.cores[core_id].cstate[new_cstate].entry_count, 1);
}

TEST_FUNCTION(test_comp_metrics_reset_local_2_min_metrics, test_setup, test_teardown)
{
    memset(&computed_metrics_2_mins, 0xFF, sizeof(computed_metrics_2_mins));
    comp_metrics_reset_local_2_min_metrics();
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

TEST_FUNCTION(test_comp_metrics_for_max_dimm_temp, test_setup, test_teardown)
{
    uint16_t test_values_dC[DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES] = {200, 220};
    die_2_die_exch_init(0);

    for (uint8_t i = 0; i < DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES; i++)
    {
        comp_metrics_for_max_dimm_temp(test_values_dC[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_oob.max_dimm_temp_mov_avg_dC.total_sum, 420);
    assert_int_equal(computed_metrics_oob.max_dimm_temp_mov_avg_dC.sample_count, 2);

    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES; i++)
    {
        comp_metrics_for_max_dimm_temp(test_values_dC[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_oob.max_dimm_temp_mov_avg_dC.total_sum, 420);
    assert_int_equal(computed_metrics_oob.max_dimm_temp_mov_avg_dC.sample_count, 2);
}

TEST_FUNCTION(test_comp_metrics_for_total_dimm_pwr, test_setup, test_teardown)
{
    die_2_die_exch_init(0);

    uint16_t test_values_mW[DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES] = {190, 210};

    for (uint8_t i = 0; i < DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES; i++)
    {
        comp_metrics_for_total_dimm_pwr(test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.total_sum, 400);
    assert_int_equal(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.sample_count, 2);

    die_2_die_exch_init(1);

    for (uint8_t i = 0; i < DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES; i++)
    {
        comp_metrics_for_total_dimm_pwr(test_values_mW[i]);
    }

    // Check the results
    assert_int_equal(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.total_sum, 400);
    assert_int_equal(computed_metrics_oob.dimm_total_pwr_mov_avg_mW.sample_count, 2);
}

TEST_FUNCTION(test_comp_metrics_init_active_cores, test_setup, test_teardown)
{

    will_return(__wrap_core_info_get_enable_cores_result, 0x01);
    will_return(__wrap_core_info_get_enable_cores_result, 0x02);
    will_return(__wrap_core_info_get_enable_cores_result, 0x04);

    comp_metrics_init_active_cores(); // calls comp_metrics_init_active_cores after clearing the data

    // mock function sets cores 0, 33, 66 as active
    assert_true(core_is_active[0]);
    assert_true(core_is_active[33]);
    assert_true(core_is_active[66]);
}

TEST_FUNCTION(test_comp_metrics_for_cores_droop_counts, test_setup, test_teardown)
{
    static uint64_t expected_droop_counts[NUMBER_OF_CORES_PER_DIE];
    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        expected_droop_counts[i] = i * 10;
        core_is_active[i] = (i % 2 == 0);
    }

    will_return(__wrap_pwr_tlm_core_exch_mcp_read_droop_counts, expected_droop_counts);

    comp_metrics_for_cores_droop_counts();

    for (uint8_t i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
    {
        if (core_is_active[i])
        {
            assert_int_equal(computed_metrics_2_mins.cores[i].droop_count, expected_droop_counts[i]);
        }
    }
}

TEST_FUNCTION(test_comp_metrics_for_single_core_aging_counters, test_setup, test_teardown)
{
    uint8_t core_id = 0;
    uint64_t this_pwr_pkg_timestamp_uS = 20000;
    uint16_t latest_voltage_mV = 100;
    uint16_t latest_max_value_dC = 19;

    uint32_t latest_aged_counter = 30;
    uint32_t latest_unaged_counter = 28;

    core_is_active[core_id] = 1;
    core_aging[core_id].measurement_armed = true;
    memset(&computed_metrics_24_hrs, 0, sizeof(computed_metrics_24_hrs));

    for (uint8_t counter_id = 0; counter_id < NUMBER_OF_AGING_COUNTER_PAIRS; counter_id++)
    {

        comp_metrics_for_single_core_aging_counters(core_id,
                                                    latest_voltage_mV,
                                                    latest_max_value_dC,
                                                    this_pwr_pkg_timestamp_uS,
                                                    latest_aged_counter,
                                                    latest_unaged_counter,
                                                    counter_id);

        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS, 20000);
        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter, 30);
        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter, 28);
        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].counter_id,
                         counter_id); // this will increment on each iteration
        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].voltage_mV, 100);
        assert_int_equal(computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].temperature_dC, 19);
    }
}
TEST_FUNCTION(test_comp_metrics_for_mpam_power, test_setup, test_teardown)
{
    // Arrange: Create test MPAM power array with known values
    uint32_t test_mpam_power[NUMBER_OF_MPAMS];

    // Initialize with test values - use different patterns for verification
    for (uint8_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_mpam_power[i] = (i + 1) * 100; // 100, 200, 300, ..., 12800 mW
    }

    // Act: Call the function under test
    comp_metrics_for_mpam_power(&test_mpam_power);

    // Assert: Verify that all MPAM power values are updated correctly
    for (uint8_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        // Check that min, max are set to the input value (first sample)
        assert_int_equal(computed_metrics_d2d_2mins.mpam[i].core_power.min, test_mpam_power[i]);
        assert_int_equal(computed_metrics_d2d_2mins.mpam[i].core_power.max, test_mpam_power[i]);

        // Check that average is set to the input value (first sample)
        assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_d2d_2mins.mpam[i].core_power.running_avg),
                         test_mpam_power[i]);
        assert_int_equal(computed_metrics_d2d_2mins.mpam[i].core_power.running_avg.num_samples, 1);
    }
}

TEST_FUNCTION(test_comp_metrics_for_mpam_power_null_pointer, test_setup, test_teardown)
{
    // Arrange: Initialize some values in the metrics
    computed_metrics_d2d_2mins.mpam[0].core_power.min = 100;
    computed_metrics_d2d_2mins.mpam[0].core_power.max = 200;
    computed_metrics_d2d_2mins.mpam[0].core_power.running_avg.summation = 150 * 1;
    computed_metrics_d2d_2mins.mpam[0].core_power.running_avg.num_samples = 1;

    // Act: Call function with null pointer (should not crash)
    comp_metrics_for_mpam_power(NULL);

    // Assert: Verify that existing values are unchanged
    assert_int_equal(computed_metrics_d2d_2mins.mpam[0].core_power.min, 100);
    assert_int_equal(computed_metrics_d2d_2mins.mpam[0].core_power.max, 200);
    assert_int_equal(data_util_running_avg_u32_get(&computed_metrics_d2d_2mins.mpam[0].core_power.running_avg), 150);
    assert_int_equal(computed_metrics_d2d_2mins.mpam[0].core_power.running_avg.num_samples, 1);
}
