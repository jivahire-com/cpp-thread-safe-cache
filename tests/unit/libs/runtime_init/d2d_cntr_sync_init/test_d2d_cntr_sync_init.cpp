//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_d2d_cntr_sync_init.cpp
 * d2d_cntr_sync init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_d2d_cntr_sync;

/*------------- Functions ----------------*/

//
// Mocks
//
uint8_t __wrap_idsw_get_cpu_type()
{
    return mock_type(uint8_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_d2d_cntr_sync_init(KNG_DIE_ID die_num, KNG_CPU_TYPE cpu_type)
{
    FPFW_UNUSED(die_num);
    FPFW_UNUSED(cpu_type);
    function_called();
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
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

TEST_FUNCTION(test_d2d_cntr_sync_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expect_function_call(__wrap_d2d_cntr_sync_init);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_D2DCNTR_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_d2d_cntr_sync.init_fn();
}
}
