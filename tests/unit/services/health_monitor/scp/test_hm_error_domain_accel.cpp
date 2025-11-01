//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_domain_mcp.cpp
 * Tests health monitor error injection
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...
#include <map>

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
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

#define BUG_CHECK_RETURN_VALUE 0x1
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN
#ifndef ACCEL_CPER_EXPECTED_MAGIC
    #define ACCEL_CPER_EXPECTED_MAGIC (0x43504552U) /* 'CPER' */
#endif

/*------------- Typedefs -----------------*/

struct callback_info
{
    icc_base_recv_complete_notify cb;
    void* ctx;
};

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// Global map that uses uint32_t as key and callback_info as value
static std::map<uint32_t, callback_info> g_callback_map;
static jmp_buf mock_jump_buf;

/*------------- Functions ----------------*/

//
// Mocks
//
_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);
    return (mock_type(uint32_t));
}

// Custom functions
void custom_fpfw_recv(fpfw_icc_base_recv_req_t* req)
{
    // Check if the callback exists in the map
    callback_info info = {.cb = req->cb, .ctx = req->cb_ctx};
    g_callback_map[req->recv_cmd_code] = info;
}

void custom_invoke_callback(uint32_t cmd_code, fpfw_status_t status, bool send_null_ctx = false)
{
    // Check if the callback exists in the map
    auto it = g_callback_map.find(cmd_code);
    if (it != g_callback_map.end())
    {
        // Invoke the callback with the provided status
        if (!send_null_ctx)
        {
            it->second.cb(it->second.ctx, 0, status);
        }
        else
        {
            it->second.cb(nullptr, 0, status);
        }
    }
}

//
// Tests
//

TEST_FUNCTION(hm_sdm_error_domain_register_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_prepare_sdm_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM));
}

TEST_FUNCTION(hm_sdm_error_domain_register_cb_all_paths, post_ddr_setup, nullptr)
{
    hm_accel_error_domain_register_payload_t* reg_payload;

    // Get the payload object and fill it with info
    auto it = g_callback_map.find(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM));
    if (it != g_callback_map.end())
    {
        reg_payload = (hm_accel_error_domain_register_payload_t*)it->second.ctx;
        reg_payload->header.cmd = ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM);
        reg_payload->error_domain_idx = ACPI_ERROR_DOMAIN_SDM;
        reg_payload->valid_fru_id = 1;
        reg_payload->valid_fru_text = 1;
        reg_payload->fru_id = (guid_t)SDM_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
        strncpy(reg_payload->fru_text, SDM_PROC_FRU, sizeof(reg_payload->fru_text) - 1);
        reg_payload->fru_text[sizeof(reg_payload->fru_text) - 1] = '\0';
    }

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    // The hmm registration callback succeeds without any issues
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM), FPFW_STATUS_SUCCESS);

    // The registration callback will fail due to NULL payload
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    // The cper submit buffer will fail in the callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM), FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_FAIL);
    // The registration ack to accel will fail in the callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM), FPFW_STATUS_SUCCESS);

    // Fail on status check in callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM), FPFW_STATUS_FAIL);
}

TEST_FUNCTION(hm_sdm_error_domain_register_listener_recv_reg_fail, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);

    // This will trigger the bug_assert
    if (!bugcheck_mock_return())
    {
        hm_prepare_sdm_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM));
    }
}

TEST_FUNCTION(hm_cded_error_domain_register_listener, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    // expect_function_call_any(__wrap_wait_for_semaphore);
    // expect_function_call_any(__wrap_release_semaphore);
    // expect_function_call_any(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    // will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_prepare_cded_sdm_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED));
}

TEST_FUNCTION(hm_cded_error_domain_register_cb_all_paths, post_ddr_setup, nullptr)
{
    hm_accel_error_domain_register_payload_t* reg_payload;

    // Get the payload object and fill it with info
    auto it = g_callback_map.find(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED));
    if (it != g_callback_map.end())
    {
        reg_payload = (hm_accel_error_domain_register_payload_t*)it->second.ctx;
        reg_payload->header.cmd = ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED);
        reg_payload->error_domain_idx = ACPI_ERROR_DOMAIN_CDED_SDM;
        reg_payload->valid_fru_id = 1;
        reg_payload->valid_fru_text = 1;
        reg_payload->fru_id = (guid_t)CDED_SDM_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
        strncpy(reg_payload->fru_text, CDED_SDM_PROC_FRU, sizeof(reg_payload->fru_text) - 1);
        reg_payload->fru_text[sizeof(reg_payload->fru_text) - 1] = '\0';
    }

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    // The hmm registration callback succeeds without any issues
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS);

    // The registration callback will fail due to NULL payload
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS, true);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    // The cper submit buffer will fail in the callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_FAIL);
    // The registration ack to accel will fail in the callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS);

    // Fail on status check in callback path
    custom_invoke_callback(ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_FAIL);
}

