//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_injection.cpp
 * Tests health monitor error injection
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern acpi_error_domain_t test_error_domain;
extern char* TEST_ERROR_DOMAIN_NAME;
extern ras_einj_info_t einj_payload_local;
/*------------- Functions ----------------*/

//
// Mocks
//
uint8_t __wrap_idsw_get_die_id()
{
    return (mock_type(uint8_t));
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(params);
    function_called();

    return (mock_type(fpfw_status_t));
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(params);
    function_called();

    return (mock_type(fpfw_status_t));
}
}

//
// Tests
//
TEST_FUNCTION(test_hm_inject_error, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(hm_error_injection_cb);
    will_return_always(__wrap_idsw_get_die_id, 0);

    hm_register_error_domain((uint16_t)test_error_domain, NULL, TEST_ERROR_DOMAIN_NAME, hm_error_injection_cb, nullptr);

    hm_config_t* hm_config = get_hm_config();

    ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;
    assert_true(einj_payload->version == ERROR_INJECTION_PAYLOAD_VERSION);
    assert_true(einj_payload->component_group == test_error_domain);
    assert_true(hm_inject_error() == ACPI_EINJ_SUCCESS);
}

TEST_FUNCTION(test_hm_inject_error_remote, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, 1);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);

    assert_true(hm_inject_error() == ACPI_EINJ_SUCCESS);

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(hm_error_injection_cb);
    hm_inject_error_recv_cb(NULL, 0, 0);
}