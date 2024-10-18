//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddr_manager.cpp
 * Test entry into in band component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <in_band_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

}
/*-- Symbolic Constant Macros (defines) --*/
#define DIE_ID (0)
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

TEST_FUNCTION(test_in_band_cmpnt_init, test_setup, test_teardown)
{
    in_band_tlm_cmpnt_init(DIE_ID);
}

TEST_FUNCTION(test_gen_inst_report, test_setup, test_teardown)
{
    for(uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_report_event_enable[i] = false;
    }

    in_band_tlm_cmpnt_generate_inst_report();
}

TEST_FUNCTION(test_gen_pwr_report, test_setup, test_teardown)
{
    for(uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_report_event_enable[i] = false;
    }

    in_band_tlm_cmpnt_generate_pwr_report();
}