TEST_FUNCTION(hm_cded_error_domain_register_listener_recv_reg_fail, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    // expect_function_call_any(__wrap_wait_for_semaphore);
    // expect_function_call_any(__wrap_release_semaphore);
    // expect_function_call_any(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);
    // will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    if (!bugcheck_mock_return())
    {
        hm_prepare_cded_sdm_listener((fpfw_icc_base_ctx_t*)ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED));
    }
}

TEST_FUNCTION(hm_sdm_error_injection_cb, post_ddr_setup, nullptr)
{
    will_return(__wrap_system_info_get_mission_mode, false);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_SDM;
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

TEST_FUNCTION(hm_sdm_error_injection_cb_icc_send_fail, post_ddr_setup, nullptr)
{
    will_return(__wrap_system_info_get_mission_mode, false);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_STATUS_FAIL);
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_SDM;
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

TEST_FUNCTION(hm_cded_error_injection_cb, post_ddr_setup, nullptr)
{
    will_return(__wrap_system_info_get_mission_mode, false);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_CDED_SDM;
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

TEST_FUNCTION(hm_cded_error_injection_cb_icc_send_fail, post_ddr_setup, nullptr)
{
    will_return(__wrap_system_info_get_mission_mode, false);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_STATUS_FAIL);
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return_always(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_atu_unmap, SILIBS_SUCCESS);

    hm_config_t* hm_config = get_hm_config();
    ras_einj_info_t input_einj_payload;
    memset(&input_einj_payload, 0, sizeof(ras_einj_info_t));
    input_einj_payload.version = ERROR_INJECTION_PAYLOAD_VERSION;
    input_einj_payload.component_type = 0;
    input_einj_payload.component_group = (uint16_t)ACPI_ERROR_DOMAIN_CDED_SDM;
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

TEST_FUNCTION(hm_sdm_err_submission_cb_all_path, post_ddr_setup, nullptr)
{
    hm_accel_cper_info_t* cper_info;
    acpi_err_sec_accel_vendor_t cper_mesg;

    // Get the payload object and fill it with info
    auto it = g_callback_map.find(ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED));
    if (it == g_callback_map.end())
    {
        return;
    }

    // Just set the dummy payload to 0
    memset(&cper_mesg, 0, sizeof(acpi_err_sec_accel_vendor_t));

    cper_info = (hm_accel_cper_info_t*)it->second.ctx;
    cper_info->msg_payload.header.cmd = ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED);
    // Using absolute address as atu mock will return 0
    // Subtract 0x80000 - the dtcm offset that gets added in cper cb function
    cper_info->msg_payload.dtcm_mem_offset = ((uint32_t)&cper_mesg) - 0x80000;

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value_count(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, -1);
    expect_value_count(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT, -1);
    expect_function_call_any(__wrap_mmio_write32);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // First cper run
    custom_invoke_callback(ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS);

    // Switch accel ID to SDM and send fail status
    cper_info->accel_id = ACCEL_ID_SDM;

    custom_invoke_callback(ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_FAIL);

    // Command code mismatch
    custom_invoke_callback(ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED), FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_update_fatal_cper_info, post_ddr_setup, nullptr)
{
    hm_update_accel_fatal_cper_info(ACCEL_ID_SDM, 0x0, 0x0);
}

TEST_FUNCTION(hm_collect_fatal_cper, post_ddr_setup, nullptr)
{
    // The API has a hard-coded memcopy which will cause an exception
    // The only way to avoid it is to allocate a dummy buffer
    uint32_t* cper_dummy_addr = (uint32_t*)calloc(1, sizeof(acpi_err_sec_accel_vendor_t));
    uint32_t cper_diff = ((uint32_t)cper_dummy_addr) - 0x80000;

    // Return the cper diff via __wrap_atu_svc_accel_atu_addr
    will_return_always(__wrap_atu_svc_accel_atu_addr, cper_diff);

    expect_any(__wrap_mmio_read32, addr);
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, ACCEL_CPER_EXPECTED_MAGIC);

    FPFW_UNUSED(cper_dummy_addr);
    FPFW_UNUSED(cper_diff);

    assert_true(hm_collect_accel_fatal_cper(ACCEL_ID_SDM));

    // Trigger failure path
    expect_any(__wrap_mmio_read32, addr);
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0x0);

    assert_false(hm_collect_accel_fatal_cper(ACCEL_ID_SDM));
}

TEST_FUNCTION(hm_send_fatal_cper, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value_count(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, -1);
    expect_value_count(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT, -1);
    expect_function_call_any(__wrap_mmio_write32);

    hm_send_accel_error_cper(ACCEL_ID_SDM);
    // CPER not collected for cded, will not trigger the send path
    hm_send_accel_error_cper(ACCEL_ID_CDED);
}
}
