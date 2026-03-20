//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_watchdog_init.cpp
 * Watchdog Init Test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <idsw.h>
#include <idsw_kng.h>
#include <stddef.h> // for NULL
#include <tx_api.h> // for TX_AUTO_ACTIVATE, TX_SUCCESS

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_wdog;
VOID (*watchdog_function)(ULONG id) = nullptr;

/*------------- Functions ----------------*/
//
// Mocks
//
uint32_t __wrap_wdog_service_init(uint32_t timeout_in_s, uint32_t wdog_freq)
{
    check_expected(timeout_in_s);
    check_expected(wdog_freq);
    function_called();
    return mock_type(uint32_t);
}

bool __wrap_config_get_wdog_en()
{
    return mock_type(bool);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}

//
// Tests
//
TEST_FUNCTION(test_watchdog_init_non_rvp, nullptr, nullptr)
{
    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_config_get_wdog_en, true);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON - 1);
    fpfw_init_result_t result = _fpfw_component_wdog.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_watchdog_init_disabled, nullptr, nullptr)
{
    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_config_get_wdog_en, false);
    fpfw_init_result_t result = _fpfw_component_wdog.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_watchdog_init_rvp, nullptr, nullptr)
{
    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_config_get_wdog_en, true);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

#define MHz 1000000
    expect_value(__wrap_wdog_service_init, timeout_in_s, config_get_wdog_timeout_s());
    expect_value(__wrap_wdog_service_init, wdog_freq, config_get_wdog_counter_freq_Mhz() * MHz);
    expect_function_call(__wrap_wdog_service_init);
    will_return(__wrap_wdog_service_init, TX_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_wdog.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_watchdog_init_ift_enabled, nullptr, nullptr)
{

    will_return(__wrap_ift_is_enabled, true); // IFT enabled

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_wdog.init_fn();

    // Check expected return value
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}
}
