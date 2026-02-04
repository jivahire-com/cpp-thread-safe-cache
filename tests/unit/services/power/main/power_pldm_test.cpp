//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm_test.cpp
 * Power service PLDM tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h> // for expect_any_always, CmockaWrapperTest
#include <assert.h>        // for assert
#include <cstddef>         // for NULL, size_t
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t, fpfw_icc_base_recv_req_t
#include <fpfw_status.h>   // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <idsw_kng.h>
#include <mock_bug_check.h>    // for __wrap_crash_dump_bug_check
#include <pldm_common_power.h> // for icc_pwr_cap_request_t, icc_pwr_throt_state_request_t
#include <power_i.h>           // for power function prototypes
#include <power_pldm_scp.h>    // for handle_performance_state_request, handle_power_cap_request
#include <power_runconfig.h>   // for power_runconfig_t
#include <power_runconfig_i.h> // for power_runconfig_t internals
#include <stdint.h>            // for int8_t, uint64_t, uint8_t, uintptr_t
#include <string.h>            // for memcpy
#include <warm_start_id.h>     // for WARM_START_ID_POWER_FUSE

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_POWER_CAP_NORMAL 200
#define TEST_POWER_CAP_HIGH   450 // Use a very high value that would exceed typical limits
#define TEST_TDP_UPPER_LIMIT  400 // Typically 400W limit for SOC power cap

// Constants from power_runconfig.h enum
#define MP_POWER_CAP_SUCCESS               0
#define MP_POWER_CAP_PENDING               1
#define MP_POWER_CAP_FAIL_PREVIOUS_UPDATED 2
#define MP_POWER_CAP_FAIL_CLI_NOT_ALLOWED  3
#define MP_POWER_CAP_FAIL_INVALID_REQUEST  4

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint32_t dummy_icc_base_ctx = 0; //! dummy context for icc_mscp_ctx
static power_runconfig_t test_power_runconfig = {};
static power_service_config_t test_power_service_config = {
    .icc_mscp_ctx = &dummy_icc_base_ctx,
    .is_primary_die = true,
};
static power_latest_calcs_t test_calcs = {.soc_power = 100.0F, .vcpu_power = 50.0F, .vcpu_avs_voltage = 1000, .vcpu_avs_current = 1800};

/*------------- Functions ----------------*/
//
// Mocks
//

// Mock for get_current_soc_power_cap
uint16_t __wrap_get_current_soc_power_cap(void)
{
    return (uint16_t)mock();
}

// Mock for power_control_loop_throttled
bool __wrap_power_control_loop_throttled(void)
{
    return (bool)mock();
}

// Mock for power_control_loop_degraded
bool __wrap_power_control_loop_degraded(void)
{
    return (bool)mock();
}

// Mock for power_cap_update
int __wrap_power_cap_update(void* callback, uint16_t power_cap, bool force)
{
    check_expected_ptr(callback);
    check_expected(power_cap);
    check_expected(force);
    return (int)mock();
}

// Mock for fpfw_icc_base_send_resp
fpfw_status_t __wrap_fpfw_icc_base_send_resp(fpfw_icc_base_ctx_t* ctx, fpfw_icc_base_send_rsp_t* param)
{
    assert_non_null(ctx);
    assert_non_null(param);
    return (fpfw_status_t)mock();
}

// // Mock for FPFwErrorRaise - needed to handle FPFW_RUNTIME_ASSERT_EXT calls
// void __wrap_FPFwErrorRaise(uint32_t error, const char* file, uint32_t line, const char* func, const char* msg)
// {
//     FPFW_UNUSED(error);
//     FPFW_UNUSED(file);
//     FPFW_UNUSED(line);
//     FPFW_UNUSED(func);
//     FPFW_UNUSED(msg);
//     // Accept any error code, don't check it since the assertion logic varies
//     // Just consume the call to prevent test failures
//     function_called();
// }

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* ctx, fpfw_icc_base_recv_req_t* param)
{
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(param);
    return mock_type(fpfw_status_t);
}

// void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
// {
//     FPFW_UNUSED(arg0);
//     FPFW_UNUSED(arg1);
//     FPFW_UNUSED(arg2);
//     FPFW_UNUSED(arg3);
//     check_expected(expression);
// }

} // extern "C"

static int setup(void** state)
{
    FPFW_UNUSED(state);
    // Set up test power runconfig with proper values
    test_power_runconfig.p_sconfig = &test_power_service_config;
    test_power_runconfig.derived.soc_maximum_thermal_watts_limit = TEST_TDP_UPPER_LIMIT;

    //! Setup expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS, 2);

    //! Call setup function for power_pldm_service_init
    power_pldm_service_init(&test_power_runconfig);
    //! Initialize power cap module
    power_cap_init();

    return 0;
}

static int teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

/*------------- Test Cases ----------------*/

//
// Tests for handle_power_cap_request
//

