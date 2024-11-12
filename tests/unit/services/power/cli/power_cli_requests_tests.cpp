#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t
#include <silibs_common.h>

extern "C" {

/**
 * @file power_cli_requests.c
 * Implements the handlers for CLI requests
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h"
#include "power_i.h" // for POWER_LOG_INFO, power_ap_soc_init
#include "power_loops_i.h"
#include "power_runconfig_i.h" // for power_runconfig_get_element, power...

#include <CMockaWrapper.h>  // for expect_value, check_expected_ptr, Cmo...
#include <CMockaWrapper.h>  // for expect_any_always, CmockaWrapperTest
#include <DfwkCommon.h>     // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <DfwkDriver.h>     // for DfwkAsyncRequestComplete, DfwkInte...
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <assert.h>         // for assert
#include <cstddef>          // for NULL, size_t
#include <dvfs.h>           // for dvfs_get_cppc_from_pstate, dvfs_pll_g...
#include <fpfw_status.h>    // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <mock_bug_check.h> // for __wrap_crash_dump_bug_check
#include <power_dfwk.h>     // for (anonymous), ppower_service_cli_re...
#include <stdbool.h>        // for false
#include <stddef.h>

} // extern "C"

power_exec_in_idle_handler_t s_saved_idle_handler = NULL;
power_cap_completed_callback_t s_saved_power_cap_completed_callback = NULL;

//
// Mocks
//
extern "C" {

void __wrap_power_loops_exec_in_idle(power_exec_in_idle_handler_t p_handler, PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    check_expected(p_request);
    check_expected(p_context);
    s_saved_idle_handler = p_handler;
}

int __wrap_power_cap_update(power_cap_completed_callback_t callback, uint16_t new_power_cap, bool source_is_cli)
{

    s_saved_power_cap_completed_callback = callback;
    check_expected(new_power_cap);
    check_expected(source_is_cli);
    return mock_type(int);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}

// wrap for power_runconfig_get
power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}

} // extern "C"

POWER_TEST(async_handler, NULL, NULL)
{
#define TEST_PREQUEST 0x12345678
#define TEST_PCONTEXT 0x98765432

    expect_value(__wrap_power_loops_exec_in_idle, p_request, TEST_PREQUEST);

    expect_value(__wrap_power_loops_exec_in_idle, p_context, TEST_PCONTEXT);

    power_cli_requests_async_handler((PDFWK_ASYNC_REQUEST_HEADER)TEST_PREQUEST, (void*)TEST_PCONTEXT);
}

POWER_TEST(handler_power_set_cap, NULL, NULL)
{

#define CAP_VALUE    10
#define CLI_BOOL     1
#define CAP_PREVIOUS 300
#define RESULT       0x1234

    const corebits_t default_cores = (const corebits_t)COREBITS_INIT_UINT32(0xFF, 0xFFFFFFFF, 0xF);
    power_service_config_t test_config = {
        .platform_cores_in_die = &default_cores,
        .platform_die_core_count = 1,
        .platform_core_power_support = true,
    };
    power_runconfig_t test_runconfig = {.p_sconfig = &test_config};

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // we have to create an actual request to pass into the handler
    power_service_cli_request_t request = {.header = {.RequestType = CLI_COMMANDS_POWER_SET}};

    request.power_ext_if_cmd_id = POWER_IF_CMD_SET_CAP;
    request.sub_command_args.pwrset_sub_command_args.cap_val = CAP_VALUE;

    expect_value(__wrap_power_cap_update, new_power_cap, CAP_VALUE);
    expect_value(__wrap_power_cap_update, source_is_cli, CLI_BOOL);
    will_return(__wrap_power_cap_update, MP_POWER_CAP_PENDING);

    // call the power_cli_requests_callback with request for a set power cap
    assert_non_null(s_saved_idle_handler);
    s_saved_idle_handler(&request.header, NULL);

    // assuming the test runs to here, power_cap_update wrapper should have been called and saved the callback
    // let's test the callback
    assert_non_null(s_saved_power_cap_completed_callback);

    // set expectations
    // callback should complete the request we passed in earlier
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &request.header);
    // call the saved callback with our test results
    s_saved_power_cap_completed_callback(RESULT, CAP_VALUE, CAP_PREVIOUS);

    // ensure response updated as expected
    assert_int_equal(request.fetch_data.pwrset_response_val.pwr_icc_cap_result.current_cap, CAP_VALUE);
    assert_int_equal(request.fetch_data.pwrset_response_val.pwr_icc_cap_result.previous_cap, CAP_PREVIOUS);
    assert_int_equal(request.fetch_data.pwrset_response_val.pwr_icc_cap_result.result, RESULT);
}
