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
#include <gicd_regs.h>
#include <health_monitor.h>
#include <health_monitor_i.h>
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
void __wrap_gpio_set_output(uint32_t gpio_ctrl_pin_id, uint32_t level)
{
    FPFW_UNUSED(gpio_ctrl_pin_id);
    FPFW_UNUSED(level);
    function_called();
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
    function_called();
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

int __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(int);
}

//
// Mocks
//
}

//
// Tests
//

TEST_FUNCTION(test_hm_submit_cper_no_ras, post_ddr_setup, nullptr)
{
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    will_return(__wrap_config_get_ras_disable_single_die, true);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    acpi_cper_section_t cper_section;
    cper_section.sec_mesh = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(general_cper_section));
}

TEST_FUNCTION(test_hm_submit_cper_ce, post_ddr_setup, nullptr)
{
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS);
    expect_value(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT);
    expect_function_call(__wrap_mmio_write32);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    acpi_cper_section_t cper_section;
    cper_section.sec_mesh = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    assert_true(current_ghes_error_record_base->block_status_ce == 1);
}

TEST_FUNCTION(test_hm_submit_cper_ce_multi, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value_count(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, -1);
    expect_value_count(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT, -1);
    expect_function_call_any(__wrap_mmio_write32);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    acpi_cper_section_t cper_section;
    cper_section.sec_mesh = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(general_cper_section));
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    assert_true(current_ghes_error_record_base->block_status_multi_ce == true);
}

TEST_FUNCTION(test_hm_submit_cper_ue, post_ddr_setup, nullptr)
{
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS);
    expect_value(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT);
    expect_function_call(__wrap_mmio_write32);
    expect_function_call(__wrap_gpio_set_output);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    acpi_cper_section_t cper_section;
    cper_section.sec_mesh = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    assert_true(current_ghes_error_record_base->block_status_ue == true);
}

TEST_FUNCTION(test_hm_submit_cper_ue_multi, post_ddr_setup, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_value_count(__wrap_mmio_write32, addr, (uint32_t)MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, -1);
    expect_value_count(__wrap_mmio_write32, data, OS_CPER_ERROR_DEVICE_EVT, -1);
    expect_function_call_any(__wrap_mmio_write32);
    expect_function_call_any(__wrap_gpio_set_output);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    acpi_cper_section_t cper_section;
    cper_section.sec_mesh = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(general_cper_section));
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    assert_true(current_ghes_error_record_base->block_status_multi_ue == true);
}

TEST_FUNCTION(test_hm_submit_cper_arsm, post_ddr_setup, nullptr)
{
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);

    // reset last cper record
    hm_config_t* hm_config = get_hm_config();
    void* last_cper_base = (void*)hm_config->mscp_full_cper_record_base;
    memset(last_cper_base, 0, RAS_LAST_CPER_SIZE);

    acpi_err_sec_firmware_t general_cper_section = {};

    acpi_cper_section_t cper_section;
    cper_section.sec_fw = general_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(general_cper_section));

    // locate error record
    acpi_cper_record_t* full_mscp_cper = (acpi_cper_record_t*)hm_config->mscp_full_cper_record_base;
    assert_true(full_mscp_cper->record_header.revision == 0x0101);
}

TEST_FUNCTION(test_hm_submit_cached_cper, post_ddr_setup, nullptr)
{
    hm_submit_cached_cper();
}