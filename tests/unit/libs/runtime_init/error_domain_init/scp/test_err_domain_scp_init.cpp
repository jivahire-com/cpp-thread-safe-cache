//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_err_domain_scp_init.cpp
 * Tests the init of the scp error domain
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <boot_status.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <idsw.h>      // for idsw_get_die_id
#include <idsw_kng.h>  // for KNG_DIE_ID, KNG_PLAT_ID
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_scp_ras;
/*------------- Functions ----------------*/
//
// Mocks
//

void __wrap_register_scp_error_domain()
{
    function_called();
}

void __wrap_register_smmu_error_domain()
{
    function_called();
}

void __wrap_register_gic_error_domain()
{
    function_called();
}

void __wrap_register_ap_wdt_error_domain()
{
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_mcp_error_injection_setup_listener(void* mhu_handle)
{
    FPFW_UNUSED(mhu_handle);
    function_called();
}

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
}

//
// Tests
//

TEST_FUNCTION(scp_ras, nullptr, nullptr)
{
    expect_function_call(__wrap_mcp_error_injection_setup_listener);
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_function_call(__wrap_register_scp_error_domain);
    expect_function_call(__wrap_register_smmu_error_domain);
    expect_function_call(__wrap_register_gic_error_domain);
    expect_function_call(__wrap_register_ap_wdt_error_domain);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_RAS_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_scp_ras.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
