//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_domain_mcp.cpp
 * Tests health monitor error injection
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <atu_api.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
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
}

//
// Tests
//
TEST_FUNCTION(hm_mcp_error_domain_register_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_mcp_error_domain_register_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_MCP);
}

TEST_FUNCTION(hm_mcp_error_domain_register_listener_failed, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    hm_mcp_error_domain_register_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_MCP);
}

TEST_FUNCTION(hm_mcp_error_injection_cb, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_MCP_PROC;
    input_einj_payload.component_instance = 0;
    input_einj_payload.status_operation.value = 0;
    input_einj_payload.param_type.error_parameters[0] = 0;
    input_einj_payload.param_type.error_parameters[1] = 0;
    input_einj_payload.value_type.error_values = 0;

    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    for (uint32_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((volatile uint8_t*)einj_payload)[i] = ((const uint8_t*)&input_einj_payload)[i];
    }

    hm_inject_error();
}

TEST_FUNCTION(hm_mcp_error_record_submit_listener, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    expect_value_count(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, -1);
    expect_value_count(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT, -1);
    expect_function_call_any(__wrap_mmio_write32);

    hm_mcp_error_record_submit_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_RECORD_SUBMIT_MCP);
}

TEST_FUNCTION(hm_mcp_error_record_submit_listener_failed, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    hm_mcp_error_record_submit_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_RECORD_SUBMIT_MCP);
}