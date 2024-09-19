//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddr_manager.cpp
 * Test DDR memory management
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

}
/*-- Symbolic Constant Macros (defines) --*/

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


TEST_FUNCTION(test_ddr_mgr_allocate_mem_for_pwr_report, test_setup, test_teardown)
{
    uintptr_t pkg_location = 0;
    size_t available_size = 0;

    fpfw_status_t status = ddr_manager_allocate_mem_for_pwr_report(&pkg_location, &available_size);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_not_equal(pkg_location, 0);
    assert_int_not_equal(available_size,0);
}

TEST_FUNCTION(test_ddr_mgr_allocate_mem_for_perf_report, test_setup, test_teardown)
{
    uintptr_t pkg_location = 0;
    size_t available_size = 0;

    fpfw_status_t status = ddr_manager_allocate_mem_for_perf_report(&pkg_location, &available_size);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_not_equal(pkg_location, 0);
    assert_int_not_equal(available_size,0);
}