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

//
// Tests
//

TEST_FUNCTION(test_utc_sync_manager_init, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_manager_init, p_config);
    will_return(__wrap_utc_sync_manager_init, FPFW_STATUS_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_manager_init_failed_init, nullptr, nullptr)
{
    // When we have no MCTP handle
    s_mock_handle_set = false;
    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);

    s_mock_handle_set = true;

    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_manager_init, p_config);
    will_return(__wrap_utc_sync_manager_init, FPFW_STATUS_FAIL);

    result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == (uint32_t)FPFW_STATUS_FAIL);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_manager_init_die_1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_1);

    fpfw_init_result_t result = _fpfw_component_utc_mngr_svc_mcp.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

} // extern "C"