//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_utc_sync_manager_init.cpp
 * Test coverage for initializing the UTC Sync Manager
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <tx_api.h>
#include <utc_sync_manager_lib.h>
#include <utc_sync_manager_service.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_SYS_COUNT_FREQ_HZ (1000000000ULL)

#define TEST_MCTP_HANDLE_ADDR (0xDEADBEEF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_utc_mngr_svc_mcp;

// Handles whether a valid pointer is returned for the MCTP context
static bool s_mock_handle_set = true;

/*------------- Functions ----------------*/

//
// Mocks
//

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    if (!s_mock_handle_set)
    {
        return nullptr;
    }
    return (void*)TEST_MCTP_HANDLE_ADDR;
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

void __wrap_utc_sync_manager_mctp_init(fpfw_mctp* mctp_ctx)
{
    FPFW_UNUSED(mctp_ctx);
}

fpfw_status_t __wrap_utc_sync_manager_init(utc_sync_manager_config_t* p_config)
{
    assert_ptr_equal(p_config->time_provider_cb, utc_sync_manager_request_utc_timestamp);
    assert_ptr_equal(p_config->get_current_system_count, gtimer_prodfw_get_counter);
    assert_int_equal(p_config->thread_config.time_slice_option, TX_NO_TIME_SLICE);
    assert_int_equal(p_config->system_counter_freq_hz, TEST_SYS_COUNT_FREQ_HZ);

    check_expected_ptr(p_config);
    return mock_type(fpfw_status_t);
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

TEST_FUNCTION(test_utc_sync_manager_init, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_manager_init, p_config);
    will_return(__wrap_utc_sync_manager_init, FPFW_STATUS_SUCCESS);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_MCTP_INIT_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_MCTP_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_manager_init_no_mctp_handle, nullptr, nullptr)
{
    // When we have no MCTP handle - boot status is not sent since we return early
    s_mock_handle_set = false;

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_manager_init_failed_init, nullptr, nullptr)
{
    s_mock_handle_set = true;

    // Test when utc_sync_manager_init fails - boot status START is sent but not END
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_manager_init, p_config);
    will_return(__wrap_utc_sync_manager_init, FPFW_STATUS_FAIL);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_MCTP_INIT_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == (uint32_t)FPFW_STATUS_FAIL);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_manager_init_die_1, nullptr, nullptr)
{
    // DIE_1 does not send boot status and does not initialize UTC sync manager
    will_return(__wrap_idsw_get_die_id, DIE_1);

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

} // extern "C"