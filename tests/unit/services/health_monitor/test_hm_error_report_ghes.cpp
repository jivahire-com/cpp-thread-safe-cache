//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_error_report_ghes.cpp
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
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define TEST_ERROR_DOMAIN_GUID                             \
    {                                                      \
        0x8e050c02, 0x0e53, 0x4e54,                        \
        {                                                  \
            0xa1, 0x44, 0xa5, 0x4c, 0x08, 0xbb, 0xaf, 0xf4 \
        }                                                  \
    }

/*-- Declarations (Statics and globals) --*/
const char* TEST_ERROR_DOMAIN_NAME = "hm_err";
guid_t error_domain_guid[] = {TEST_ERROR_DOMAIN_GUID};

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_wait_for_semaphore(SEMAPHORE_ID id, uint32_t key)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(key);
    function_called();
}

void __wrap_release_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);
    function_called();
}
}

//
// Tests
//
TEST_FUNCTION(test_construct_mscp_ghes_table, post_ddr_setup, nullptr)
{
    // will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    construct_mscp_ghes_table();

    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base = NULL;

    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        // GHES error record
        uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
        current_ghes_error_record_base =
            (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

        assert_true(current_ghes_error_record_base->data_length == sizeof(current_ghes_error_record_base->data));

        // Error Section
        for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
        {
            if ((current_ghes_error_record_base->data[section_count].revision != 0x300) ||
                (current_ghes_error_record_base->data[section_count].error_data_length != sizeof(acpi_cper_section_t)))
            {
                assert_true(false);
            }
        }

        current_ghes_base++;
    }
}

TEST_FUNCTION(test_activate_error_domain, post_ddr_setup, nullptr)
{
    expect_function_call(__wrap_wait_for_semaphore);
    expect_function_call(__wrap_release_semaphore);

    hm_config_t* hm_config = get_hm_config();

    // locate error record
    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;
    uint32_t error_record_base = (uint32_t)(current_ghes_base->address.address);
    acpi_ghes_error_record_dual_die_t* current_ghes_error_record_base =
        (acpi_ghes_error_record_dual_die_t*)(*(uint32_t*)error_record_base + hm_config->mscp_ghes_base_apcore_offset);

    for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
    {
        assert_false(current_ghes_error_record_base->data[section_count].valid_fru);
        assert_false(current_ghes_error_record_base->data[section_count].valid_fru_str);
    }

    activate_error_domain(0, error_domain_guid, TEST_ERROR_DOMAIN_NAME);

    for (uint32_t section_count = 0; section_count < current_ghes_base->max_sections_per_record; section_count++)
    {
        assert_true(current_ghes_error_record_base->data[section_count].valid_fru);
        assert_true(current_ghes_error_record_base->data[section_count].valid_fru_str);
        assert_true(memcmp(&current_ghes_error_record_base->data[section_count].fru_id, error_domain_guid, sizeof(guid_t)) == 0);
        assert_true(strcmp((const char*)current_ghes_error_record_base->data[section_count].fru_text,
                           TEST_ERROR_DOMAIN_NAME) == 0);
    }
}
