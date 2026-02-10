//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_domain_register.cpp
 * Tests health monitor error domain registration
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
extern bool ddr_subsystem_up;
extern char* TEST_ERROR_DOMAIN_NAME;
extern guid_t error_domain_guid[];
acpi_error_domain_t test_error_domain = ACPI_ERROR_DOMAIN_MESH;
/*------------- Functions ----------------*/
acpi_einj_cmd_status_t hm_error_injection_cb(ras_einj_info_t* payload, void* ctx)
{
    FPFW_UNUSED(payload);
    FPFW_UNUSED(ctx);

    function_called();

    return ACPI_EINJ_SUCCESS;
}
//
// Mocks
//
}

//
// Tests
//
TEST_FUNCTION(test_hm_register_error_domain_no_ras, post_ddr_setup, nullptr)
{
    int error_domain_idx = (int)test_error_domain;
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    will_return(__wrap_config_get_ras_disable_single_die, true);

    hm_register_error_domain(error_domain_idx, error_domain_guid, TEST_ERROR_DOMAIN_NAME, hm_error_injection_cb, nullptr);
}

TEST_FUNCTION(test_hm_register_error_domain, post_ddr_setup, nullptr)
{
    int error_domain_idx = (int)test_error_domain;

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    hm_register_error_domain(error_domain_idx, error_domain_guid, TEST_ERROR_DOMAIN_NAME, hm_error_injection_cb, nullptr);

    hm_config_t* hm_config = get_hm_config();

    // locate error record
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += error_domain_idx;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
    {
        assert_true(current_ghes_error_record_base->data[section_count].valid_fru);
        assert_true(current_ghes_error_record_base->data[section_count].valid_fru_str);
        assert_true(memcmp(&current_ghes_error_record_base->data[section_count].fru_id, error_domain_guid, sizeof(guid_t)) == 0);
        assert_true(strcmp((const char*)current_ghes_error_record_base->data[section_count].fru_text,
                           TEST_ERROR_DOMAIN_NAME) == 0);
    }
}

TEST_FUNCTION(test_hm_register_error_domain_nofru, post_ddr_setup, nullptr)
{
    int error_domain_idx = (int)test_error_domain + 1;

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);
    hm_register_error_domain(error_domain_idx, NULL, NULL, hm_error_injection_cb, nullptr);

    hm_config_t* hm_config = get_hm_config();

    // locate error record
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    current_ghes_base += error_domain_idx;

    assert_true(current_ghes_base->enabled == true);

    uint32_t error_record_base = MSCP_GHES_ADDR(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)MSCP_GHES_ADDR((uint64_t)(*(uint32_t*)error_record_base));

    for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
    {
        assert_false(current_ghes_error_record_base->data[section_count].valid_fru);
        assert_false(current_ghes_error_record_base->data[section_count].valid_fru_str);
    }
}

TEST_FUNCTION(test_hm_register_cached_error_domain, post_ddr_setup, nullptr)
{
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);

    hm_error_domain_info_t* err_domain_info = hm_get_registered_error_domain(test_error_domain);
    err_domain_info->activated = false;

    hm_register_cached_error_domain();

    err_domain_info = hm_get_registered_error_domain(test_error_domain);
    assert_true(err_domain_info->activated == true);
}