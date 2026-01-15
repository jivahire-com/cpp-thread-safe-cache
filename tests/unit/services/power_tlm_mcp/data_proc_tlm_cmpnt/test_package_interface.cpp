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
#include "data_proc_mock.h"

#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <die_2_die_exchange_i.h>
#include <dvfs.h>
#include <in_band_package_interface_i.h>
#include <mcp_telemetry_shared.h> //for cstate_instr_timestamp_t
#include <sensor_fifo_service.h>  // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>               // for uint32_t, uint64_t, int32_t
#include <string.h>               // for memcmp
}

/*-- Symbolic Constant Macros (defines) --*/
extern "C" {
extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;
extern dimm_runtime_info_t dimm_rt;
}

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_SNSR_ID_0      (0)
#define TEST_HNF_CHANN_ID_1 (1)
#define TEST_RAIL_ID_2      (2)
#define TEST_DIMM_MOD_ID_3  (3)
#define TEST_MPAM_ID_4      (4)
#define TEST_CORE_ID_5      (5)

#define AVG_DURATION_USEC (100000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    comp_metrics_reset_local_2_min_metrics();
    comp_metrics_reset_24_hrs_metrics();
    setup_cstate_tfa_mock_buffer();
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
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.summation = 4000 * 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.num_samples = 1;
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

        assert_int_equal(pstate_array[pstate_index].avg_power_mW,
                         data_util_running_avg_u16_get(
                             &computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_index].power_mW.running_avg));

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
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.running_avg.summation = 5000 * 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.running_avg.num_samples = 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.min = 55;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.max = 155;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].entry_count = 2;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].residency_uS = 2000;

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(NUMBER_OF_CORES_PER_DIE, &pstate_array);

    // verify for pstate 0
    assert_int_not_equal(pstate_array[index].avg_power_mW,
                         data_util_running_avg_u16_get(
                             &computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[index].power_mW.running_avg));
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
    pwr_core_element_throttle_t throttle_array[NUMBER_OF_THROTTLE_SOURCES] = {{0}};
    uint8_t throttle_source = 0;
    uint8_t avg_pstate = 5;
    uint32_t entry_count = 10;
    uint32_t residency_uS = 5000;
    uint8_t max_pstate = 5;

    for (throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].pstate.running_avg.summation =
            avg_pstate * 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].pstate.running_avg.num_samples = 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].entry_count = entry_count;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].pstate.max = max_pstate;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].residency_uS = residency_uS;
    }
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(TEST_CORE_ID_5, &throttle_array);

    for (throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
    {
        assert_int_equal(throttle_array[throttle_source].avg_pstate, avg_pstate);
        assert_int_equal(throttle_array[throttle_source].entry_count, entry_count);
        assert_int_equal(throttle_array[throttle_source].max_pstate, max_pstate);
        assert_int_equal(throttle_array[throttle_source].residency_mS, residency_uS / 1000);
        assert_int_equal(throttle_array[throttle_source].type_id, throttle_source);
    }

    // setup for failure case.
    uint8_t index = 0; // test for one of the throttle type.
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].pstate.running_avg.summation = 6 * 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].pstate.running_avg.num_samples = 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].entry_count = 12;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].pstate.max = 28;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[index].residency_uS = 20000;
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(NUMBER_OF_CORES_PER_DIE, &throttle_array);

    throttle_source = 0;

    assert_int_not_equal(
        throttle_array[throttle_source].avg_pstate,
        data_util_running_avg_u16_get(
            &computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].pstate.running_avg));

    assert_int_not_equal(throttle_array[throttle_source].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].entry_count);

    assert_int_not_equal(throttle_array[throttle_source].max_pstate,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].pstate.max);

    assert_int_not_equal(throttle_array[throttle_source].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].throttle_info[throttle_source].residency_uS / 1000);
}

TEST_FUNCTION(test_get_pwr_core_rack_priority_data, test_setup, test_teardown)
{
    pwr_core_element_rack_priorities_t rack_priority_array[NUMBER_OF_RACK_THROTTLE_PRIORITIES] = {{0}};
    uint8_t rack_throttle_priority = 0;
    // test setup
    // Throttling Priorities are only for the Rack Throttling type and Rack Priorities are
    // only populated when the throttling type is the Rack Throttling.
    for (rack_throttle_priority = 0; rack_throttle_priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; rack_throttle_priority++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].entry_count = 1;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].residency_uS = 7000;
    }

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(TEST_CORE_ID_5, &rack_priority_array);

    for (rack_throttle_priority = 0; rack_throttle_priority < NUMBER_OF_RACK_THROTTLE_PRIORITIES; rack_throttle_priority++)
    {
        assert_int_equal(rack_priority_array[rack_throttle_priority].priority_id, rack_throttle_priority);

        assert_int_equal(rack_priority_array[rack_throttle_priority].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].entry_count);

        assert_int_equal(rack_priority_array[rack_throttle_priority].residency_mS,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].residency_uS / 1000);
    }
    rack_throttle_priority = 0;
    // test for failure case .
    // change data in core data structure for rack priority 0 only.
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].entry_count = 2;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].residency_uS = 18000;

    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(NUMBER_OF_CORES_PER_DIE, &rack_priority_array);

    assert_int_equal(rack_priority_array[rack_throttle_priority].priority_id, rack_throttle_priority);

    assert_int_not_equal(rack_priority_array[rack_throttle_priority].entry_count,
                         computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].entry_count);

    assert_int_not_equal(
        rack_priority_array[rack_throttle_priority].residency_mS,
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].rack_priorities[rack_throttle_priority].residency_uS / 1000);
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
{
    pwr_core_element_voltage_t voltage_data = {0};

    uint8_t core_id = 0;
    computed_metrics_2_mins.cores[core_id].voltage_mV.min = 1200;
    computed_metrics_2_mins.cores[core_id].voltage_mV.max = 1800;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.summation = 1500 * 1;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.num_samples = 1;

    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(core_id, &voltage_data);

    assert_int_equal(voltage_data.min_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.min);
    assert_int_equal(voltage_data.max_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.max);
    assert_int_equal(voltage_data.average_mV,
                     data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg));

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

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.summation = 30 * 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.num_samples = 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max = 40;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min = 20;

    data_proc_tlm_cmpnt_get_pwr_core_current_data(index, &current_get_data);

    assert_int_equal(current_get_data.min_mA, computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min);
    assert_int_equal(current_get_data.max_mA, computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max);
    assert_int_equal(current_get_data.average_mA,
                     data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg));

    // setup for fail case .
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.summation = 0 * 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.running_avg.num_samples = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.max = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].current_mA.min = 0;

    data_proc_tlm_cmpnt_get_pwr_core_current_data(NUMBER_OF_CORES_PER_DIE, &current_get_data);
    assert_int_not_equal(current_get_data.average_mA, 0);
}

TEST_FUNCTION(test_get_pwr_core_temperature_data, test_setup, test_teardown)
{
    pwr_core_element_temperature_t temp_data = {0};

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.summation = 30 * 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.num_samples = 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max = 40;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min = 20;

    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, &temp_data);

    assert_int_equal(temp_data.min_dC, computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min);
    assert_int_equal(temp_data.max_dC, computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max);
    assert_int_equal(
        temp_data.average_dC,
        data_util_running_avg_u32_get(&computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg));

    // setup for fail case .

    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.summation = 0 * 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.running_avg.num_samples = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.max = 0;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].temperature_dC.min = 0;

    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(NUMBER_OF_CORES_PER_DIE, &temp_data);
    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, nullptr);
}

