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
UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    assert_non_null(timer_ptr);
    check_expected_ptr(name_ptr);
    assert_non_null(expiration_function);
    check_expected(expiration_input);
    check_expected(initial_ticks);
    check_expected(reschedule_ticks);
    check_expected(auto_activate);
    (void)timer_control_block_size; // Not used in this test.

    function_called();

    watchdog_function = expiration_function;

    return mock_type(UINT);
}

void __wrap_wdog_cmsdk_apb_init(uint32_t reload_timeout, bool reset_enable)
{
    check_expected(reload_timeout);
    assert_true(reset_enable);

    function_called();
}

void __wrap_wdog_cmsdk_apb_reload()
{
    function_called();
}

bool __wrap_config_get_wdog_en()
{
    return true;
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

//
// Tests
//
TEST_FUNCTION(test_watchdog_init, nullptr, nullptr)
{
    expect_string(__wrap__txe_timer_create, name_ptr, "wdog_timer");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    expect_function_call(__wrap__txe_timer_create);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_wdog.init_fn();

    // Check expected return value
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);

    // Watchdog timer callback must initialize the watchdog at the first call
    expect_value(__wrap_wdog_cmsdk_apb_init, reload_timeout, 5);
    expect_function_call(__wrap_wdog_cmsdk_apb_init);
    watchdog_function(5);

    // Watchdog timer callback must reload the watchdog at the second call
    expect_function_call(__wrap_wdog_cmsdk_apb_reload);
    watchdog_function(5);
}
}
