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

/*------------- Functions ----------------*/
void __wrap_gpio_set_output(uint32_t gpio_ctrl_pin_id, uint32_t level)
{
    FPFW_UNUSED(gpio_ctrl_pin_id);
    FPFW_UNUSED(level);
    function_called();
}

//
// Mocks
//
}

//
// Tests
//
TEST_FUNCTION(test_hm_submit_cper_ce, post_ddr_setup, nullptr)
{
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &general_cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    assert_true(current_ghes_error_record_base->block_status_ce == 1);
}

TEST_FUNCTION(test_hm_submit_cper_ce_multi, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &general_cper_section, sizeof(general_cper_section));
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_CORRECTED, &general_cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    assert_true(current_ghes_error_record_base->block_status_multi_ce == true);
}

TEST_FUNCTION(test_hm_submit_cper_ue, post_ddr_setup, nullptr)
{
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    expect_function_call(__wrap_gpio_set_output);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &general_cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    assert_true(current_ghes_error_record_base->block_status_ue == true);
}

TEST_FUNCTION(test_hm_submit_cper_ue_multi, post_ddr_setup, nullptr)
{
    expect_function_call_any(__wrap_wait_for_semaphore);
    expect_function_call_any(__wrap_release_semaphore);
    expect_function_call_any(__wrap_gpio_set_output);

    acpi_err_sec_generic_t general_cper_section = {};
    general_cper_section.err_misc0 = 0x12;
    general_cper_section.err_misc1 = 0x34;
    general_cper_section.err_misc2 = 0x56;
    general_cper_section.err_misc3 = 0x78;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &general_cper_section, sizeof(general_cper_section));
    hm_submit_cper(ACPI_ERROR_DOMAIN_MESH, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &general_cper_section, sizeof(general_cper_section));

    // locate error record
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += ACPI_ERROR_DOMAIN_MESH;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    assert_true(current_ghes_error_record_base->block_status_multi_ue == true);
}

TEST_FUNCTION(test_hm_submit_cached_cper, post_ddr_setup, nullptr)
{
    hm_submit_cached_cper();
}