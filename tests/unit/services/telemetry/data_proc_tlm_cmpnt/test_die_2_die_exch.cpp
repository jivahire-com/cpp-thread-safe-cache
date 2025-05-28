//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_die_2_die_exch.cpp
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

    die_2_die_exchange_init(1);
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
    die_2_die_exchange_write_max_die_temp(MAX_DIE_TEMPERATURE_DC);

    uint16_t read_temp_dC = die_2_die_exchange_read_max_die_temp_dC(1); // Read from die 0

    assert_int_equal(read_temp_dC, MAX_DIE_TEMPERATURE_DC); // Verify the read value matches the written value
}

TEST_FUNCTION(test_write_errors, test_setup, test_teardown)
{
    die_2_die_exchange_init(0);
    die_2_die_exchange_write_max_die_temp(MAX_DIE_TEMPERATURE_DC); // die 0 doesn't write to exchange

    die_2_die_exchange_init(2);
    die_2_die_exchange_write_max_die_temp(MAX_DIE_TEMPERATURE_DC); // die 2 doesn't exist
}

TEST_FUNCTION(test_read_errors, test_setup, test_teardown)
{
    uint16_t read_temp_dC = die_2_die_exchange_read_max_die_temp_dC(0); // die 0 doesn't write to exchange
    assert_int_equal(read_temp_dC, 0); // Verify the read value is 0 for die 0

    read_temp_dC = die_2_die_exchange_read_max_die_temp_dC(2); // die 2 doesn't exist
    assert_int_equal(read_temp_dC, 0);                         // Verify the read value is 0 for die 2
}
