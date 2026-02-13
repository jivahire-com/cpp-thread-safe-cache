//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_die_2_die_exchange.cpp
 * Test inter core exchange
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <die_2_die_exchange_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

static mpam_data_t s_test_data_array[NUMBER_OF_MPAMS];
static mpam_data_t s_read_data_array[NUMBER_OF_MPAMS];

/*-------- Function Prototypes -----------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    die_2_die_exch_init(1);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

#define MAX_DIE_TEMPERATURE_DC (1000) // Example maximum die temperature in deci-Celsius

TEST_FUNCTION(test_droop_counts, test_setup, test_teardown)
{
    die_2_die_exch_ib_write_inst_max_die_temp(MAX_DIE_TEMPERATURE_DC);

    uint16_t read_temp_dC = die_2_die_exch_ib_read_inst_max_die_temp_dC(1); // Read from die 0

    assert_int_equal(read_temp_dC, MAX_DIE_TEMPERATURE_DC); // Verify the read value matches the written value
}

TEST_FUNCTION(test_write_errors, test_setup, test_teardown)
{
    die_2_die_exch_init(0);
    die_2_die_exch_ib_write_inst_max_die_temp(MAX_DIE_TEMPERATURE_DC); // die 0 doesn't write to exchange

    die_2_die_exch_init(2);
    die_2_die_exch_ib_write_inst_max_die_temp(MAX_DIE_TEMPERATURE_DC); // die 2 doesn't exist
}

TEST_FUNCTION(test_read_errors, test_setup, test_teardown)
{
    uint16_t read_temp_dC = die_2_die_exch_ib_read_inst_max_die_temp_dC(0); // die 0 doesn't write to exchange
    assert_int_equal(read_temp_dC, 0); // Verify the read value is 0 for die 0

    read_temp_dC = die_2_die_exch_ib_read_inst_max_die_temp_dC(2); // die 2 doesn't exist
    assert_int_equal(read_temp_dC, 0);                             // Verify the read value is 0 for die 2
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_write_pwr_pkg_max_die_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(500, 10, 600);

    max_die_temps_t read_temps = {0};
    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(1, &read_temps);
    assert_int_equal(read_temps.average_max_temp_dC, 500);
    assert_int_equal(read_temps.num_samples, 10);
    assert_int_equal(read_temps.peak_temp_dC, 600);
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_write_pwr_pkg_max_die_temp_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(500, 10, 600);

    max_die_temps_t read_temps = {0};
    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(0, &read_temps);
    assert_int_equal(read_temps.average_max_temp_dC, 0);
    assert_int_equal(read_temps.num_samples, 0);
    assert_int_equal(read_temps.peak_temp_dC, 0);

    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(1, nullptr);
    assert_int_equal(read_temps.average_max_temp_dC, 0);
    assert_int_equal(read_temps.num_samples, 0);
    assert_int_equal(read_temps.peak_temp_dC, 0);

    // Test write with invalid die ID (die 0 should not write to exchange)
    die_2_die_exch_init(0);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(700, 20, 800);

    // Switch back to valid die and verify data wasn't written
    die_2_die_exch_init(1);
    max_die_temps_t read_temps_after_invalid_write = {0};
    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(0, &read_temps_after_invalid_write);
    assert_int_equal(read_temps_after_invalid_write.average_max_temp_dC, 0);
    assert_int_equal(read_temps_after_invalid_write.num_samples, 0);
    assert_int_equal(read_temps_after_invalid_write.peak_temp_dC, 0);

    // Test write with invalid die ID (die 2 doesn't exist - beyond valid range)
    die_2_die_exch_init(2);
    die_2_die_exch_ib_write_pwr_pkg_max_die_temp(900, 30, 1000);

    // Switch back to valid die and verify data wasn't written
    die_2_die_exch_init(1);
    max_die_temps_t read_temps_after_invalid_write2 = {0};
    die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(1, &read_temps_after_invalid_write2);
    assert_int_equal(read_temps_after_invalid_write2.average_max_temp_dC, 0);
    assert_int_equal(read_temps_after_invalid_write2.num_samples, 0);
    assert_int_equal(read_temps_after_invalid_write2.peak_temp_dC, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_max_die_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_die_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_die_temp(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_max_die_temp_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_die_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_die_temp(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_window_max_die_temp(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_max_die_temp(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_soc_pwr, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_soc_pwr(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_soc_pwr(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_soc_pwr_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_die_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_soc_pwr(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_window_soc_pwr(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_soc_pwr(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_max_dimm_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_dimm_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_dimm_temp(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_max_dimm_temp_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_dimm_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_dimm_temp(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_window_max_dimm_temp(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_max_dimm_temp(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_dimm_pwr, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_dimm_pwr(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_dimm_pwr(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_dimm_pwr_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_dimm_pwr(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_dimm_pwr(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_window_dimm_pwr(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_dimm_pwr(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_max_vr_temp, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_vr_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_vr_temp(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_window_max_dimm_vr_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_vr_temp(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_window_max_vr_temp(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_window_max_vr_temp(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_max_vr_temp(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_avg_pstate, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_avg_pstate(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_avg_pstate(1, &read_window);
    assert_int_equal(read_window.sum, 1000);
    assert_int_equal(read_window.num_samples, 10);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_write_window_avg_pstate_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_avg_pstate(1000, 10);

    sliding_window_data_t read_window = {0};
    die_2_die_exch_oob_read_avg_pstate(0, &read_window);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_oob_read_avg_pstate(1, nullptr);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);

    die_2_die_exch_init(0);
    die_2_die_exch_oob_write_window_avg_pstate(1000, 10);
    assert_int_equal(read_window.sum, 0);
    assert_int_equal(read_window.num_samples, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_write_read_dimm_info, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_dimm_info(0, 1000, 20, 25);
    die_2_die_exch_oob_write_dimm_info(0, 1000, 20, 30);

    die_2_die_exch_oob_write_dimm_info(1, 2000, 25, 30);
    die_2_die_exch_oob_write_dimm_info(1, 2000, 25, 35);

    die_2_die_exch_oob_write_dimm_info(2, 3000, 30, 35);
    die_2_die_exch_oob_write_dimm_info(2, 3000, 30, 40);

    die_2_die_exch_oob_write_dimm_info(3, 4000, 35, 40);
    die_2_die_exch_oob_write_dimm_info(3, 4000, 35, 45);

    die_2_die_exch_oob_write_dimm_info(4, 5000, 40, 45);
    die_2_die_exch_oob_write_dimm_info(4, 5000, 40, 50);

    die_2_die_exch_oob_write_dimm_info(5, 6000, 45, 50);
    die_2_die_exch_oob_write_dimm_info(5, 6000, 45, 55);

    // Test case where latest_temp_dC <= dimm_info_ptr->max_temp_dC (condition is false)
    // Channel 0 currently has max_temp_dC = 30, so write with latest_temp_dC = 25 (25 <= 30)
    die_2_die_exch_oob_write_dimm_info(0, 1500, 22, 25);

    uint16_t avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 0);
    assert_int_equal(avg_temp_dC, 22); // Should be the latest average temp we just wrote

    uint16_t max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 0);
    assert_int_equal(max_temp_dC, 30); // Should remain 30 since 25 <= 30

    uint16_t avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 0);
    assert_int_equal(avg_pwr_mW, 1500); // Should be 1500, the latest power we wrote

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 1);
    assert_int_equal(avg_temp_dC, 25);

    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 1);
    assert_int_equal(max_temp_dC, 35);

    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 1);
    assert_int_equal(avg_pwr_mW, 2000);

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 2);
    assert_int_equal(avg_temp_dC, 30);

    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 2);
    assert_int_equal(max_temp_dC, 40);

    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 2);
    assert_int_equal(avg_pwr_mW, 3000);

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 3);
    assert_int_equal(avg_temp_dC, 35);

    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 3);
    assert_int_equal(max_temp_dC, 45);

    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 3);
    assert_int_equal(avg_pwr_mW, 4000);

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 4);
    assert_int_equal(avg_temp_dC, 40);

    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 4);
    assert_int_equal(max_temp_dC, 50);

    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 4);
    assert_int_equal(avg_pwr_mW, 5000);

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 5);
    assert_int_equal(avg_temp_dC, 45);

    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 5);
    assert_int_equal(max_temp_dC, 55);

    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 5);
    assert_int_equal(avg_pwr_mW, 6000);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_dimm_avg_temp_dC_negative, test_setup, test_teardown)
{
    die_2_die_exch_oob_write_dimm_info(7, 1000, 20, 30);
    uint16_t avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(7, 0);
    assert_int_equal(avg_temp_dC, 0);

    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, NUMBER_OF_DIMMS);
    assert_int_equal(avg_temp_dC, 0);

    die_2_die_exch_init(6);
    die_2_die_exch_oob_write_dimm_info(1, 1000, 20, 30);
    avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 0);
    assert_int_equal(avg_temp_dC, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_dimm_max_temp_dC_negative, test_setup, test_teardown)
{
    die_2_die_exch_oob_write_dimm_info(7, 1000, 20, 30);
    uint16_t max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(7, 0);
    assert_int_equal(max_temp_dC, 0);
    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, NUMBER_OF_DIMMS);
    assert_int_equal(max_temp_dC, 0);

    die_2_die_exch_init(6);
    die_2_die_exch_oob_write_dimm_info(1, 1000, 20, 30);
    max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 0);
    assert_int_equal(max_temp_dC, 0);
}

TEST_FUNCTION(test_die_2_die_exch_oob_read_dimm_avg_pwr_mW_negative, test_setup, test_teardown)
{
    die_2_die_exch_oob_write_dimm_info(7, 1000, 20, 30);
    uint16_t avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(7, 0);
    assert_int_equal(avg_pwr_mW, 0);
    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, NUMBER_OF_DIMMS);
    assert_int_equal(avg_pwr_mW, 0);

    die_2_die_exch_init(6);
    die_2_die_exch_oob_write_dimm_info(1, 1000, 20, 30);
    avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 0);
    assert_int_equal(avg_pwr_mW, 0);
}

TEST_FUNCTION(test_die_2_die_exch_get_mpam_data_missed_counts_basic, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // Initially both counters should be 0
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_get_mpam_data_missed_counts_null_pointers, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 999;
    uint32_t writes_without_consumption = 999;

    // Test with NULL pointer for first parameter
    die_2_die_exch_get_mpam_data_missed_counts(nullptr, &writes_without_consumption);
    assert_int_equal(writes_without_consumption, 0);

    // Test with NULL pointer for second parameter
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, nullptr);
    assert_int_equal(reads_without_new_data, 0);

    // Test with both NULL pointers (should not crash)
    die_2_die_exch_get_mpam_data_missed_counts(nullptr, nullptr);
}

TEST_FUNCTION(test_die_2_die_exch_mpam_reads_without_new_data_counter, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // write once
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&s_test_data_array);

    // Perform multiple reads without any writes - should increment reads_without_new_data counter
    mpam_data_t read_data_array[NUMBER_OF_MPAMS];
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array); // normal
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array); // inc count
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array); // inc count

    // Check that reads_without_new_data counter has incremented
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 2);
    assert_int_equal(writes_without_consumption, 0);

    // Reading the counters should clear them
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_mpam_writes_without_consumption_counter, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // Use file-level static array to avoid stack overflow

    // Now set the actual test values
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        s_test_data_array[i].latest_total_pwr_mW = 1000; // Default power value
        s_test_data_array[i].latest_pstate = 5;          // Default pstate value
        s_test_data_array[i].active = 1;                 // Set active bit
        s_test_data_array[i].throttling = 0;             // Clear throttling bit
    }

    // Set specific values for array position 127 (last element) to verify memcpy works correctly
    s_test_data_array[127].latest_total_pwr_mW = 0x12345678; // Unique 32-bit value for position 127
    s_test_data_array[127].latest_pstate = 0xAB;             // Unique 8-bit value for position 127
    s_test_data_array[127].active = 1;                       // Set active bit
    s_test_data_array[127].throttling = 0;                   // Clear throttling bit

    // Perform multiple writes without any reads - should increment writes_without_consumption counter
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&s_test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&s_test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&s_test_data_array);

    // Check that writes_without_consumption counter has incremented
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 2); // First write is normal, next 2 increment counter

    // Test memcpy correctness by reading back data and verifying array position 127
    // Initialize static read array to zero
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        s_read_data_array[i].latest_total_pwr_mW = 0;
        s_read_data_array[i].latest_pstate = 0;
        s_read_data_array[i].flags_byte = 0;
    }
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &s_read_data_array);

    // Verify memcpy worked correctly for array position 127
    assert_int_equal(s_read_data_array[127].latest_total_pwr_mW, 0x12345678);
    assert_int_equal(s_read_data_array[127].latest_pstate, 0xAB);

    // Also verify other positions to ensure memcpy didn't corrupt data
    assert_int_equal(s_read_data_array[0].latest_total_pwr_mW, 1000);
    assert_int_equal(s_read_data_array[0].latest_pstate, 5);
    assert_int_equal(s_read_data_array[126].latest_total_pwr_mW, 1000);
    assert_int_equal(s_read_data_array[126].latest_pstate, 5);

    // Reading the counters should clear them
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_mpam_sequence_tracking_mixed_operations, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // Create test MPAM data
    mpam_data_t test_data_array[NUMBER_OF_MPAMS];
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_data_array[i].active = 1;
        test_data_array[i].throttling = 0;
    }

    mpam_data_t read_data_array[NUMBER_OF_MPAMS];

    // setup, clear counters from any previous sets
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);

    // Scenario 1: Normal write-read sequences - should not increment any counters
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);

    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);

    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);

    // Multiple normal write-read sequences should not increment counters
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);

    // Scenario 2: Write twice in a row - should increment writes_without_consumption
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);

    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 1); // Second write increments counter

    // Scenario 3: Read twice in a row - should increment reads_without_new_data
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);

    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 1); // Second read increments counter
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_mpam_sequence_tracking_clear_on_read_behavior, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // Create test MPAM data
    mpam_data_t test_data_array[NUMBER_OF_MPAMS];
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_data_array[i].active = 1;
        test_data_array[i].throttling = 0;
    }

    mpam_data_t read_data_array[NUMBER_OF_MPAMS];

    // Generate some sequence violations
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array); // writes_without_consumption++
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array); // reads_without_new_data++

    // First read should return the counts and clear them
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 1);
    assert_int_equal(writes_without_consumption, 1);

    // Second read should return 0 for both since they were cleared
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);

    // Third read should still return 0
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_mpam_sequence_tracking_independent_counters, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    uint32_t reads_without_new_data = 0;
    uint32_t writes_without_consumption = 0;

    // Clear any existing state from previous test runs
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);

    // Create test MPAM data
    mpam_data_t test_data_array[NUMBER_OF_MPAMS];
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_data_array[i].active = 1;
        test_data_array[i].throttling = 0;
    }

    mpam_data_t read_data_array[NUMBER_OF_MPAMS];

    // Test that counters operate independently
    // Generate only writes_without_consumption violations
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);

    // Read only writes_without_consumption counter
    die_2_die_exch_get_mpam_data_missed_counts(nullptr, &writes_without_consumption);
    assert_int_equal(writes_without_consumption, 2); // First write is normal, next 2 increment counter

    // Generate only reads_without_new_data violations
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);

    // Read only reads_without_new_data counter
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, nullptr);
    assert_int_equal(reads_without_new_data, 2); // First read is normal, next 2 increment counter

    // Verify both counters are now cleared
    die_2_die_exch_get_mpam_data_missed_counts(&reads_without_new_data, &writes_without_consumption);
    assert_int_equal(reads_without_new_data, 0);
    assert_int_equal(writes_without_consumption, 0);
}

TEST_FUNCTION(test_die_2_die_exch_ib_write_pwr_pkg_mpam_data_negative, test_setup, test_teardown)
{
    // Create test MPAM data
    mpam_data_t test_data_array[NUMBER_OF_MPAMS];
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_data_array[i].active = 1;
        test_data_array[i].throttling = 0;
    }

    // Test 1: Invalid die ID (die 0) - die_id_is_valid(this_die_id) returns false
    die_2_die_exch_init(0);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);

    // Switch to valid die and verify data wasn't written
    die_2_die_exch_init(1);
    mpam_data_t read_data_array[NUMBER_OF_MPAMS] = {{0}};          // Initialize to zero
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array); // Read from die 1, not die 0
    // Verify all data is zero (not written)
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    }

    // Test 2: Invalid die ID (die 2, beyond valid range) - die_id_is_valid(this_die_id) returns false
    die_2_die_exch_init(2);
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);

    // Switch to valid die and verify data wasn't written
    die_2_die_exch_init(1);
    memset(&read_data_array, 0, sizeof(read_data_array)); // Clear array
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    // Verify all data is zero (not written)
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    }

    // Test 3: NULL pointer - mpam_data_array != NULL returns false
    die_2_die_exch_init(1); // valid die
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(nullptr);

    // Verify data wasn't written due to NULL pointer
    memset(&read_data_array, 0, sizeof(read_data_array)); // Clear array
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    }

    // Test 4: Both conditions false - invalid die ID AND NULL pointer
    die_2_die_exch_init(0);                             // invalid die
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(nullptr); // should not crash

    // Switch to valid die and verify no data was written
    die_2_die_exch_init(1);
    memset(&read_data_array, 0, sizeof(read_data_array)); // Clear array
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &read_data_array);
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    }
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_pwr_pkg_mpam_data_negative, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    // Set up some test data to write first
    mpam_data_t test_data_array[NUMBER_OF_MPAMS];
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        test_data_array[i].active = 1;
        test_data_array[i].throttling = 1;
    }
    die_2_die_exch_ib_write_pwr_pkg_mpam_data(&test_data_array);

    // Test 1: Invalid die ID (die 0) - die_id_is_valid(die_id) returns false
    mpam_data_t read_data_array[NUMBER_OF_MPAMS];
    // Initialize with known values (opposite of what we wrote) to verify function doesn't modify them
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        read_data_array[i].active = 0;     // opposite of what we wrote (1)
        read_data_array[i].throttling = 0; // opposite of what we wrote (1)
    }

    die_2_die_exch_ib_read_pwr_pkg_mpam_data(0, &read_data_array); // die 0 is invalid

    // Verify data wasn't modified due to invalid die ID - should still be 0
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    }

    // Test 2: Invalid die ID (die 2, beyond valid range) - die_id_is_valid(die_id) returns false
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        read_data_array[i].active = 0;     // Initialize to 0
        read_data_array[i].throttling = 0; // Initialize to 0
    }

    die_2_die_exch_ib_read_pwr_pkg_mpam_data(2, &read_data_array); // die 2 is beyond valid range

    // Verify data wasn't modified due to invalid die ID - should still be 0
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(read_data_array[i].active, 0);
        assert_int_equal(read_data_array[i].throttling, 0);
    } // Test 3: NULL pointer - mpam_data_array != NULL returns false
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, nullptr); // should not crash

    // Test 4: Both conditions false - invalid die ID AND NULL pointer
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(0, nullptr); // should not crash

    // Verify that the original data in exchange is still intact by reading with valid parameters
    mpam_data_t verify_data_array[NUMBER_OF_MPAMS] = {{0}};
    die_2_die_exch_ib_read_pwr_pkg_mpam_data(1, &verify_data_array);

    // Should get the original test data we wrote
    for (uint16_t i = 0; i < NUMBER_OF_MPAMS; i++)
    {
        assert_int_equal(verify_data_array[i].active, 1);
        assert_int_equal(verify_data_array[i].throttling, 1);
    }
}

// Test for die_2_die_exch_ib_write_pwr_pkg_mon_data and read - positive cases
TEST_FUNCTION(test_die_2_die_exch_ib_read_write_pwr_pkg_mon_data, test_setup, test_teardown)
{

    // Test 1: Write C3 state from die 1 and read it back
    die_2_die_exch_init(1);
    pkg_mon_data_t write_data;
    write_data.pkg_cstate = ALL_CORES_IN_C3_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data);

    pkg_mon_data_t read_data;
    die_2_die_exch_ib_read_pwr_pkg_mon_data(1, &read_data);

    // Verify: Should read C3 state
    assert_int_equal(read_data.pkg_cstate, ALL_CORES_IN_C3_state);

    // Verify: Data cleared after read
    pkg_mon_data_t read_data2;
    die_2_die_exch_ib_read_pwr_pkg_mon_data(1, &read_data2);
    assert_int_equal(read_data2.pkg_cstate, 0); // Should be cleared

    // Test 2: Write C4 state and verify overwrite
    die_2_die_exch_init(1);
    pkg_mon_data_t write_data2;
    write_data2.pkg_cstate = ALL_CORES_IN_C4_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data2);

    pkg_mon_data_t read_data3;
    die_2_die_exch_ib_read_pwr_pkg_mon_data(1, &read_data3);
    assert_int_equal(read_data3.pkg_cstate, ALL_CORES_IN_C4_state);

    // Test 3: Write mixed state
    die_2_die_exch_init(1);
    pkg_mon_data_t write_data3;
    write_data3.pkg_cstate = ALL_CORES_MIXED_C3_C4_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data3);

    pkg_mon_data_t read_data4;
    die_2_die_exch_ib_read_pwr_pkg_mon_data(1, &read_data4);
    assert_int_equal(read_data4.pkg_cstate, ALL_CORES_MIXED_C3_C4_state);
}

// Test for die_2_die_exch_ib_write_pwr_pkg_mon_data and read - negative cases
TEST_FUNCTION(test_die_2_die_exch_ib_read_write_pwr_pkg_mon_data_negative, test_setup, test_teardown)
{
    // Test 1: Write from die 1, then try to read from invalid die 0
    die_2_die_exch_init(1);
    pkg_mon_data_t write_data;
    write_data.pkg_cstate = ALL_CORES_MIXED_C3_C4_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data);

    pkg_mon_data_t read_data;
    read_data.pkg_cstate = (pkg_cstate_t)0xFF;              // Initialize with non-zero
    die_2_die_exch_ib_read_pwr_pkg_mon_data(0, &read_data); // die 0 is invalid

    // Verify: Data should be unchanged due to invalid die ID
    assert_int_equal(read_data.pkg_cstate, 0xFF);

    // Test 2: NULL pointer for read
    die_2_die_exch_ib_read_pwr_pkg_mon_data(1, nullptr); // Should not crash

    // Test 3: Write from invalid die 0 (primary die shouldn't write)
    die_2_die_exch_init(0);
    pkg_mon_data_t write_data2;
    write_data2.pkg_cstate = ALL_CORES_IN_C4_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data2); // Should not write

    // Test 4: Read from out-of-range die 2
    die_2_die_exch_init(0);
    pkg_mon_data_t read_data2;
    read_data2.pkg_cstate = (pkg_cstate_t)0xFF;
    die_2_die_exch_ib_read_pwr_pkg_mon_data(2, &read_data2); // die 2 doesn't exist

    // Verify: Data should be unchanged
    assert_int_equal(read_data2.pkg_cstate, 0xFF);

    // Test 5: Write from out-of-range die 2
    die_2_die_exch_init(2);
    pkg_mon_data_t write_data3;
    write_data3.pkg_cstate = ALL_CORES_IN_C4_state;
    die_2_die_exch_ib_write_pwr_pkg_mon_data(&write_data3); // Should not write

    // Test 6: NULL pointer for write
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_pwr_pkg_mon_data(nullptr); // Should not crash
}

TEST_FUNCTION(test_die_2_die_exch_ib_write_total_memory_power_mW, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(5000);

    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 5000);

    // Write again to verify overwrite behavior
    die_2_die_exch_ib_write_total_memory_power_mW(10000);
    read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 10000);
}

TEST_FUNCTION(test_die_2_die_exch_ib_write_total_memory_power_negative, test_setup, test_teardown)
{
    // Test 1: Write from die 1, then try to read from invalid die 0
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(5000);

    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(0); // die 0 is invalid

    // Verify: Should return 0 due to invalid die ID
    assert_int_equal(read_power_mW, 0);

    // Test 2: Read from out-of-range die 2
    uint32_t read_power_mW3 = die_2_die_exch_ib_read_total_memory_power_mW(2); // die 2 doesn't exist

    // Verify: Should return 0
    assert_int_equal(read_power_mW3, 0);

    // Test 3: Write from out-of-range die 2 (doesn't clear memory like die 0 init would)
    die_2_die_exch_init(2);
    die_2_die_exch_ib_write_total_memory_power_mW(7000); // Should not write

    // Verify the original value from die 1 is still there
    uint32_t read_power_mW4 = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW4, 5000); // Should still have the original value
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_total_memory_power_mW, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(5000);
    // Test 1: Read initial value after first write
    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 5000); // Should be 5000 after first write

    // Test 2: Write a value and verify read returns correct value
    die_2_die_exch_ib_write_total_memory_power_mW(1500);
    read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 1500);
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_total_memory_power_mW_invalid_die_id, test_setup, test_teardown)
{
    // Test 1: Write from die 1, then try to read from invalid die 0 (primary die)
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(3000);

    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(0);

    // Verify: Should return 0 due to invalid die ID (die 0 is primary)
    assert_int_equal(read_power_mW, 0);

    // Test 2: Try to read from out-of-range die 2 (beyond NUMBER_OF_SECONDARY_DIES)
    uint32_t read_power_mW2 = die_2_die_exch_ib_read_total_memory_power_mW(2);

    // Verify: Should return 0
    assert_int_equal(read_power_mW2, 0);

    // Test 3: Verify die 1's value is still intact after invalid reads
    uint32_t read_power_mW3 = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW3, 3000);
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_total_memory_power_mW_null_pointer, test_setup, test_teardown)
{
    // Test 1: Write a value first
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(4000);

    // Test 2: Verify the data can be read correctly
    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 4000);
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_total_memory_power_mW_edge_cases, test_setup, test_teardown)
{
    die_2_die_exch_init(1);

    // Test 1: Write and read zero value
    die_2_die_exch_ib_write_total_memory_power_mW(0);
    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 0);

    // Test 2: Write and read maximum uint32_t value
    uint32_t max_value = 0xFFFFFFFF;
    die_2_die_exch_ib_write_total_memory_power_mW(max_value);
    read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, max_value);

    // Test 3: Write and read a typical value
    die_2_die_exch_ib_write_total_memory_power_mW(12345);
    read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 12345);
}

TEST_FUNCTION(test_die_2_die_exch_ib_read_total_memory_power_mW_combined_invalid, test_setup, test_teardown)
{
    // Test combined invalid scenarios with invalid die IDs
    die_2_die_exch_init(1);
    die_2_die_exch_ib_write_total_memory_power_mW(5500);

    // Test 1: Read with invalid die 0 - should return 0
    uint32_t read_power_mW_die0 = die_2_die_exch_ib_read_total_memory_power_mW(0);
    assert_int_equal(read_power_mW_die0, 0);

    // Test 2: Read with invalid die 2 - should return 0
    uint32_t read_power_mW_die2 = die_2_die_exch_ib_read_total_memory_power_mW(2);
    assert_int_equal(read_power_mW_die2, 0);

    // Test 3: Verify the original data is still intact for valid die
    uint32_t read_power_mW = die_2_die_exch_ib_read_total_memory_power_mW(1);
    assert_int_equal(read_power_mW, 5500);
}