TEST_FUNCTION(test_get_pwr_core_histogram_data, test_setup, test_teardown)
{
    pwr_core_element_histogram_t histogram_data[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES] = {{{0}}};

    // Set up test values in computed_metrics_24_hrs for the test core
    for (uint8_t v = 0; v < NUMBER_OF_HS_VOLTAGE_SCALES; v++)
    {
        for (uint8_t t = 0; t < NUMBER_OF_HS_TEMP_SCALES; t++)
        {
            computed_metrics_24_hrs.cores[TEST_CORE_ID_5].histogram.bin_count[v][t] = (v * 100) + t;
        }
    }

    // Valid call - should succeed
    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, &histogram_data);

    // Verify the histogram data was copied correctly
    for (uint8_t v = 0; v < NUMBER_OF_HS_VOLTAGE_SCALES; v++)
    {
        for (uint8_t t = 0; t < NUMBER_OF_HS_TEMP_SCALES; t++)
        {
            assert_int_equal(histogram_data[v][t].voltage_band, v);
            assert_int_equal(histogram_data[v][t].temperature_band, t);
            assert_int_equal(histogram_data[v][t].bin_count, (v * 100) + t);
        }
    }

    // Test error path: invalid core_id
    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(NUMBER_OF_CORES_PER_DIE, &histogram_data);

    // Test error path: NULL pointer
    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, nullptr);
}

TEST_FUNCTION(test_get_pwr_soc_pkg_mon_data, test_setup, test_teardown)
{
    pwr_soc_element_pkg_monitor_t pkg_mon_data = {0};

    // Test case 1: Valid data with PC3 and PC4 residency values
    computed_metrics_24_hrs.soc.pc3_residency_mS = 500000; // 500 seconds in PC3
    computed_metrics_24_hrs.soc.pc4_residency_mS = 300000; // 300 seconds in PC4

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    // Verify the data was copied correctly from computed_metrics_24_hrs
    assert_int_equal(pkg_mon_data.pc3_duration_mS, 500000);
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 300000);

    // Test case 2: Zero values
    computed_metrics_24_hrs.soc.pc3_residency_mS = 0;
    computed_metrics_24_hrs.soc.pc4_residency_mS = 0;

    memset(&pkg_mon_data, 0xFF, sizeof(pkg_mon_data)); // Pre-fill to verify clearing

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    // Verify zero values are handled correctly
    assert_int_equal(pkg_mon_data.pc3_duration_mS, 0);
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 0);

    // Test case 3: Maximum uint32_t values
    computed_metrics_24_hrs.soc.pc3_residency_mS = 0xFFFFFFFF; // uint32_t max
    computed_metrics_24_hrs.soc.pc4_residency_mS = 0xFFFFFFFF;

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    // Verify that maximum uint32_t values are handled correctly
    assert_int_equal(pkg_mon_data.pc3_duration_mS, 0xFFFFFFFF);
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 0xFFFFFFFF);

    // Test case 4: Typical values - 24 hour monitoring period
    // Assuming continuous PC3 for 12 hours and PC4 for 12 hours
    computed_metrics_24_hrs.soc.pc3_residency_mS = 12U * 60 * 60 * 1000; // 12 hours in ms
    computed_metrics_24_hrs.soc.pc4_residency_mS = 12U * 60 * 60 * 1000; // 12 hours in ms

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    assert_int_equal(pkg_mon_data.pc3_duration_mS, 43200000); // 12 hours = 43200000 ms
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 43200000);

    // Test case 5: NULL pointer handling - should log error and not crash
    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(NULL);
    // Function should handle NULL pointer gracefully (logs error via FPFW_ET_LOG)

    // Test case 6: Asymmetric PC3/PC4 residency values
    computed_metrics_24_hrs.soc.pc3_residency_mS = 1000000; // 1000 seconds
    computed_metrics_24_hrs.soc.pc4_residency_mS = 50000;   // 50 seconds

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    assert_int_equal(pkg_mon_data.pc3_duration_mS, 1000000);
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 50000);

    // Test case 7: Verify this is aggregated data from both dies (reported from die 0 only)
    // The data in computed_metrics_24_hrs.soc should already be aggregated from both dies
    // via data_smpl_update_soc_package_cstate() which accumulates from both die 0 and die 1
    computed_metrics_24_hrs.soc.pc3_residency_mS = 86400000; // 24 hours in ms
    computed_metrics_24_hrs.soc.pc4_residency_mS = 0;

    data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(&pkg_mon_data);

    assert_int_equal(pkg_mon_data.pc3_duration_mS, 86400000); // Full 24 hours in PC3
    assert_int_equal(pkg_mon_data.pc4_duration_mS, 0);
}

TEST_FUNCTION(test_get_pwr_soc_vr_rail_data, test_setup, test_teardown)
{
    pwr_soc_element_vr_rail_t rail_data = {{0}};
    uint16_t rail_id = TEST_RAIL_ID_2;

    // Set up test values in computed_metrics_2_mins for this rail
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.running_avg.summation = 123 * 1;
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.running_avg.num_samples = 1;
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.max = 150;
    computed_metrics_2_mins.soc.vr_rail[rail_id].current_mA.min = 100;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.running_avg.summation = 1100 * 1;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.running_avg.num_samples = 1;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.max = 1200;
    computed_metrics_2_mins.soc.vr_rail[rail_id].voltage_mV.min = 1000;
    computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.running_avg.summation = 55 * 1;
    computed_metrics_2_mins.soc.vr_rail[rail_id].temperature_dC.running_avg.num_samples = 1;
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

    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.summation = 100 * 1;
    computed_metrics_2_mins.soc.hnf_temperature_dC[hnf_channel].running_avg.num_samples = 1;
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
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.running_avg.summation = 250 * 1;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.running_avg.num_samples = 1;

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.running_avg.summation = 250 * 1;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.running_avg.num_samples = 1;

    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(TEST_DIMM_MOD_ID_3, &dimm_data);

    assert_int_equal(dimm_data.s0.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.max);
    assert_int_equal(dimm_data.s0.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min);
    assert_int_equal(dimm_data.s0.average_dC,
                     data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.running_avg));

    assert_int_equal(dimm_data.s1.max_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.max);
    assert_int_equal(dimm_data.s1.min_dC,
                     computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.min);
    assert_int_equal(dimm_data.s1.average_dC,
                     data_util_running_avg_u16_get(
                         &computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s1_dC.running_avg));

    // Invalid case
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min = 400;
    data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(NUMBER_OF_DIMMS_PER_DIE, &dimm_data);
    assert_int_not_equal(dimm_data.s0.min_dC,
                         computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].temperature_s0_dC.min);
}

