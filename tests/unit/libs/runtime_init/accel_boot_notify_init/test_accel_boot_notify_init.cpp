//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_accel_boot_notify.cpp
 * Accel Boot Init Test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <stddef.h>    // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_boot_notify;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_accel_boot_notify_service()
{
    function_called();
}

bool __wrap_system_info_is_warm_start()
{
    function_called();
    return mock_type(bool);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}

//
// Tests
//
TEST_FUNCTION(test_accel_boot_notify, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    expect_function_call(__wrap_system_info_is_warm_start);
    will_return(__wrap_system_info_is_warm_start, 0);
    expect_function_call(__wrap_accel_boot_notify_service);

    // Call API under test
    _fpfw_component_boot_notify.init_fn();
}

TEST_FUNCTION(test_accel_boot_notify_warm_reset, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    expect_function_call(__wrap_system_info_is_warm_start);
    will_return(__wrap_system_info_is_warm_start, 1);

    // Call API under test
    _fpfw_component_boot_notify.init_fn();
}

TEST_FUNCTION(test_accel_boot_notify_ift_enabled, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_ift_is_enabled, true); // IFT enabled

    // Call API under test
    _fpfw_component_boot_notify.init_fn();
}
}