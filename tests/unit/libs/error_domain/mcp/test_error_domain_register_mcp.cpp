//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_startup_shutdown_init.cpp
 * Tests the init of the sos service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_icc.h>
#include <mscp_error_domain.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    FPFW_UNUSED(icc_ctx);

    assert_true(params != NULL);
    assert_true(params->payload_buffer != NULL);
    assert_true(params->buffer_size == sizeof(hm_mhu_error_domain_register_payload_t));

    params->cb(params, FPFW_ICC_BASE_STATUS_SUCCESS);

    function_called();

    return (mock_type(fpfw_status_t));
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    assert_true(params != NULL);
    assert_true(params->payload_buffer != NULL);
    assert_true(params->recv_cmd_code == ICC_HM_ERROR_INJECTION_MCP);

    static bool mcp_einj_tested = false;
    if (params->recv_cmd_code == ICC_HM_ERROR_INJECTION_MCP && mcp_einj_tested == false)
    {
        mcp_einj_tested = true;
        hm_mhu_error_injection_payload_t einj_payload;
        einj_payload.header.msg_header.command = params->recv_cmd_code;
        params->cb(&einj_payload, sizeof(einj_payload), FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    function_called();
    return (mock_type(fpfw_status_t));
}
}

//
// Tests
//

TEST_FUNCTION(test_register_mcp_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    register_mcp_error_domain((fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_start_mcp_error_injection_listener, nullptr, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    start_mcp_error_injection_listener((fpfw_icc_base_ctx_t*)1234);
}