TEST_FUNCTION(test_get_pwr_soc_dimm_power_data, test_setup, test_teardown)
{

    pwr_soc_element_dimm_power_t dimm_data = {{0}};
    // Check DIMM information

    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min = 200;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.max = 300;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.running_avg.summation = 250 * 1;
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.running_avg.num_samples = 1;

    // Check DIMM information
    data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(TEST_DIMM_MOD_ID_3, &dimm_data);

    assert_int_equal(
        dimm_data.power_mW.average_mW,
        data_util_running_avg_u16_get(&computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.running_avg));

    assert_int_equal(dimm_data.power_mW.min_mW, computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min);

    assert_int_equal(dimm_data.power_mW.max_mW, computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.max);

    // Invalid case
    computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min = 300;
    data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(NUMBER_OF_DIMMS_PER_DIE, &dimm_data);
    assert_int_not_equal(dimm_data.power_mW.min_mW,
                         computed_metrics_2_mins.soc.dimm[TEST_DIMM_MOD_ID_3].power_mW.min);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_sensor_temp_t sensor_temp_data = {0};
    uint16_t sensor_id = 1;

    // Set up test values in computed_metrics_2_mins for this sensor
    computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].running_avg.summation = 77 * 1;
    computed_metrics_2_mins.soc.top_sensor_temp_dC[sensor_id].running_avg.num_samples = 1;
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

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data, test_setup, test_teardown)
{
    // Test with valid data - populate computed metrics first with counter values
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_entry_count = 10;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_entry_count = 5;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m0_residency_count = 3000000000ULL; // 3 billion counts = 1500 ms
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_residency_count = 1500000000ULL; // 1.5 billion counts = 750 ms
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_residency_count = 500000000ULL; // 500 million counts = 250 ms
    computed_metrics_2_mins.mesh.die_mesh_pwr.delivered_perf_count = 4000000000ULL; // 4 billion counts = 2000 ms

    pwr_soc_element_die_mesh_t die_mesh_data = {0};

    // Call the function under test
    data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data(&die_mesh_data);

    // Verify the data was copied correctly from computed metrics (counts converted to ms)
    assert_int_equal(die_mesh_data.m1_entry_count, 10);
    assert_int_equal(die_mesh_data.m2_entry_count, 5);
    assert_int_equal(die_mesh_data.m0_residency_mS, 1500);
    assert_int_equal(die_mesh_data.m1_residency_mS, 750);
    assert_int_equal(die_mesh_data.m2_residency_mS, 250);
    assert_int_equal(die_mesh_data.delivery_perf_mS, 2000);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data_null_pointer, test_setup, test_teardown)
{
    // Test with NULL pointer - should handle gracefully and log error
    data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data(NULL);
    // Function should handle NULL pointer gracefully without crashing
    // (Real implementation logs an error via FPFW_ET_LOG)
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data_zero_values, test_setup, test_teardown)
{
    // Test with all zero values in computed metrics
    memset(&computed_metrics_2_mins.mesh.die_mesh_pwr, 0, sizeof(computed_metrics_2_mins.mesh.die_mesh_pwr));

    pwr_soc_element_die_mesh_t die_mesh_data = {0xFF}; // Initialize to non-zero

    // Call the function under test
    data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data(&die_mesh_data);

    // Verify all values are zero
    assert_int_equal(die_mesh_data.m1_entry_count, 0);
    assert_int_equal(die_mesh_data.m2_entry_count, 0);
    assert_int_equal(die_mesh_data.m0_residency_mS, 0);
    assert_int_equal(die_mesh_data.m1_residency_mS, 0);
    assert_int_equal(die_mesh_data.m2_residency_mS, 0);
    assert_int_equal(die_mesh_data.delivery_perf_mS, 0);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data_max_values, test_setup, test_teardown)
{
    // Test with maximum values
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_entry_count = UINT64_MAX;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_entry_count = UINT64_MAX;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m0_residency_count = UINT64_MAX;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m1_residency_count = UINT64_MAX;
    computed_metrics_2_mins.mesh.die_mesh_pwr.m2_residency_count = UINT64_MAX;
    computed_metrics_2_mins.mesh.die_mesh_pwr.delivered_perf_count = UINT64_MAX;

    pwr_soc_element_die_mesh_t die_mesh_data = {0};

    // Call the function under test
    data_proc_tlm_cmpnt_get_pwr_soc_die_mesh_data(&die_mesh_data);

    // Verify maximum values are handled correctly
    // Entry counts remain as-is
    assert_int_equal(die_mesh_data.m1_entry_count, UINT64_MAX);
    assert_int_equal(die_mesh_data.m2_entry_count, UINT64_MAX);
    // Residency values are converted from counts to milliseconds
    uint64_t expected_max_ms = (uint64_t)MESH_COUNTER_TO_MS(UINT64_MAX);
    assert_int_equal(die_mesh_data.m0_residency_mS, expected_max_ms);
    assert_int_equal(die_mesh_data.m1_residency_mS, expected_max_ms);
    assert_int_equal(die_mesh_data.m2_residency_mS, expected_max_ms);
    assert_int_equal(die_mesh_data.delivery_perf_mS, expected_max_ms);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data, test_setup, test_teardown)
{
    // Test case 1: Valid interface_id with populated data
    pwr_soc_element_d2d_link_t d2d_link_data[NUMBER_OF_D2D_LINKS_STATE] = {{0}};
    uint8_t valid_interface_id = 0;

    // Set up test data in computed_metrics_2_mins for the d2d link interface
    // Based on actual d2d link telemetry structure, we have:
    // - tx_residency_count and rx_residency_count (converted to mS in output)
    // -  bw_tx_flit_count  and bw_rx_flit_count (bandwidth counters)
    // - link_id (link identifier)

    // Set up test data for d2d link states (states 0, 1, and 2)
    // Using DIE_MESH_FREQ_HZ = 2GHz, to get desired ms values: ms * 2,000,000,000 / 1000
    // Bandwidth values are flit counts that will be converted to bytes using D2DSS_FLIT_COUNT_TO_BYTES (count * 64)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[0].tx_residency_count =
        3000000000ULL; // TX residency count (converts to 1500 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[0].rx_residency_count =
        2800000000ULL; // RX residency count (converts to 1400 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[0].bw_tx_flit_count =
        2500; // TX flit count (converts to 2500*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[0].bw_rx_flit_count =
        2400; // RX flit count (converts to 2400*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[0].link_id = 0; // Link ID

    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[1].tx_residency_count =
        600000000ULL; // TX residency count (converts to 300 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[1].rx_residency_count =
        500000000ULL; // RX residency count (converts to 250 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[1].bw_tx_flit_count =
        0; // TX flit count (converts to 0*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[1].bw_rx_flit_count =
        0; // RX flit count (converts to 0*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[1].link_id = 1; // Link ID

    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[2].tx_residency_count =
        1000000000ULL; // TX residency count (converts to 500 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[2].rx_residency_count =
        800000000ULL; // RX residency count (converts to 400 mS)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[2].bw_tx_flit_count =
        1200; // TX flit count (converts to 1200*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[2].bw_rx_flit_count =
        1100; // RX flit count (converts to 1100*64 bytes)
    computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link[2].link_id = 2; // Link ID

    // Call the function under test
    data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(valid_interface_id, &d2d_link_data);

    // Now with the bug fixed, each array position gets populated correctly
    assert_int_equal(d2d_link_data[0].tx_residency_mS, 1500);       // State 0 TX residency
    assert_int_equal(d2d_link_data[0].rx_residency_mS, 1400);       // State 0 RX residency
    assert_int_equal(d2d_link_data[0].bw_tx_flit_bytes, 2500 * 64); // State 0 TX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[0].bw_rx_flit_bytes, 2400 * 64); // State 0 RX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[0].link_id, 0);                  // State 0 Link ID

    assert_int_equal(d2d_link_data[1].tx_residency_mS, 300);     // State 1 TX residency
    assert_int_equal(d2d_link_data[1].rx_residency_mS, 250);     // State 1 RX residency
    assert_int_equal(d2d_link_data[1].bw_tx_flit_bytes, 0 * 64); // State 1 TX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[1].bw_rx_flit_bytes, 0 * 64); // State 1 RX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[1].link_id, 1);               // State 1 Link ID

    assert_int_equal(d2d_link_data[2].tx_residency_mS, 500);        // State 2 TX residency
    assert_int_equal(d2d_link_data[2].rx_residency_mS, 400);        // State 2 RX residency
    assert_int_equal(d2d_link_data[2].bw_tx_flit_bytes, 1200 * 64); // State 2 TX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[2].bw_rx_flit_bytes, 1100 * 64); // State 2 RX bandwidth (flit count * 64 bytes)
    assert_int_equal(d2d_link_data[2].link_id, 2);                  // State 2 Link ID

    // Test case 2: Invalid interface_id (out of bounds)
    pwr_soc_element_d2d_link_t invalid_d2d_data[NUMBER_OF_D2D_LINKS_STATE];
    memset(invalid_d2d_data, 0xFF, sizeof(invalid_d2d_data)); // Pre-fill with known values

    uint8_t invalid_interface_id = NUMBER_OF_D2D_INTERFACES;

    data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(invalid_interface_id, &invalid_d2d_data);

    // For invalid interface_id, the data should remain unchanged (error case)
    // Verify that the function didn't modify the pre-filled data
    assert_int_equal(invalid_d2d_data[0].tx_residency_mS, 0xFFFFFFFFFFFFFFFF);
    assert_int_equal(invalid_d2d_data[0].rx_residency_mS, 0xFFFFFFFFFFFFFFFF);
    assert_int_equal(invalid_d2d_data[0].link_id, 0xFF);

    // Test case 3: NULL pointer handling
    data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(valid_interface_id, NULL);
    // Function should handle NULL pointer gracefully without crashing

    // Test case 4: Edge case - maximum valid interface_id
    if (NUMBER_OF_D2D_INTERFACES > 1)
    {
        uint8_t max_interface_id = NUMBER_OF_D2D_INTERFACES - 1;

        // Set up test data for maximum interface - need all states, but only state 2 will be used
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[0].tx_residency_count = 4000000000ULL; // Converts to 2000 mS
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[0].rx_residency_count = 3600000000ULL; // Converts to 1800 mS
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[0].bw_tx_flit_count = 5000;
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[0].bw_rx_flit_count = 4800;
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[0].link_id = 0;

        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[1].tx_residency_count = 2000000000ULL; // Converts to 1000 mS
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[1].rx_residency_count = 1800000000ULL; // Converts to 900 mS
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[1].bw_tx_flit_count = 3000;
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[1].bw_rx_flit_count = 2800;
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[1].link_id = 1;

        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[2].tx_residency_count =
            6000000000ULL; // Converts to 3000 mS - final value
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[2].rx_residency_count =
            5000000000ULL; // Converts to 2500 mS - final value
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[2].bw_tx_flit_count = 7000; // final value
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[2].bw_rx_flit_count = 6500; // final value
        computed_metrics_2_mins.d2dss[max_interface_id].d2d_link[2].link_id = 2;             // final value

        pwr_soc_element_d2d_link_t max_d2d_data[NUMBER_OF_D2D_LINKS_STATE] = {{0}};

        data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(max_interface_id, &max_d2d_data);

        // With bug fixed, all array positions get populated correctly
        assert_int_equal(max_d2d_data[0].tx_residency_mS, 2000);       // State 0 value
        assert_int_equal(max_d2d_data[0].rx_residency_mS, 1800);       // State 0 value
        assert_int_equal(max_d2d_data[0].bw_tx_flit_bytes, 5000 * 64); // State 0 value
        assert_int_equal(max_d2d_data[0].bw_rx_flit_bytes, 4800 * 64); // State 0 value
        assert_int_equal(max_d2d_data[0].link_id, 0);                  // State 0 value

        assert_int_equal(max_d2d_data[1].tx_residency_mS, 1000);       // State 1 value
        assert_int_equal(max_d2d_data[1].rx_residency_mS, 900);        // State 1 value
        assert_int_equal(max_d2d_data[1].bw_tx_flit_bytes, 3000 * 64); // State 1 value
        assert_int_equal(max_d2d_data[1].bw_rx_flit_bytes, 2800 * 64); // State 1 value
        assert_int_equal(max_d2d_data[1].link_id, 1);                  // State 1 value

        assert_int_equal(max_d2d_data[2].tx_residency_mS, 3000);       // State 2 value
        assert_int_equal(max_d2d_data[2].rx_residency_mS, 2500);       // State 2 value
        assert_int_equal(max_d2d_data[2].bw_tx_flit_bytes, 7000 * 64); // State 2 value
        assert_int_equal(max_d2d_data[2].bw_rx_flit_bytes, 6500 * 64); // State 2 value
        assert_int_equal(max_d2d_data[2].link_id, 2);                  // State 2 value
    }

    // Test case 5: Zero values test
    // Clear computed metrics and verify zero handling
    memset(&computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link,
           0,
           sizeof(computed_metrics_2_mins.d2dss[valid_interface_id].d2d_link));

    pwr_soc_element_d2d_link_t zero_d2d_data[NUMBER_OF_D2D_LINKS_STATE] = {{0xFF}}; // Pre-fill to verify clearing

    data_proc_tlm_cmpnt_get_pwr_soc_d2d_link_data(valid_interface_id, &zero_d2d_data);

    // Verify zero values are handled correctly
    for (uint8_t state = 0; state < NUMBER_OF_D2D_LINKS_STATE; state++)
    {
        assert_int_equal(zero_d2d_data[state].tx_residency_mS, 0);
        assert_int_equal(zero_d2d_data[state].rx_residency_mS, 0);
        assert_int_equal(zero_d2d_data[state].bw_tx_flit_bytes, 0);
        assert_int_equal(zero_d2d_data[state].bw_rx_flit_bytes, 0);
        assert_int_equal(zero_d2d_data[state].link_id, state);
    }
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data, test_setup, test_teardown)
{
    pwr_soc_element_max_soc_temp_t max_temp_data = {0};

    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(500, 10, 600);

    computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.summation = 300 * 5;
    computed_metrics_d2d_2mins.max_soc_temp_dC.running_avg.num_samples = 5;
    computed_metrics_d2d_2mins.max_soc_temp_dC.max = 400;
    computed_metrics_d2d_2mins.max_soc_temp_dC.min = 200;

    data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(&max_temp_data);

    assert_int_equal(max_temp_data.average_max_dC, 433);
    assert_int_equal(max_temp_data.peak_max_dC, 600);

    computed_metrics_d2d_2mins.max_soc_temp_dC.max = 800;
    data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(&max_temp_data);
    assert_int_equal(max_temp_data.peak_max_dC, 800);
}

