//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mts_init.cpp
 * ATU init tests
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h>                  // for idsw_get_die_id
#include <idsw_kng.h>              // for KNG_DIE_ID, KNG_PLAT_ID
#include <pwr_tlm_core_exchange.h> // for ACCEL_ID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pwr_tlm_core_ex;

/*------------- Functions ----------------*/
void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

//
// Tests
//
TEST_FUNCTION(test_pwr_tlm_core_exch_init, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_POWER_TLM_CORE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_tlm_core_ex.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}
