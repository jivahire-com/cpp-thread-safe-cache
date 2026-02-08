//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_utc_sync_client_init.cpp
 * Test coverage for initializing the UTC Sync Client
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <hsp_firmware_headers.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <tx_api.h>
#include <utc_sync_client_service.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_SYS_COUNT_FREQ_HZ (1000000000ULL)
#define TEST_HANDLE_ADDR       (0xDEADBEEF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_utc_client_svc;
extern fpfw_init_component_t _fpfw_component_utc_cli;

static utc_sync_client_config_t s_captured_config = {};

/*------------- Functions ----------------*/

//
// Mocks
//

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    check_expected(icc_ctx);
    check_expected(payload_buffer);
    check_expected(buffer_size);
    return mock_type(fpfw_status_t);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return (void*)TEST_HANDLE_ADDR;
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

fpfw_status_t __wrap_utc_sync_client_init(utc_sync_client_config_t* p_config)
{
    check_expected(p_config->get_current_system_count);
    check_expected(p_config->thread_config.time_slice_option);
    check_expected(p_config->system_counter_freq_hz);

    check_expected_ptr(p_config);

    s_captured_config = *p_config;

    return mock_type(fpfw_status_t);
}

void __wrap_utc_cli_svc_initialize(void)
{
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

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

//
// Tests
//

TEST_FUNCTION(test_utc_sync_client_init, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    // Setup the expectations in the same order the mock checks them
    expect_any(__wrap_utc_sync_client_init, p_config);
    expect_value(__wrap_utc_sync_client_init, p_config->get_current_system_count, gtimer_prodfw_get_counter);
    expect_value(__wrap_utc_sync_client_init, p_config->thread_config.time_slice_option, TX_NO_TIME_SLICE);
    expect_value(__wrap_utc_sync_client_init, p_config->system_counter_freq_hz, TEST_SYS_COUNT_FREQ_HZ);
    will_return(__wrap_utc_sync_client_init, FPFW_STATUS_SUCCESS);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_UTC_SYNC_INIT_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_UTC_SYNC_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_utc_client_svc.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);

    // Test the plaform handler callback and context were set correctly
    assert_non_null(s_captured_config.platform_handler.cb);
    assert_int_equal((uintptr_t)s_captured_config.platform_handler.ctx, TEST_HANDLE_ADDR);

    // Call the handler and verify it sends an icc message
    utc_timestamp_bundle_t test_bundle = {
        .timestamp = 0x1122334455667788ULL,
        .system_counter = 0x99AABBCCDDEEFF00ULL,
    };
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, (void*)TEST_HANDLE_ADDR);
    expect_any(__wrap_fpfw_icc_base_send_sync, payload_buffer);
    expect_value(__wrap_fpfw_icc_base_send_sync, buffer_size, sizeof(kng_hsp_mailbox_msg));
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    s_captured_config.platform_handler.cb(&test_bundle, s_captured_config.platform_handler.ctx);
}

TEST_FUNCTION(test_utc_sync_client_init_failed_init, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_client_init, p_config);
    expect_value(__wrap_utc_sync_client_init, p_config->get_current_system_count, gtimer_prodfw_get_counter);
    expect_value(__wrap_utc_sync_client_init, p_config->thread_config.time_slice_option, TX_NO_TIME_SLICE);
    expect_value(__wrap_utc_sync_client_init, p_config->system_counter_freq_hz, TEST_SYS_COUNT_FREQ_HZ);
    will_return(__wrap_utc_sync_client_init, FPFW_STATUS_FAIL);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, (test_die == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_UTC_SYNC_INIT_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, (test_die == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_UTC_SYNC_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_utc_client_svc.init_fn();

    assert_true(result.status == (uint32_t)FPFW_INIT_STATUS_E_POINTER);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_cli_init, nullptr, nullptr)
{
    expect_function_call(__wrap_utc_cli_svc_initialize);

    fpfw_init_result_t result = _fpfw_component_utc_cli.init_fn();

    assert_true(result.status == (uint32_t)FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

} // extern "C"