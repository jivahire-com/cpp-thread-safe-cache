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

    uint16_t avg_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, 0);
    assert_int_equal(avg_temp_dC, 20);

    uint16_t max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, 0);
    assert_int_equal(max_temp_dC, 30);

    uint16_t avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, 0);
    assert_int_equal(avg_pwr_mW, 1000);

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