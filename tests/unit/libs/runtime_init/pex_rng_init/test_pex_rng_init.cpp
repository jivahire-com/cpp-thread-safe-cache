//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pex_rng_init.cpp
 * PEX RNG init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <core_info.h>
#include <corebits.h>
#include <fpfw_init.h>
#include <pex_rng.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pex_rng;

/*------------- Functions ----------------*/
//
// Mocks
//
corebits_t* __wrap_core_info_get_enable_cores_result(void)
{
    return mock_ptr_type(corebits_t*);
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

void __wrap_register_pex_error_domain(const pex_rng_config_t* rng_config)
{
    check_expected(rng_config);
}

void __wrap_init_pex_rng(const pex_rng_config_t* rng_config)
{
    check_expected(rng_config);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}
}

//
// Tests
//
TEST_FUNCTION(test_pex_rng_init_cold_start, nullptr, nullptr)
{
    static const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);

    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_core_info_get_enable_cores_result, &test_platform_cores);
    will_return(__wrap_system_info_is_warm_start, false); // cold start
    expect_any(__wrap_init_pex_rng, rng_config);
    expect_any(__wrap_register_pex_error_domain, rng_config);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_pex_rng_init_warm_start, nullptr, nullptr)
{
    static const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);

    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_core_info_get_enable_cores_result, &test_platform_cores);
    will_return(__wrap_system_info_is_warm_start, true); // warm start
    // On warm start, skip init_pex_rng
    expect_any(__wrap_register_pex_error_domain, rng_config);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_pex_rng_init_ift_enabled, nullptr, nullptr)
{
    will_return(__wrap_ift_is_enabled, true); // IFT enabled

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
