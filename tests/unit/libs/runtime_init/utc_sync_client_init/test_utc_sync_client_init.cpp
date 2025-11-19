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
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <gtimer_prodfw.h>
#include <tx_api.h>
#include <utc_sync_client_service.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_SYS_COUNT_FREQ_HZ (1000000000ULL)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_utc_client_svc;

/*------------- Functions ----------------*/

//
// Mocks
//

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
    return mock_type(fpfw_status_t);
}

//
// Tests
//

TEST_FUNCTION(test_utc_sync_client_init, nullptr, nullptr)
{
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    // Setup the expectations in the same order the mock checks them
    expect_any(__wrap_utc_sync_client_init, p_config);
    expect_value(__wrap_utc_sync_client_init, p_config->get_current_system_count, gtimer_prodfw_get_counter);
    expect_value(__wrap_utc_sync_client_init, p_config->thread_config.time_slice_option, TX_NO_TIME_SLICE);
    expect_value(__wrap_utc_sync_client_init, p_config->system_counter_freq_hz, TEST_SYS_COUNT_FREQ_HZ);
    will_return(__wrap_utc_sync_client_init, FPFW_STATUS_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_utc_client_svc.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_true(result.associated_handle == NULL);
}

TEST_FUNCTION(test_utc_sync_client_init_failed_init, nullptr, nullptr)
{
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);

    expect_any(__wrap_utc_sync_client_init, p_config);
    expect_value(__wrap_utc_sync_client_init, p_config->get_current_system_count, gtimer_prodfw_get_counter);
    expect_value(__wrap_utc_sync_client_init, p_config->thread_config.time_slice_option, TX_NO_TIME_SLICE);
    expect_value(__wrap_utc_sync_client_init, p_config->system_counter_freq_hz, TEST_SYS_COUNT_FREQ_HZ);
    will_return(__wrap_utc_sync_client_init, FPFW_STATUS_FAIL);

    fpfw_init_result_t result = _fpfw_component_utc_client_svc.init_fn();

    assert_true(result.status == (uint32_t)FPFW_INIT_STATUS_E_POINTER);
    assert_true(result.associated_handle == NULL);
}

} // extern "C"