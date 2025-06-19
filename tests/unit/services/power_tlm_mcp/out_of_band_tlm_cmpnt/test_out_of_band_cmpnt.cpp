//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_out_of_band_cmpnt.cpp
 * Test out of band telemetry component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <out_of_band_tlm_cmpnt.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
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

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_init, test_setup, test_teardown)
{
    out_of_band_tlm_cmpnt_init(0);

    out_of_band_tlm_cmpnt_init(1);
}

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_handle_new_pldm_requests, test_setup, test_teardown)
{
    out_of_band_tlm_cmpnt_handle_new_pldm_requests();
}