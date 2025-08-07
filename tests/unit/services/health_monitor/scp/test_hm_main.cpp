//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_main.cpp
 * Tests health monitor main functionality
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
#include <mscp_exp_spi_synchronize_dies.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define ICC_APCORE_TEST_POINT 0x1000
#define ICC_MSCP_TEST_POINT   0x2000
#define ICC_HSP_TEST_POINT    0x3000
#define ICC_SDM_TEST_POINT    0x4000
#define ICC_CDED_TEST_POINT   0x5000

/*-------- Function Prototypes -----------*/
extern int pre_ddr_setup(void** state);
extern hm_config_t hm_config_test;

extern acpi_ghes_t ghes_local[];
extern acpi_ghes_error_record_dual_die_t ghes_error_record_local[];

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_initialize_semaphore()
{
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return (mock_type(bool));
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    FPFW_UNUSED(sync_point);
    FPFW_UNUSED(die_id);
    return (mock_type(int));
}
}

//
// Tests
//
TEST_FUNCTION(test_hm_pre_ddr_init, pre_ddr_setup, nullptr)
{
}

TEST_FUNCTION(test_hm_post_ddr_init, pre_ddr_setup, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_mscp_exp_spi_synchronize_dies, 0);
    expect_function_call(__wrap_gpio_set_output);
    hm_post_ddr_init();

    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;

    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        // GHES table
        if ((current_ghes_base->type != HM_GHES_VERSION_V2) || (current_ghes_base->source_id != error_domain_idx) ||
            (current_ghes_base->num_of_records != HM_ERROR_RECORD_COUNT) ||
            (current_ghes_base->max_sections_per_record != HM_ERROR_SECTION_COUNT))
        {
            assert_true(false);
        }

        current_ghes_base++;
    }
}

TEST_FUNCTION(test_hm_post_ddr_init_single_die, pre_ddr_setup, nullptr)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    expect_function_call(__wrap_gpio_set_output);

    hm_post_ddr_init();
    hm_config_t* hm_config = get_hm_config();

    acpi_ghes_t* current_ghes_base = hm_config->mscp_ghes_base;

    for (uint32_t error_domain_idx = 0; error_domain_idx < ACPI_ERROR_DOMAIN_COUNT; error_domain_idx++)
    {
        // GHES table
        if ((current_ghes_base->type != HM_GHES_VERSION_V2) || (current_ghes_base->source_id != error_domain_idx) ||
            (current_ghes_base->num_of_records != HM_ERROR_RECORD_COUNT) ||
            (current_ghes_base->max_sections_per_record != HM_ERROR_SECTION_COUNT))
        {
            assert_true(false);
        }

        current_ghes_base++;
    }
}

TEST_FUNCTION(test_hm_post_ddr_init_on_mcp, pre_ddr_setup_on_mcp, nullptr)
{
    hm_post_ddr_init();
    assert_true(ddr_subsystem_enabled());
}

TEST_FUNCTION(test_get_hm_config, pre_ddr_setup, nullptr)
{
    assert_true(get_hm_config() == &hm_config_test);
}

TEST_FUNCTION(test_hm_post_intercore_init_local, pre_ddr_setup, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_LOCAL, (fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_hm_post_intercore_init_remote, pre_ddr_setup, nullptr)
{
    expect_function_calls(__wrap_fpfw_icc_base_recv, 2);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_REMOTE, (fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_hm_post_intercore_init_local_mcp, pre_ddr_setup_on_mcp, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_LOCAL, (fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_hm_post_intercore_init_remote_mcp, pre_ddr_setup_on_mcp, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_REMOTE, (fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_hm_post_intercore_init_sdm, pre_ddr_setup, nullptr)
{
    expect_function_calls(__wrap_fpfw_icc_base_recv, 1);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_SDM, (fpfw_icc_base_ctx_t*)ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_SDM));
}

TEST_FUNCTION(test_hm_post_intercore_init_cded, pre_ddr_setup, nullptr)
{
    expect_function_calls(__wrap_fpfw_icc_base_recv, 1);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_CDED, (fpfw_icc_base_ctx_t*)ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED));
}

TEST_FUNCTION(test_hm_post_intercore_init_hsp, pre_ddr_setup, nullptr)
{
    expect_function_calls(__wrap_fpfw_icc_base_recv, 2);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_HSP, (fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_hm_post_intercore_init_apcore, pre_ddr_setup, nullptr)
{
    expect_function_calls(__wrap_fpfw_icc_base_recv, 1);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    hm_post_intercore_init(HM_INTERCORE_APCORE, (fpfw_icc_base_ctx_t*)1234);
}