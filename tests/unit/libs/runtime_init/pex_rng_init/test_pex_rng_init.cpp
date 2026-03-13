//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pex_rng_init.cpp
 * PEX RNG init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h>
#include <core_info.h>
#include <corebits.h>
#include <cper.h>
#include <crash_dump.h>
#include <fpfw_init.h>
#include <pex_rng.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void __real_register_pex_error_domain(pex_rng_config_t* pex_config);

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pex_rng;

/*------------- Functions ----------------*/
//
// Mocks
//
corebits_t* __wrap_core_info_get_enable_cores_result(void)
{
    return mock_ptr_type(corebits_t*);
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

void __wrap_register_pex_error_domain(const pex_rng_config_t* rng_config)
{
    check_expected(rng_config);
}

void __wrap_init_pex_rng(const pex_rng_config_t* rng_config, bool is_warm_start)
{
    check_expected(rng_config);
    check_expected(is_warm_start);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}

static UINT mock_tx_timer_create(TX_TIMER* timer_ptr,
                                 CHAR* name_ptr,
                                 VOID (*expiration_function)(ULONG),
                                 ULONG expiration_input,
                                 ULONG initial_ticks,
                                 ULONG reschedule_ticks,
                                 UINT auto_activate)
{
    (void)timer_ptr;
    (void)name_ptr;
    (void)expiration_function;
    (void)expiration_input;
    (void)initial_ticks;
    (void)reschedule_ticks;
    (void)auto_activate;
    return mock_type(UINT);
}

UINT __wrap__tx_timer_create(TX_TIMER* timer_ptr,
                             CHAR* name_ptr,
                             VOID (*expiration_function)(ULONG),
                             ULONG expiration_input,
                             ULONG initial_ticks,
                             ULONG reschedule_ticks,
                             UINT auto_activate)
{
    return mock_tx_timer_create(timer_ptr, name_ptr, expiration_function, expiration_input, initial_ticks, reschedule_ticks, auto_activate);
}

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate)
{
    return mock_tx_timer_create(timer_ptr, name_ptr, expiration_function, expiration_input, initial_ticks, reschedule_ticks, auto_activate);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    (void)p1;
    (void)p2;
    check_expected(errorCode);
    check_expected(p3);
    check_expected(p4);
}

void __wrap_hm_register_error_domain(uint16_t error_domain_idx,
                                     void* error_domain_guid,
                                     void* error_domain_name,
                                     void* err_inject_cb,
                                     void* err_inject_ctx)
{
    check_expected(error_domain_idx);
    FPFW_UNUSED(error_domain_guid);
    FPFW_UNUSED(error_domain_name);
    FPFW_UNUSED(err_inject_cb);
    FPFW_UNUSED(err_inject_ctx);
    function_called();
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}
}

//
// Tests
//
TEST_FUNCTION(test_pex_rng_init_cold_start, nullptr, nullptr)
{
    static const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);

    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_core_info_get_enable_cores_result, &test_platform_cores);
    will_return(__wrap_system_info_is_warm_start, false); // cold start
    expect_any(__wrap_init_pex_rng, rng_config);
    expect_value(__wrap_init_pex_rng, is_warm_start, false);
    expect_any(__wrap_register_pex_error_domain, rng_config);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PEX_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_pex_rng_init_warm_start, nullptr, nullptr)
{
    static const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);

    will_return(__wrap_ift_is_enabled, false); // IFT disabled
    will_return(__wrap_core_info_get_enable_cores_result, &test_platform_cores);
    will_return(__wrap_system_info_is_warm_start, true); // warm start
    // On warm start, init_pex_rng is still called but skips HW RNG enable
    expect_any(__wrap_init_pex_rng, rng_config);
    expect_value(__wrap_init_pex_rng, is_warm_start, true);
    expect_any(__wrap_register_pex_error_domain, rng_config);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PEX_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_pex_rng_init_ift_enabled, nullptr, nullptr)
{
    will_return(__wrap_ift_is_enabled, true); // IFT enabled

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pex_rng.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_register_pex_error_domain_timer_failure, nullptr, nullptr)
{
    pex_rng_config_t config = {0};
    const int32_t timer_error_status = 0x15; // TX_TIMER_ERROR

    will_return(mock_tx_timer_create, (UINT)timer_error_status);
    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_PEX_POLLING_FAILED);
    expect_value(__wrap_crash_dump_bug_check, p3, timer_error_status);
    expect_value(__wrap_crash_dump_bug_check, p4, 100); // POLL_INTERVAL_MS
    expect_value(__wrap_hm_register_error_domain, error_domain_idx, ACPI_ERROR_DOMAIN_PEX);
    expect_function_call(__wrap_hm_register_error_domain);

    __real_register_pex_error_domain(&config);
}

TEST_FUNCTION(test_register_pex_error_domain_success, nullptr, nullptr)
{
    pex_rng_config_t config = {0};
    will_return(mock_tx_timer_create, TX_SUCCESS);
    expect_value(__wrap_hm_register_error_domain, error_domain_idx, ACPI_ERROR_DOMAIN_PEX);
    expect_function_call(__wrap_hm_register_error_domain);

    __real_register_pex_error_domain(&config);
}
