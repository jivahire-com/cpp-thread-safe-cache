//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_out_of_band_interface.cpp
 * Test data processing component out-of-band telemetry interface.
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
#include <die_2_die_exchange_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <string.h> // for memcmp

extern void oob_inf_init(void);
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

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

TEST_FUNCTION(test_oob_inf_init, test_setup, test_teardown)
{
    oob_inf_init();
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_die_temp(5000, 30);
    computed_metrics_oob.max_soc_temp_mov_avg_dC.total_sum = 1000;
    computed_metrics_oob.max_soc_temp_mov_avg_dC.sample_count = 20;
    uint16_t temp_dC = data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC();
    assert_int_equal(temp_dC, 120);
}

TEST_FUNCTION(test_data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC, test_setup, test_teardown)
{
    die_2_die_exch_init(1);
    die_2_die_exch_oob_write_window_max_dimm_temp(5000, 30);
    computed_metrics_oob.max_dimm_temp_mov_avg_dC.total_sum = 1000;
    computed_metrics_oob.max_dimm_temp_mov_avg_dC.sample_count = 20;
    uint16_t temp_dC = data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC();
    assert_int_equal(temp_dC, 120);
}
