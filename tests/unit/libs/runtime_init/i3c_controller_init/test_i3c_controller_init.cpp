//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_i3c_controller_init.cpp
 * i3c_controller init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_i3c_controller;

/*------------- Functions ----------------*/

//
// Mocks
//

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_i3c_controller(uint8_t die_num)
{
    check_expected(die_num);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

//
// Tests
//

TEST_FUNCTION(test_i3c_controller_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);

    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_I3C_INIT_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    expect_value(__wrap_i3c_controller, die_num, test_die);

    expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_I3C_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);
    // Call API under test
    _fpfw_component_i3c_controller.init_fn();
}
}