TEST_FUNCTION(test_get_pwr_mpam_core_pwr_data, test_setup, test_teardown)
{
    pwr_soc_element_mpam_core_power_t mpam_core_pwr_data = {0};
    uint16_t mpam_id = TEST_MPAM_ID_4;

    // Set up test values in computed_metrics_2_mins for this MPAM
    computed_metrics_2_mins.mpam[mpam_id].core_power.running_avg.summation = 5000 * 2;
    computed_metrics_2_mins.mpam[mpam_id].core_power.running_avg.num_samples = 2;
    computed_metrics_2_mins.mpam[mpam_id].core_power.max = 6000;

    // Call the API with valid MPAM ID
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_core_pwr_data(mpam_id, &mpam_core_pwr_data);

    // Verify the data was populated correctly
    assert_int_equal(mpam_core_pwr_data.average_mW, 5000); // summation / num_samples = 10000 / 2 = 5000
    assert_int_equal(mpam_core_pwr_data.max_mW, 6000);

    // Test boundary case - MPAM ID 127 (last valid index) to verify array access is working correctly
    uint16_t last_mpam_id = NUMBER_OF_MPAMS - 1; // 127
    computed_metrics_2_mins.mpam[last_mpam_id].core_power.running_avg.summation = 0x12345678;
    computed_metrics_2_mins.mpam[last_mpam_id].core_power.running_avg.num_samples = 2;
    computed_metrics_2_mins.mpam[last_mpam_id].core_power.max = 0xABCDEF00;

    pwr_soc_element_mpam_core_power_t mpam_127_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_core_pwr_data(last_mpam_id, &mpam_127_data);

    // Verify array position 127 works correctly
    assert_int_equal(mpam_127_data.average_mW, 0x12345678 / 2);
    assert_int_equal(mpam_127_data.max_mW, 0xABCDEF00);

    // Negative test: invalid MPAM ID (out of bounds)
    pwr_soc_element_mpam_core_power_t mpam_invalid_data;
    memset(&mpam_invalid_data, 0xFF, sizeof(mpam_invalid_data)); // Pre-fill with known values
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_core_pwr_data(NUMBER_OF_MPAMS, &mpam_invalid_data);

    // Should not have modified the structure (error case should not populate data)
    assert_int_equal(mpam_invalid_data.average_mW, 0xFFFFFFFF);
    assert_int_equal(mpam_invalid_data.max_mW, 0xFFFFFFFF);

    data_proc_tlm_cmpnt_get_pwr_soc_mpam_core_pwr_data(mpam_id, NULL); // Should not crash
}

