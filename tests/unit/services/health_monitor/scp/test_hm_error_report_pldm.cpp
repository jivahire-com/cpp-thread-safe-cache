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
#include <libpldm/platform.h>
#include <ras.h>
#include <stdint.h>
#include <tx_timer.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
VOID (*watchdog_function)(ULONG id) = nullptr;

/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);

    assert_true(p_pe_config->p_descriptor->event_payload_size ==
                sizeof(acpi_cper_record_t) + sizeof(struct pldm_platform_cper_event));

    function_called();

    p_notification->CallBack(FPFW_PLDM_CC_SUCCESS, p_notification->context);

    return mock_type(fpfw_status_t);
}

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    assert_non_null(timer_ptr);
    check_expected_ptr(name_ptr);
    assert_non_null(expiration_function);
    check_expected(expiration_input);
    check_expected(initial_ticks);
    check_expected(reschedule_ticks);
    check_expected(auto_activate);
    FPFW_UNUSED(timer_control_block_size);

    watchdog_function = expiration_function;

    return mock_type(UINT);
}

UINT __wrap__txe_timer_deactivate(TX_TIMER* timer_ptr)
{
    assert_non_null(timer_ptr);
    function_called();

    return mock_type(UINT);
}
}

//
// Tests
//
TEST_FUNCTION(test_pldm_not_ready, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);

    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    hm_cper_transfer_listener_from_scp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_REQ_MCP);
}

TEST_FUNCTION(test_pldm_from_primary_scp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    // Clear last CPER record
    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    hm_set_pldm_ready_status();

    // dummy CPER record requested state
    hm_arsm_cper_backup_t* backup_cper = (hm_arsm_cper_backup_t*)last_cper_base;
    backup_cper->last_cper_record.transfer_status = HM_PLDM_TRANSFER_STATUS_REQUESTED;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);

    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    hm_cper_transfer_listener_from_scp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_REQ_MCP);
}

TEST_FUNCTION(test_pldm_from_primary_scp_no_pending, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    // Clear last CPER record
    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    hm_set_pldm_ready_status();

    // dummy CPER record requested state
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
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

    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    hm_set_pldm_ready_status();

    // dummy CPER record requested state
    hm_arsm_cper_backup_t* backup_cper = (hm_arsm_cper_backup_t*)last_cper_base;
    backup_cper->last_cper_record.transfer_status = HM_PLDM_TRANSFER_STATUS_REQUESTED;

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    hm_transfer_cper_mcp2bmc();
}

TEST_FUNCTION(test_pldm_from_secondary_mcp, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = false;
    hm_config->is_mcp = true;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call_any(__wrap_fpfw_icc_base_recv);

    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    hm_cper_transfer_listener_from_secondary_mcp((fpfw_icc_base_ctx_t*)ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP);
}

TEST_FUNCTION(test_pldm_OS_booted_pending_items, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    hm_set_pldm_ready_status();

    // dummy CPER record requested state
    hm_arsm_cper_backup_t* backup_cper = (hm_arsm_cper_backup_t*)last_cper_base;
    backup_cper->last_cper_record.transfer_status = HM_PLDM_TRANSFER_STATUS_REQUESTED;

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    hm_transfer_cper_mcp2bmc();

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS);
    expect_value(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT);
    expect_function_call(__wrap_mmio_write32);

    watchdog_function(0);
}

TEST_FUNCTION(test_pldm_OS_booted_no_pending_items, post_ddr_setup, nullptr)
{
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_primary = true;
    hm_config->is_mcp = true;

    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    hm_set_pldm_ready_status();

    // dummy CPER record requested state
    hm_arsm_cper_backup_t* backup_cper = (hm_arsm_cper_backup_t*)last_cper_base;
    backup_cper->last_cper_record.transfer_status = HM_PLDM_TRANSFER_STATUS_REQUESTED;

    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    expect_string(__wrap__txe_timer_create, name_ptr, "hm_flush");
    expect_any(__wrap__txe_timer_create, expiration_input);
    expect_any(__wrap__txe_timer_create, initial_ticks);
    expect_any(__wrap__txe_timer_create, reschedule_ticks);
    expect_value(__wrap__txe_timer_create, auto_activate, TX_AUTO_ACTIVATE);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);

    hm_transfer_cper_mcp2bmc();

    // simulate completion of all pending items
    volatile uint64_t* err_record_addr = (uint64_t*)hm_config->mscp_ghes_error_record_addr_table_base;
    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        acpi_ghes_error_record_dual_die_t* current_error_status_block =
            (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)err_record_addr);

        current_error_status_block->block_status_entry_count = 0;
        err_record_addr++;
    }

    expect_function_call(__wrap__txe_timer_deactivate);
    will_return(__wrap__txe_timer_deactivate, TX_SUCCESS);

    watchdog_function(0);
}