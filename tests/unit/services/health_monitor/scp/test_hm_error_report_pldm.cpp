//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_report_cper.cpp
 * Tests health monitor error report functionality
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <atu_api.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <fpfw_pldm_service.h>
#include <gicd_regs.h>
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <ras.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);

    function_called();

    p_notification->CallBack(FPFW_PLDM_CC_SUCCESS, p_notification->context);

    return mock_type(fpfw_status_t);
}
}

//
// Tests
//
TEST_FUNCTION(test_pldm_from_primary_scp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    hm_cper_transfer_listener_from_scp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_REQ_MCP);
}

TEST_FUNCTION(test_pldm_from_secondary_scp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = false;
    hm_config->is_mcp = true;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);

    hm_cper_transfer_listener_from_scp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_REQ_MCP);
}

TEST_FUNCTION(test_pldm_from_primary_mcp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);

    hm_transfer_cper_to_bmc();
}

TEST_FUNCTION(test_pldm_from_secondary_mcp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = false;
    hm_config->is_mcp = true;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);

    hm_cper_transfer_listener_from_secondary_mcp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP);
}