TEST_FUNCTION(test_get_pwr_soc_mpam_throttle_data, test_setup, test_teardown)
{
    pwr_soc_element_mpam_throttle_t mpam_throttle_data = {0};
    uint16_t mpam_id = TEST_MPAM_ID_4;

    // Set up test values in computed_metrics_2_mins for this MPAM using correct field names
    computed_metrics_2_mins.mpam[mpam_id].residency_uS = 150000; // 150ms
    computed_metrics_2_mins.mpam[mpam_id].nominal_pstate = 5;
    computed_metrics_2_mins.mpam[mpam_id].active_pstate.running_avg.summation = 6 * 3;
    computed_metrics_2_mins.mpam[mpam_id].active_pstate.running_avg.num_samples = 3;
    computed_metrics_2_mins.mpam[mpam_id].active_pstate.max = 8;

    // Call the API with valid MPAM ID
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(mpam_id, &mpam_throttle_data);

    // Get expected frequency values by calling dvfs_get_freq_from_plimit directly
    uint32_t expected_nominal_freq = dvfs_get_freq_from_plimit(5);
    uint32_t expected_max_freq = dvfs_get_freq_from_plimit(8);
    uint32_t expected_avg_freq = dvfs_get_freq_from_plimit(6); // running_avg = 18/3 = 6

    // Verify the basic data was populated correctly
    assert_int_equal(mpam_throttle_data.throttle_duration_mS, 150); // 150000 uS / 1000 = 150 mS
    assert_int_equal(mpam_throttle_data.nominal_pstate_frequency_Mhz, expected_nominal_freq);
    assert_int_equal(mpam_throttle_data.max_pstate_frequency_Mhz, expected_max_freq);
    assert_int_equal(mpam_throttle_data.avg_pstate_frequency_Mhz, expected_avg_freq);

    // Calculate expected throttle extent: (nominal - max) / nominal * 100
    // Use the same formula as the implementation - this may wrap around for underflow cases
    uint32_t expected_throttle_extent = 0;
    if (expected_nominal_freq > 0)
    {
        // Cast to match the implementation exactly: (uint32_t)(nominal - max) * 100 / nominal
        expected_throttle_extent =
            (((uint32_t)(expected_nominal_freq - expected_max_freq) * 100U) / expected_nominal_freq);
    }
    assert_int_equal(mpam_throttle_data.throttle_extent_centipct, expected_throttle_extent);

    // Test case where max frequency > nominal frequency (underflow protection)
    pwr_soc_element_mpam_throttle_t mpam_data_underflow = {0};
    computed_metrics_2_mins.mpam[mpam_id].nominal_pstate = 10;
    computed_metrics_2_mins.mpam[mpam_id].active_pstate.max = 5; // Lower pstate = higher frequency

    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(mpam_id, &mpam_data_underflow);

    // When max frequency > nominal frequency, throttle extent should be 0 and an error should be logged
    uint32_t underflow_nominal_freq = dvfs_get_freq_from_plimit(10);
    uint32_t underflow_max_freq = dvfs_get_freq_from_plimit(5);
    if (underflow_max_freq > underflow_nominal_freq)
    {
        assert_int_equal(mpam_data_underflow.throttle_extent_centipct, 0);
    }

    // Test case for potential overflow: nominal_pstate = 0, active_pstate.max = 31
    pwr_soc_element_mpam_throttle_t mpam_data_overflow = {0};
    computed_metrics_2_mins.mpam[mpam_id].nominal_pstate = 0;     // Highest frequency
    computed_metrics_2_mins.mpam[mpam_id].active_pstate.max = 31; // Lowest frequency

    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(mpam_id, &mpam_data_overflow);

    // Calculate expected values - this tests the large frequency difference case
    uint32_t overflow_nominal_freq = dvfs_get_freq_from_plimit(0); // Maximum frequency
    uint32_t overflow_max_freq = dvfs_get_freq_from_plimit(31);    // Minimum frequency
    uint32_t expected_overflow_throttle =
        (((uint32_t)(overflow_nominal_freq - overflow_max_freq) * 100U) / overflow_nominal_freq);

    // Verify the calculation handles the large difference correctly
    assert_int_equal(mpam_data_overflow.throttle_extent_centipct, expected_overflow_throttle);
    // Ensure the result fits in uint16_t and doesn't overflow
    assert_true(mpam_data_overflow.throttle_extent_centipct <= 0xFFFF);

    // Negative test: invalid MPAM ID (out of bounds)
    pwr_soc_element_mpam_throttle_t invalid_data;
    memset(&invalid_data, 0xFF, sizeof(invalid_data));
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(NUMBER_OF_MPAMS, &invalid_data);

    // Should not have modified the structure (error case should not populate data)
    assert_int_equal(invalid_data.throttle_duration_mS, 0xFFFFFFFF); // uint32_t
    assert_int_equal(invalid_data.throttle_extent_centipct, 0xFFFF); // uint16_t

    // Negative test: null pointer (should not crash)
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(mpam_id, NULL);
}

TEST_FUNCTION(test_get_pwr_soc_mpam_memory_power_data, test_setup, test_teardown)
{
    // TODO: Implement the rest of the record once computed metrics are available for mpam memory power
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/2584713/?view=edit
    pwr_soc_element_mpam_memory_power_t mpam_memory_power_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_memory_power_data(TEST_MPAM_ID_4, &mpam_memory_power_data);
}

TEST_FUNCTION(test_get_pwr_soc_memory_throttle_data, test_setup, test_teardown)
{
    pwr_soc_element_memory_throttle_t memory_throttle_data = {0};
    uint16_t dimm_idx = TEST_DIMM_MOD_ID_3;

    // Set up test values in computed_metrics_2_mins for this DIMM
    computed_metrics_2_mins.soc.dimm[dimm_idx].entry_counts = 25;
    computed_metrics_2_mins.soc.dimm[dimm_idx].duration_mS = 1500;
    computed_metrics_2_mins.soc.dimm[dimm_idx].throttle_source = 3; // Some throttle source value

    // Call the API with valid DIMM index
    data_proc_tlm_cmpnt_get_pwr_soc_memory_throttle_data(dimm_idx, &memory_throttle_data);

    // Verify the data was populated correctly (dimm_id is set by the caller, not this function)
    assert_int_equal(memory_throttle_data.entry_counts, 25);
    assert_int_equal(memory_throttle_data.total_duration_mS, 1500);
    assert_int_equal(memory_throttle_data.throttle_source, 3);

    // Test boundary case - DIMM index 5 (last valid index for per-die) to verify array access
    uint16_t last_dimm_idx = NUMBER_OF_DIMMS_PER_DIE - 1; // 5
    computed_metrics_2_mins.soc.dimm[last_dimm_idx].entry_counts = 0x12345;
    computed_metrics_2_mins.soc.dimm[last_dimm_idx].duration_mS = 0x6789;
    computed_metrics_2_mins.soc.dimm[last_dimm_idx].throttle_source = 7;

    pwr_soc_element_memory_throttle_t memory_throttle_last_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_memory_throttle_data(last_dimm_idx, &memory_throttle_last_data);

    // Verify array position 5 works correctly (dimm_id is set by the caller, not this function)
    assert_int_equal(memory_throttle_last_data.entry_counts, 0x12345);
    assert_int_equal(memory_throttle_last_data.total_duration_mS, 0x6789);
    assert_int_equal(memory_throttle_last_data.throttle_source, 7);

    // Negative test: invalid DIMM index (out of bounds)
    pwr_soc_element_memory_throttle_t memory_throttle_invalid_data;
    // Pre-fill with distinctive known values to verify function doesn't modify them on error
    memory_throttle_invalid_data.entry_counts = 0xDEADBEEF;
    memory_throttle_invalid_data.total_duration_mS = 0xCAFEBABE;
    memory_throttle_invalid_data.throttle_source = 0xAB;
    memory_throttle_invalid_data.dimm_id = 0xCD;

    data_proc_tlm_cmpnt_get_pwr_soc_memory_throttle_data(NUMBER_OF_DIMMS_PER_DIE, &memory_throttle_invalid_data);

    // Should not have modified the structure (error case should not populate data)
    assert_int_equal(memory_throttle_invalid_data.entry_counts, 0xDEADBEEF);
    assert_int_equal(memory_throttle_invalid_data.total_duration_mS, 0xCAFEBABE);
    assert_int_equal(memory_throttle_invalid_data.throttle_source, 0xAB);
    // Note: dimm_id is set by the caller, not by this function, so we don't test it here

    // Negative test: null pointer (should not crash)
    data_proc_tlm_cmpnt_get_pwr_soc_memory_throttle_data(dimm_idx, NULL);
}

