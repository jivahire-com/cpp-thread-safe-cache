//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_systick_update_init.cpp
 * System tick update module init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>      // for idsw_get_platform_sdv
#include <idsw_kng.h>  // for PLATFORM_SVP_SIM, PLATFORM_FPGA_LARGE, PLATFORM_RVP_EVT_SILICON
#include <systick_update.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_systick_upd;
/*------------- Functions ----------------*/
//
// Mocks
//
idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    function_called();

    return mock_type(idsw_plat_id_t);
}

//
// Tests
//
TEST_FUNCTION(test_systick_update_init, nullptr, nullptr)
{
    //
    // Test for SVP
    //
    // Set up expectations
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_function_call(__wrap_idsw_get_platform_sdv);

    // Call API under test
    _fpfw_component_systick_upd.init_fn();

    assert_int_equal(systick_get_emcpu_clock(), 125000000U);

    //
    // Test for FPGA
    //
    // Set up expectations
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    expect_function_call(__wrap_idsw_get_platform_sdv);

    // Call API under test
    _fpfw_component_systick_upd.init_fn();

    assert_int_equal(systick_get_emcpu_clock(), 10000000U);

    //
    // Test for SOC
    //
    // Set up expectations
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call(__wrap_idsw_get_platform_sdv);

    // Call API under test
    _fpfw_component_systick_upd.init_fn();

    assert_int_equal(systick_get_emcpu_clock(), 1000000000U);
}
}
