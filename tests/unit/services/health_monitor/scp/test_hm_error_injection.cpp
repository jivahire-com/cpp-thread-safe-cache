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
#include <accelip_id.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <hm_test.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <stdint.h>
#include <string.h>

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

    if (params->cb != NULL)
    {
        params->cb(params->cb_ctx, FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    return (mock_type(fpfw_status_t));
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(params);
    function_called();

    static bool mcp_register_tested = false;
    static bool hsp_register_tested = false;
    static bool mcp_cper_tested = false;
    static bool hsp_cper_tested = false;
    static bool apcore_listener_tested = false;

    if (icc_ctx == (fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_MCP && mcp_register_tested == false)
    {
        mcp_register_tested = true;

        static hm_mhu_error_domain_register_payload_t mcp_err_domain_register_payload;
        mcp_err_domain_register_payload.header.msg_header.command = ICC_HM_ERROR_DOMAIN_REGISTER_MCP;
        mcp_err_domain_register_payload.header.msg_header.payload_size =
            sizeof(hm_mhu_error_domain_register_payload_t) - sizeof(icc_mhu_header_t);
        mcp_err_domain_register_payload.error_domain_idx = ACPI_ERROR_DOMAIN_MCP_PROC;
        mcp_err_domain_register_payload.valid_fru_id = 1;
        mcp_err_domain_register_payload.valid_fru_text = 1;
        mcp_err_domain_register_payload.fru_id = (guid_t)MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
        strncpy(mcp_err_domain_register_payload.fru_text, MCP_PROC_FRU, sizeof(mcp_err_domain_register_payload.fru_text) - 1);
        mcp_err_domain_register_payload.fru_text[sizeof(mcp_err_domain_register_payload.fru_text) - 1] = '\0';

        params->cb(&mcp_err_domain_register_payload, sizeof(hm_mhu_error_domain_register_payload_t), FPFW_STATUS_SUCCESS);
    }
    else if (icc_ctx == (fpfw_icc_base_ctx_t*)HSP_MAILBOX_CMD_RAS_ERROR_REGISTER_REQ && hsp_register_tested == false)
    {
        hsp_register_tested = true;

        hm_config_t* hm_config = get_hm_config();
        hm_error_domain_info_t* hsp_err_domain = (hm_error_domain_info_t*)(hm_config->mscp_hsp_ras_payload_base);

        hsp_err_domain->error_domain_idx = ACPI_ERROR_DOMAIN_HSP_PROC;
        hsp_err_domain->valid_fru_id = 0;
        hsp_err_domain->valid_fru_str = 1;
        strncpy(hsp_err_domain->fru_text, HSP_PROC_FRU, sizeof(hsp_err_domain->fru_text));

        struct kng_hsp_mailbox_cmd_ras_error_register_req hsp_err_domain_register_payload;
        hsp_err_domain_register_payload.error_domain_payload_address_low = (uint32_t)hsp_err_domain;

        static fpfw_icc_base_recv_req_t hsp_err_domain_register_icc_payload;
        hsp_err_domain_register_icc_payload.payload_buffer = &hsp_err_domain_register_payload;

        params->cb(&hsp_err_domain_register_icc_payload, sizeof(fpfw_icc_base_recv_req_t), FPFW_STATUS_SUCCESS);
    }
    else if (icc_ctx == (fpfw_icc_base_ctx_t*)ICC_HM_ERROR_RECORD_SUBMIT_MCP && mcp_cper_tested == false)
    {
        mcp_cper_tested = true;

        static hm_mhu_error_record_payload_t mcp_cper_payload;
        mcp_cper_payload.header.msg_header.command = ICC_HM_ERROR_RECORD_SUBMIT_MCP;
        mcp_cper_payload.header.msg_header.payload_size = sizeof(hm_mhu_error_record_payload_t) - sizeof(icc_mhu_header_t);
        mcp_cper_payload.error_record.error_domain_idx = ACPI_ERROR_DOMAIN_MCP_PROC;
        mcp_cper_payload.error_record.section_size = sizeof(acpi_cper_section_t);

        params->cb(&mcp_cper_payload, sizeof(mcp_cper_payload), FPFW_STATUS_SUCCESS);
    }
    else if (icc_ctx == (fpfw_icc_base_ctx_t*)HSP_MAILBOX_CMD_RAS_ERROR_REPORT_REQ && hsp_cper_tested == false)
    {
        hsp_cper_tested = true;

        hm_config_t* hm_config = get_hm_config();
        hm_error_record_t* hsp_err_record =
            (hm_error_record_t*)(hm_config->mscp_hsp_ras_payload_base + HM_HSP_ERROR_RECORD_OFFSET);
        hsp_err_record->error_domain_idx = ACPI_ERROR_DOMAIN_HSP_PROC;
        hsp_err_record->section_size = sizeof(acpi_err_sec_hsp_processor_t);
        hsp_err_record->err_severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
        hsp_err_record->cper_section.sec_hsp_proc.error_type = 1;

        struct kng_hsp_mailbox_cmd_ras_error_report_req hsp_err_report_payload;
        hsp_err_report_payload.cper_payload_address_low = (uint32_t)hsp_err_record;

        static fpfw_icc_base_recv_req_t hsp_err_submit_icc_payload;
        hsp_err_submit_icc_payload.payload_buffer = &hsp_err_report_payload;

        params->cb(&hsp_err_submit_icc_payload, sizeof(fpfw_icc_base_recv_req_t), FPFW_STATUS_SUCCESS);
    }
    else if (icc_ctx == (fpfw_icc_base_ctx_t*)ICC_HM_ERROR_INJECTION_AP2SCP && apcore_listener_tested == false)
    {
        apcore_listener_tested = true;
        params->cb(NULL, 0, FPFW_STATUS_SUCCESS);
    }
    else
    {
        uint32_t cmd_code = (uint32_t)icc_ctx;
        switch (cmd_code)
        {
        case ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM):
        case ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED):
        case ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_SDM):
        case ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED):
            custom_fpfw_recv(params);
            break;
        case ICC_HM_ERROR_INJECTION_ACCEL(ACCEL_ID_SDM):
        case ICC_HM_ERROR_INJECTION_ACCEL(ACCEL_ID_CDED):
        case ICC_HM_TX_DONE_ACK_ACCEL(ACCEL_ID_SDM):
        case ICC_HM_TX_DONE_ACK_ACCEL(ACCEL_ID_CDED):
        default:
            break;
        }
    }

    return (mock_type(fpfw_status_t));
}

fpfw_status_t __wrap_fpfw_icc_base_send_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    function_called();

    params->cb(params->cb_ctx, sizeof(kng_hsp_mailbox_msg_rsp), FPFW_STATUS_SUCCESS);
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
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);

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
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);

    assert_true(hm_inject_error() == ACPI_EINJ_SUCCESS);

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(hm_error_injection_cb);
    hm_inject_error_recv_cb(NULL, 0, 0);
}

TEST_FUNCTION(test_hm_inject_error_singledie, post_ddr_setup, nullptr)
{

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return_always(__wrap_idsw_get_die_id, 1);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);

    hm_register_error_domain((uint16_t)test_error_domain, NULL, TEST_ERROR_DOMAIN_NAME, hm_error_injection_cb, nullptr);

    hm_config_t* hm_config = get_hm_config();

    ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;
    assert_true(einj_payload->version == ERROR_INJECTION_PAYLOAD_VERSION);
    assert_true(einj_payload->component_group == test_error_domain);
    einj_payload->component_instance = 0;

    assert_true(hm_inject_error() == ACPI_EINJ_INVALID_ACCESS);
}