TEST_FUNCTION(test_get_inst_soc_core_summary_data, test_setup, test_teardown)
{
    inst_core_element_summary_t core_summary_data = {0};

    // Test case 1: Active core (core_is_active[core_id] == true)
    core_is_active[TEST_CORE_ID_5] = true;

    // pstate
    core_rt[TEST_CORE_ID_5].pstate_from_pstate_pkt = 10;
    uint8_t pstate_index = core_rt[TEST_CORE_ID_5].latest_pstate;

    // core_rt[TEST_CORE_ID_5].pstate[pstate_index].frequency_Mhz = 150;
    //  cstate
    core_rt[TEST_CORE_ID_5].latest_cstate = 1;
    uint16_t cstate_index = core_rt[TEST_CORE_ID_5].latest_cstate;

    // core voltage
    core_rt[TEST_CORE_ID_5].latest_voltage_mV = 3200;
    // core temperature and current,plimit.
    core_rt[TEST_CORE_ID_5].latest_current_mA = 30;
    core_rt[TEST_CORE_ID_5].latest_max_value_dC = 400;
    // plimit
    core_rt[TEST_CORE_ID_5].latest_plimit = 1;
    core_rt[TEST_CORE_ID_5].latest_cstate_entry_latency_uS = 100;

    // setup for the cstate exit latency
    core_rt[TEST_CORE_ID_5].latest_cstate_exit_latency_uS = 0;
    core_rt[TEST_CORE_ID_5].latest_cstate_exit_timestamp_uS = 1000;

    uint8_t cstate_timestamp_id = CSTATE_EXIT_PSCI;
    die_2_die_exch_init(0);
    //  init temp timestamp for each cstate entry/exit point
    uint64_t timestamp_cstate_uS[CSTATE_MAX_ID] = {3000, 3010, 3020, 3030, 3040, 3050};
    cstate_instr_timestamp_t* core_entry = &cstate_tfa_timestamp_base[TEST_CORE_ID_5];
    core_entry->timestamp[cstate_timestamp_id] = timestamp_cstate_uS[cstate_timestamp_id] * 1000; // Convert to nS

    // store for later comparison, because we clear latency at the end of the data_proc_tlm_cmpnt_get_inst_soc_core_summary_data
    uint32_t cstate_entry_latency_uS = core_rt[TEST_CORE_ID_5].latest_cstate_entry_latency_uS;
    uint8_t core_id = TEST_CORE_ID_5;
    for (uint8_t i = 0; i < NUMBER_OF_THROTTLE_SOURCES; i++)
    { // make all active
        core_rt[core_id].throttle_source_tracker[i] = true;
    }
    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);
    assert_int_equal(core_summary_data.pstate, pstate_index);
    assert_int_equal(core_summary_data.cstate, cstate_index);

    assert_int_equal(core_summary_data.plimit, core_rt[TEST_CORE_ID_5].latest_plimit);
    assert_int_equal(core_summary_data.power_mW, core_rt[TEST_CORE_ID_5].latest_power_mW);
    assert_int_equal(core_summary_data.frequency_Mhz, dvfs_get_freq_from_plimit(pstate_index));

    assert_int_equal(core_summary_data.voltage_mV, core_rt[TEST_CORE_ID_5].latest_voltage_mV);
    assert_int_equal(core_summary_data.current_mA, core_rt[TEST_CORE_ID_5].latest_current_mA);
    assert_int_equal(core_summary_data.temperature_dC, core_rt[TEST_CORE_ID_5].latest_max_value_dC);
    assert_int_equal(core_summary_data.throttling_type, 0x7f);
    assert_int_equal(core_summary_data.throttling_rack_priority, core_rt[TEST_CORE_ID_5].latest_rack_throttle_priority);
    assert_int_equal(core_summary_data.cstate_entry_latency_uS, cstate_entry_latency_uS);
    assert_int_equal(core_summary_data.cstate_exit_latency_uS,
                     (core_entry->timestamp[cstate_timestamp_id] / 1000) - core_rt[TEST_CORE_ID_5].latest_cstate_exit_timestamp_uS);

    // Test case 2: Inactive core (core_is_active[core_id] == false)
    core_is_active[TEST_CORE_ID_5] = false;

    // Fill core_summary_data with non-zero values to verify they get cleared
    memset(&core_summary_data, 0xFF, sizeof(inst_core_element_summary_t));

    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);

    // Verify that all fields are zeroed out for inactive core
    assert_int_equal(core_summary_data.pstate, 0);
    assert_int_equal(core_summary_data.cstate, 0);
    assert_int_equal(core_summary_data.plimit, 0);
    assert_int_equal(core_summary_data.power_mW, 0);
    assert_int_equal(core_summary_data.frequency_Mhz, 0);
    assert_int_equal(core_summary_data.voltage_mV, 0);
    assert_int_equal(core_summary_data.current_mA, 0);
    assert_int_equal(core_summary_data.temperature_dC, 0);
    assert_int_equal(core_summary_data.throttling_type, 0);
    assert_int_equal(core_summary_data.throttling_rack_priority, 0);
    assert_int_equal(core_summary_data.mpam_id, 0);
    assert_int_equal(core_summary_data.cstate_entry_latency_uS, 0);
    assert_int_equal(core_summary_data.cstate_exit_latency_uS, 0);
    assert_int_equal(core_summary_data.velocity_boost_priority, 0);

    // Test case 3: Invalid parameters - test with invalid core_id
    memset(&core_summary_data, 0xFF, sizeof(inst_core_element_summary_t));
    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(NUMBER_OF_CORES_PER_DIE, &core_summary_data);
    // Data should remain unchanged when core_id is invalid
    assert_int_equal(core_summary_data.pstate, 0xFF);

    // Test case 4: Invalid parameters - test with NULL pointer
    data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(TEST_CORE_ID_5, NULL);
    // Function should handle NULL gracefully (no crash expected)
}