POWER_TEST(handle_power_cap_request__valid_cap_same_as_current__no_update, setup, teardown)
{
    icc_pwr_cap_request_t test_request = {};
    // Use NO_POWER_CAP as the request since that's what get_current_soc_power_cap returns by default
    test_request.oob_input.power_cap = NO_POWER_CAP;

    // Mock get_current_soc_power_cap to return NO_POWER_CAP (default behavior)
    will_return(__wrap_get_current_soc_power_cap, NO_POWER_CAP);

    // When current cap equals requested cap, no power_cap_update call is made
    // Instead, power_cap_completed_callback is called directly
    // Mock fpfw_icc_base_send_resp, called inside power_cap_completed_callback
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Function under test
    assert_int_equal(handle_power_cap_request(&test_request), NO_POWER_CAP);
}

POWER_TEST(handle_power_cap_request__valid_cap_different_from_current__update_pending, setup, teardown)
{
    icc_pwr_cap_request_t test_request = {};
    test_request.oob_input.power_cap = TEST_POWER_CAP_NORMAL;

    // Mock get_current_soc_power_cap to return NO_POWER_CAP (default behavior)
    will_return(__wrap_get_current_soc_power_cap, NO_POWER_CAP);

    // Function under test
    assert_int_equal(handle_power_cap_request(&test_request), TEST_POWER_CAP_NORMAL);

    // Optional, Simulate pwr control loop operations to get current vcpu cap & finalize cap update
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);
    power_cap_get_vrcpu_cap(NULL, &test_calcs, &test_calcs);
    // Mock fpfw_icc_base_send_resp, called inside power_cap_completed_callback
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);
    // power_cap_finalize should save to warm start since cap changed
    expect_function_call(__wrap_power_ws_save_pwr_cap);
    power_cap_finalize();
}

POWER_TEST(handle_power_cap_request__cap_above_limit__clamped_to_limit, setup, teardown)
{
    icc_pwr_cap_request_t test_request = {};
    test_request.oob_input.power_cap = TEST_POWER_CAP_HIGH;

    // Mock get_current_soc_power_cap to return NO_POWER_CAP (default behavior)
    will_return(__wrap_get_current_soc_power_cap, NO_POWER_CAP);

    // Function under test, verify clamping occurs
    assert_int_equal(handle_power_cap_request(&test_request), TEST_TDP_UPPER_LIMIT);
}

POWER_TEST(handle_power_cap_request__no_power_cap_requested__no_clamping, setup, teardown)
{
    icc_pwr_cap_request_t test_request = {};
    test_request.oob_input.power_cap = NO_POWER_CAP; // Special value to disable capping

    // Mock get_current_soc_power_cap to return a different value so update is triggered
    will_return(__wrap_get_current_soc_power_cap, TEST_POWER_CAP_NORMAL);

    // Function under test, verify no clamping occurs
    assert_int_equal(handle_power_cap_request(&test_request), NO_POWER_CAP);
}

// Tests for handle_performance_state_request
POWER_TEST(handle_performance_state_request__normal_state__not_throttled_not_degraded, setup, teardown)
{
    icc_pwr_throt_state_request_t test_request = {};

    // Mock power control loop functions to return normal state
    will_return(__wrap_power_control_loop_throttled, false);
    will_return(__wrap_power_control_loop_degraded, false);
    // Mock fpfw_icc_base_send_resp
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Function under test
    assert_int_equal(handle_performance_state_request(&test_request), PLDM_PERFORMANCE_NORMAL);
}

POWER_TEST(handle_performance_state_request__throttled_state__throttled_not_degraded, setup, teardown)
{
    icc_pwr_throt_state_request_t test_request = {};

    // Mock power control loop functions to return throttled state
    will_return(__wrap_power_control_loop_throttled, true);
    will_return(__wrap_power_control_loop_degraded, false);
    // Mock fpfw_icc_base_send_resp
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Function under test
    assert_int_equal(handle_performance_state_request(&test_request), PLDM_PERFORMANCE_THROTTLED);
}

POWER_TEST(handle_performance_state_request__degraded_state__not_throttled_degraded, setup, teardown)
{
    icc_pwr_throt_state_request_t test_request = {};

    // Mock power control loop functions to return degraded state
    will_return(__wrap_power_control_loop_throttled, false);
    will_return(__wrap_power_control_loop_degraded, true);
    // Mock fpfw_icc_base_send_resp
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Function under test
    assert_int_equal(handle_performance_state_request(&test_request), PLDM_PERFORMANCE_DEGRADED);
}

POWER_TEST(handle_performance_state_request__both_throttled_and_degraded__both_flags_set, setup, teardown)
{
    icc_pwr_throt_state_request_t test_request = {};

    // Mock power control loop functions to return both throttled and degraded
    will_return(__wrap_power_control_loop_throttled, true);
    will_return(__wrap_power_control_loop_degraded, true);
    // Mock fpfw_icc_base_send_resp
    will_return(__wrap_fpfw_icc_base_send_resp, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Function under test
    assert_int_equal(handle_performance_state_request(&test_request), PLDM_PERFORMANCE_THROTTLED_DEGRADED);
}
