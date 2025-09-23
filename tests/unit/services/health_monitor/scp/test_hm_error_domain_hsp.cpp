//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_domain_hsp.cpp
 * Tests health monitor hsp error domain
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
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
TEST_FUNCTION(hm_hsp_error_record_submit_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_hsp_error_record_submit_listener((fpfw_icc_base_ctx_t*)HSP_MAILBOX_CMD_RAS_ERROR_REPORT_REQ);
}

TEST_FUNCTION(hm_hsp_error_domain_register_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_hsp_error_domain_register_listener((fpfw_icc_base_ctx_t*)HSP_MAILBOX_CMD_RAS_ERROR_REGISTER_REQ);
}

TEST_FUNCTION(hm_hsp_error_injection_cb, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_recv);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_HSP_PROC;
    input_einj_payload.component_instance = 0;
    input_einj_payload.status_operation.value = 0;
    input_einj_payload.param_type.error_parameters[0] = 0;
    input_einj_payload.param_type.error_parameters[1] = 0;
    input_einj_payload.value_type.error_values = 0;

    hm_map_error_injection_payload();
    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    for (uint32_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((volatile uint8_t*)einj_payload)[i] = ((const uint8_t*)&input_einj_payload)[i];
    }
    hm_unmap_error_injection_payload();

    hm_inject_error();
}