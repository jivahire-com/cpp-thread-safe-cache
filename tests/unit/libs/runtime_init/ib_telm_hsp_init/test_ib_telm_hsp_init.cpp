//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ib_telm_hsp_init.cpp
 * Implement unit tests for in band telemetry hsp initialization
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <boot_status.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_ib_telm_hsp;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);

    return mock_type(fpfw_status_t);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
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

TEST_FUNCTION(test_ib_telm_hsp_init_success_die_0, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_0);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    const auto test_die = (KNG_DIE_ID)0;
    // will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_IB_TELM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_int_equal(result.associated_handle, NULL);
}

TEST_FUNCTION(test_ib_telm_hsp_init_success_die_1, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_1);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_IB_TELM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
    assert_int_equal(result.associated_handle, NULL);
}

TEST_FUNCTION(test_ib_telm_hsp_init_fail, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)DIE_0);
    will_return_always(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);

    fpfw_init_result_t result = _fpfw_component_ib_telm_hsp.init_fn();
    assert_int_equal(result.status, (uint32_t)FPFW_STATUS_FAIL);
    assert_int_equal(result.associated_handle, NULL);
}
}
