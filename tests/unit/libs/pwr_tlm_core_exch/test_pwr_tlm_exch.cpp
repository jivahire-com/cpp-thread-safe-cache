//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pwr_tlm_exch.cpp
 * Test inter core exchange
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <pwr_tlm_core_exchange.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

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

TEST_FUNCTION(test_droop_counts, test_setup, test_teardown)
{
    uint64_t droop_count_array[NUM_AP_CORES_PER_DIE];
    memset(droop_count_array, 0xFF, sizeof(droop_count_array));

    pwr_tlm_core_exch_scp_write_droop_counts(&droop_count_array);

    pwr_tlm_core_exch_mcp_read_droop_counts(&droop_count_array);

    // stub just clears for now
    for (size_t i = 0; i < NUM_AP_CORES_PER_DIE; ++i)
    {
        assert_int_equal(droop_count_array[i], 0);
    }
}