TEST_FUNCTION(test_get_inst_soc_rail_data, test_setup, test_teardown)
{
    inst_soc_element_rail_t rail_data = {0};

    // Test case 1: Valid rail data on primary die (die ID 0)
    die_2_die_exch_init(0); // PRIMARY_DIE_ID = 0

    // Set up test data in soc_rt for a valid rail
    soc_rt.latest_rail_current_mA[TEST_RAIL_ID_2] = 1500;
    soc_rt.latest_rail_temperature_dC[TEST_RAIL_ID_2] = 65;
    soc_rt.latest_rail_voltage_mV[TEST_RAIL_ID_2] = 1200;

    data_proc_tlm_cmpnt_get_inst_soc_rail_data(TEST_RAIL_ID_2, &rail_data);

    // Verify that valid rail data is populated correctly
    assert_int_equal(rail_data.current_mA, 1500);
    assert_int_equal(rail_data.temperature_dC, 65);
    assert_int_equal(rail_data.voltage_mV, 1200);

    // Test case 2: Valid rail data on secondary die (die ID 1)
    die_2_die_exch_init(1); // Secondary die

    // Set up test data for a rail valid on secondary die (rail_id < NUM_DIE1_VR_RAILS = 2)
    uint16_t valid_rail_die1 = 1; // This should be < NUM_DIE1_VR_RAILS (2)
    soc_rt.latest_rail_current_mA[valid_rail_die1] = 800;
    soc_rt.latest_rail_temperature_dC[valid_rail_die1] = 55;
    soc_rt.latest_rail_voltage_mV[valid_rail_die1] = 900;

    data_proc_tlm_cmpnt_get_inst_soc_rail_data(valid_rail_die1, &rail_data);

    // Verify that valid rail data is populated correctly on secondary die
    assert_int_equal(rail_data.current_mA, 800);
    assert_int_equal(rail_data.temperature_dC, 55);
    assert_int_equal(rail_data.voltage_mV, 900);

    // Test case 3: Rail ID out of bounds for secondary die
    // NUM_DIE1_VR_RAILS = 2, so rail_id = 2 should be out of bounds for die 1
    uint16_t invalid_rail_die1 = 2;
    memset(&rail_data, 0xFF, sizeof(inst_soc_element_rail_t)); // Fill with non-zero values

    data_proc_tlm_cmpnt_get_inst_soc_rail_data(invalid_rail_die1, &rail_data);

    // Verify that data is zeroed when rail_id >= num_rails for the die
    assert_int_equal(rail_data.current_mA, 0);
    assert_int_equal(rail_data.temperature_dC, 0);
    assert_int_equal(rail_data.voltage_mV, 0);

    // Test case 4: Invalid parameters - rail_id >= MAX_NUM_OF_VR_RAILS
    memset(&rail_data, 0xFF, sizeof(inst_soc_element_rail_t));
    data_proc_tlm_cmpnt_get_inst_soc_rail_data(MAX_NUM_OF_VR_RAILS, &rail_data);
    // Data should remain unchanged when rail_id is invalid
    assert_int_equal(rail_data.current_mA, 0xFFFFFFFF); // current_mA is uint32_t
    assert_int_equal(rail_data.voltage_mV, 0xFFFF);     // voltage_mV is uint16_t
    assert_int_equal(rail_data.temperature_dC, 0xFFFF); // temperature_dC is uint16_t

    // Test case 5: Invalid parameters - NULL pointer
    data_proc_tlm_cmpnt_get_inst_soc_rail_data(TEST_RAIL_ID_2, NULL);
    // Function should handle NULL gracefully (no crash expected)

    // Test case 6: Edge case - rail_id at boundary for primary die
    // On primary die, NUM_DIE0_VR_RAILS = MAX_NUM_OF_VR_RAILS, so this tests the boundary
    die_2_die_exch_init(0); // PRIMARY_DIE_ID = 0
    uint16_t boundary_rail = MAX_NUM_OF_VR_RAILS - 1;
    soc_rt.latest_rail_current_mA[boundary_rail] = 2000;
    soc_rt.latest_rail_temperature_dC[boundary_rail] = 70;
    soc_rt.latest_rail_voltage_mV[boundary_rail] = 1100;

    data_proc_tlm_cmpnt_get_inst_soc_rail_data(boundary_rail, &rail_data);

    // Verify boundary rail works on primary die
    assert_int_equal(rail_data.current_mA, 2000);
    assert_int_equal(rail_data.temperature_dC, 70);
    assert_int_equal(rail_data.voltage_mV, 1100);
}

TEST_FUNCTION(test_get_inst_soc_dimm_runtime_data, test_setup, test_teardown)
{
    inst_soc_element_dimm_runtime_t dimm_runtime_data = {0};

    // Setup test data in dimm_rt global structure
    dimm_rt.latest_dimm[TEST_DIMM_MOD_ID_3].temperature_dC = 750; // 75°C
    dimm_rt.latest_dimm[TEST_DIMM_MOD_ID_3].power_mW = 5000;      // 5W
    dimm_rt.latest_dimm[TEST_DIMM_MOD_ID_3].threshold_dC = 850;   // 85°C (critical threshold)
    dimm_rt.latest_dimm[TEST_DIMM_MOD_ID_3].throttle_source = 2;
    dimm_rt.latest_dimm[TEST_DIMM_MOD_ID_3].memory_freq_id = 1;

    // Call the function under test
    data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(TEST_DIMM_MOD_ID_3, &dimm_runtime_data);

    // Verify the threshold field is correctly retrieved (fixes Azure DevOps #3163284)
    assert_int_equal(dimm_runtime_data.threshold_dC, 850);
    assert_int_equal(dimm_runtime_data.temperature_dC, 750);
    assert_int_equal(dimm_runtime_data.power_mW, 5000);
    assert_int_equal(dimm_runtime_data.throttle_source, 2);
    assert_int_equal(dimm_runtime_data.memory_freq_id, 1);

    // Test boundary condition - invalid DIMM ID
    data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(NUMBER_OF_DIMMS_PER_DIE, &dimm_runtime_data);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data, test_setup, test_teardown)
{
    inst_soc_element_die_temp_t sensor_temp_data = {0};

    soc_rt.latest_soc_top_temp_dC[7] = 0x23;
    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(7, &sensor_temp_data);

    assert_int_equal(sensor_temp_data.temperature_dC, 0x23);

    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(NUMBER_OF_SOC_TEMP_SENSORS, &sensor_temp_data);
    data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(7, nullptr);
}

TEST_FUNCTION(test_get_inst_soc_sensor_temp_data, test_setup, test_teardown)
{
    inst_soc_element_max_temp_t read_data = {0};

    soc_rt.latest_max_die_temp_dC = 200;
    die_2_die_exch_init(1);
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
    computed_metrics_2_mins.cores[core_id].power_mW.running_avg.summation = 15 * 1;
    computed_metrics_2_mins.cores[core_id].power_mW.running_avg.num_samples = 1;

    data_proc_tlm_cmpnt_get_pwr_core_power_data(core_id, &power_data);

    assert_int_equal(power_data.min_mW, computed_metrics_2_mins.cores[core_id].power_mW.min);
    assert_int_equal(power_data.max_mW, computed_metrics_2_mins.cores[core_id].power_mW.max);
    assert_int_equal(power_data.average_mW,
                     data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].power_mW.running_avg));
    // invalid case
    computed_metrics_2_mins.cores[core_id].power_mW.min = 20;
    data_proc_tlm_cmpnt_get_pwr_core_power_data(NUMBER_OF_CORES_PER_DIE, &power_data);
    assert_int_not_equal(power_data.min_mW, computed_metrics_2_mins.cores[core_id].power_mW.min);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_core_droop_count_data, test_setup, test_teardown)
{
    // Test validates that the records published to consumers is using the correct data source

    pwr_core_element_droop_count_t droop_counts = {0};

    uint8_t core_id = 0;
    computed_metrics_2_mins.cores[core_id].droop_count = 10;

    // core
    computed_metrics_2_mins.cores[core_id].voltage_mV.min = 1200;
    computed_metrics_2_mins.cores[core_id].voltage_mV.max = 1800;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.summation = 1500 * 1;
    computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg.num_samples = 1;

    // vcpu
    computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.min = 1200;
    computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.max = 1800;
    computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg.summation = 1500 * 1;
    computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg.num_samples = 1;

    data_proc_tlm_cmpnt_get_pwr_core_droop_count_data(core_id, &droop_counts);

    assert_int_equal(droop_counts.droop_count, computed_metrics_2_mins.cores[core_id].droop_count);
    assert_int_equal(droop_counts.ldo_output_voltage.average_mV,
                     data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].voltage_mV.running_avg));

    assert_int_equal(droop_counts.ldo_output_voltage.min_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.min);
    assert_int_equal(droop_counts.ldo_output_voltage.max_mV, computed_metrics_2_mins.cores[core_id].voltage_mV.max);

    assert_int_equal(
        droop_counts.vcpu_input_voltage.average_mV,
        data_util_running_avg_u16_get(&computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.running_avg));
    assert_int_equal(droop_counts.vcpu_input_voltage.min_mV,
                     computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.min);
    assert_int_equal(droop_counts.vcpu_input_voltage.max_mV,
                     computed_metrics_2_mins.cores[core_id].vcpu_input_voltage_mV.max);

    // invalid case
    computed_metrics_2_mins.cores[core_id].droop_count = 20;
    data_proc_tlm_cmpnt_get_pwr_core_droop_count_data(NUMBER_OF_CORES_PER_DIE, &droop_counts);
    assert_int_not_equal(droop_counts.droop_count, computed_metrics_2_mins.cores[core_id].droop_count);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_pwr_core_aging_counters_data, test_setup, test_teardown)
{

    pwr_core_element_aging_t aging_data[NUMBER_OF_AGING_COUNTER_PAIRS] = {{0}};
    uint8_t counter_id = 0;
    uint8_t core_id = 0;
    for (counter_id = 0; counter_id < NUMBER_OF_AGING_COUNTER_PAIRS; counter_id++)
    {
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter = 20;
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter = 10;
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].counter_id = counter_id;
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS = 1000;
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].temperature_dC = 20;
        computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].voltage_mV = 100;
    }

    data_proc_tlm_cmpnt_get_pwr_core_aging_data(core_id, &aging_data);

    for (counter_id = 0; counter_id < NUMBER_OF_AGING_COUNTER_PAIRS; counter_id++)
    {
        assert_int_equal(aging_data[counter_id].counter_id, counter_id);
        assert_int_equal(aging_data[counter_id].aged_counter,
                         computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].aged_counter);
        assert_int_equal(aging_data[counter_id].unaged_counter,
                         computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].unaged_counter);
        assert_int_equal(aging_data[counter_id].timestamp_uS,
                         computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].timestamp_uS);
        assert_int_equal(aging_data[counter_id].voltage_mV,
                         computed_metrics_24_hrs.cores[core_id].core_aging_counters[counter_id].voltage_mV);
    }

    // case : core id out of range
    core_id = NUMBER_OF_CORES_PER_DIE;
    data_proc_tlm_cmpnt_get_pwr_core_aging_data(core_id, &aging_data);
}

// Tests for filtering logic - verify zero-filtering behavior
TEST_FUNCTION(test_get_pwr_core_pstate_data_zero_filtering_enabled, test_setup, test_teardown)
{
    pwr_core_element_pstate_t pstate_array[NUMBER_OF_PSTATES] = {{0}};

    // Enable filtering (default is true)
    package_inf_init(0, true);

    // Set all pstate data to zero except pstate 3
    for (uint16_t pstate_id = 0; pstate_id < NUMBER_OF_PSTATES; pstate_id++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.summation = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.num_samples = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.min = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.max = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].entry_count = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].residency_uS = 0;
    }

    // Set pstate 3 to non-zero
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[3].power_mW.running_avg.summation = 1000 * 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[3].power_mW.running_avg.num_samples = 1;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[3].power_mW.max = 100;

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(TEST_CORE_ID_5, &pstate_array);

    // Only pstate 3 should have non-zero pstate_id (filtering is enabled)
    for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
    {
        if (pstate_index == 3)
        {
            assert_int_equal(pstate_array[pstate_index].pstate_id, 3);
        }
        else
        {
            assert_int_equal(pstate_array[pstate_index].pstate_id, 0);
        }
    }
}

TEST_FUNCTION(test_get_pwr_core_pstate_data_zero_filtering_disabled, test_setup, test_teardown)
{
    pwr_core_element_pstate_t pstate_array[NUMBER_OF_PSTATES] = {{0}};

    // Disable filtering
    package_inf_init(0, false);

    // Set all pstate data to zero
    for (uint16_t pstate_id = 0; pstate_id < NUMBER_OF_PSTATES; pstate_id++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.summation = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.running_avg.num_samples = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.min = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].power_mW.max = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].entry_count = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].pstate[pstate_id].residency_uS = 0;
    }

    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(TEST_CORE_ID_5, &pstate_array);

    // All pstate_id values should be set (filtering is disabled)
    for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
    {
        assert_int_equal(pstate_array[pstate_index].pstate_id, pstate_index);
    }
}

TEST_FUNCTION(test_get_pwr_core_cstate_data_zero_filtering_enabled, test_setup, test_teardown)
{
    pwr_core_element_cstate_t cstate_array[NUMBER_OF_CSTATES] = {{0}};

    // Enable filtering
    package_inf_init(0, true);

    // Set all cstate data to zero except cstate 2
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].entry_count = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].residency_uS = 0;
    }

    // Set cstate 2 to non-zero
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[2].entry_count = 5;
    computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[2].residency_uS = 1000;

    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(TEST_CORE_ID_5, &cstate_array);

    // Only cstate 2 should have non-zero cstate_id (filtering is enabled)
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        if (cstate_index == 2)
        {
            assert_int_equal(cstate_array[cstate_index].cstate_id, 2);
        }
        else
        {
            assert_int_equal(cstate_array[cstate_index].cstate_id, 0);
        }
    }
}

TEST_FUNCTION(test_get_pwr_core_cstate_data_zero_filtering_disabled, test_setup, test_teardown)
{
    pwr_core_element_cstate_t cstate_array[NUMBER_OF_CSTATES] = {{0}};

    // Disable filtering
    package_inf_init(0, false);

    // Set all cstate data to zero
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].entry_count = 0;
        computed_metrics_2_mins.cores[TEST_CORE_ID_5].cstate[cstate_index].residency_uS = 0;
    }

    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(TEST_CORE_ID_5, &cstate_array);

    // All cstate_id values should be set (filtering is disabled)
    for (uint16_t cstate_index = 0; cstate_index < NUMBER_OF_CSTATES; cstate_index++)
    {
        assert_int_equal(cstate_array[cstate_index].cstate_id, cstate_index);
    }
}

TEST_FUNCTION(test_get_pwr_core_histogram_data_zero_filtering_enabled, test_setup, test_teardown)
{
    pwr_core_element_histogram_t histogram_array[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES] = {{{0}}};

    // Enable filtering
    package_inf_init(0, true);

    // Set all histogram bins to zero except [1][2]
    for (uint8_t voltage_idx = 0; voltage_idx < NUMBER_OF_HS_VOLTAGE_SCALES; voltage_idx++)
    {
        for (uint8_t temp_idx = 0; temp_idx < NUMBER_OF_HS_TEMP_SCALES; temp_idx++)
        {
            computed_metrics_24_hrs.cores[TEST_CORE_ID_5].histogram.bin_count[voltage_idx][temp_idx] = 0;
        }
    }

    // Set bin [1][2] to non-zero
    computed_metrics_24_hrs.cores[TEST_CORE_ID_5].histogram.bin_count[1][2] = 100;

    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, &histogram_array);

    // Only [1][2] should have non-zero voltage_band and temperature_band
    for (uint8_t voltage_idx = 0; voltage_idx < NUMBER_OF_HS_VOLTAGE_SCALES; voltage_idx++)
    {
        for (uint8_t temp_idx = 0; temp_idx < NUMBER_OF_HS_TEMP_SCALES; temp_idx++)
        {
            if (voltage_idx == 1 && temp_idx == 2)
            {
                assert_int_equal(histogram_array[voltage_idx][temp_idx].voltage_band, 1);
                assert_int_equal(histogram_array[voltage_idx][temp_idx].temperature_band, 2);
            }
            else
            {
                assert_int_equal(histogram_array[voltage_idx][temp_idx].voltage_band, 0);
                assert_int_equal(histogram_array[voltage_idx][temp_idx].temperature_band, 0);
            }
        }
    }
}

TEST_FUNCTION(test_get_pwr_core_histogram_data_zero_filtering_disabled, test_setup, test_teardown)
{
    pwr_core_element_histogram_t histogram_array[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES] = {{{0}}};

    // Disable filtering
    package_inf_init(0, false);

    // Set all histogram bins to zero
    for (uint8_t voltage_idx = 0; voltage_idx < NUMBER_OF_HS_VOLTAGE_SCALES; voltage_idx++)
    {
        for (uint8_t temp_idx = 0; temp_idx < NUMBER_OF_HS_TEMP_SCALES; temp_idx++)
        {
            computed_metrics_24_hrs.cores[TEST_CORE_ID_5].histogram.bin_count[voltage_idx][temp_idx] = 0;
        }
    }

    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, &histogram_array);

    // All bands should be set with their indices (filtering is disabled)
    for (uint8_t voltage_idx = 0; voltage_idx < NUMBER_OF_HS_VOLTAGE_SCALES; voltage_idx++)
    {
        for (uint8_t temp_idx = 0; temp_idx < NUMBER_OF_HS_TEMP_SCALES; temp_idx++)
        {
            assert_int_equal(histogram_array[voltage_idx][temp_idx].voltage_band, voltage_idx);
            assert_int_equal(histogram_array[voltage_idx][temp_idx].temperature_band, temp_idx);
        }
    }
}
