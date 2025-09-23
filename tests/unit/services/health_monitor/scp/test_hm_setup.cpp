//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hm_setup.cpp
 * Tests health monitor setup
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <mscp_exp_rmss_memory_map.h>
/*-- Symbolic Constant Macros (defines) --*/
hm_config_t hm_config_test = {0};

static acpi_ghes_t ghes_local[ACPI_ERROR_DOMAIN_COUNT + 1];
static acpi_ghes_error_record_dual_die_t ghes_error_record_local[ACPI_ERROR_DOMAIN_COUNT + 1];
static uint64_t ghes_record_addr_table_local[ACPI_ERROR_DOMAIN_COUNT] = {0};
static uint64_t ghes_ack_addr_table_local[ACPI_ERROR_DOMAIN_COUNT] = {0};
static uint8_t hsp_ras_payload[SCP_EXP_HSP_RAS_PAYLOAD_SIZE] = {0};
static uint8_t mscp_cper_record[SCP_EXP_MSCP_CPER_REPORT_SIZE] = {0};
extern acpi_error_domain_t test_error_domain;

/*-- Declarations (Statics and globals) --*/
uint32_t __wrap_AP_GHES_ADDR(uint32_t mscp_addr)
{
    return mscp_addr;
}

uint32_t __wrap_MSCP_GHES_ADDR(uint32_t ap_addr)
{
    return ap_addr;
}

int pre_ddr_setup(void** state)
{
    FPFW_UNUSED(state);

    hm_config_test.mscp_ghes_base = (acpi_ghes_t*)ghes_local;
    hm_config_test.mscp_ghes_error_record_addr_table_base = (uint32_t*)ghes_record_addr_table_local;
    hm_config_test.mscp_ghes_ack_addr_table_base = (uint64_t*)ghes_ack_addr_table_local;
    hm_config_test.mscp_ghes_error_record_addr_base = (uint32_t*)ghes_error_record_local;
    hm_config_test.mscp_ghes_base_apcore_offset = 0;
    hm_config_test.mscp_error_injection_addr_base = 0;
    hm_config_test.mscp_hsp_ras_payload_base = (uint8_t*)hsp_ras_payload;
    hm_config_test.mscp_full_cper_record_base = (uint8_t*)mscp_cper_record;
    hm_config_test.semaphore_id = SEM_ID_MSCP_EXP_1;
    hm_config_test.semaphore_key = 0;
    hm_config_test.is_primary = 1;
    hm_config_test.is_mcp = 0;

    hm_pre_ddr_init(&hm_config_test);

    hm_config_t* hm_config = get_hm_config();

    assert_true(hm_config->mscp_ghes_base == hm_config_test.mscp_ghes_base);
    assert_true(hm_config->mscp_ghes_error_record_addr_table_base == hm_config_test.mscp_ghes_error_record_addr_table_base);
    assert_true(hm_config->mscp_ghes_ack_addr_table_base == hm_config_test.mscp_ghes_ack_addr_table_base);
    assert_true(hm_config->mscp_ghes_error_record_addr_base == hm_config_test.mscp_ghes_error_record_addr_base);
    assert_true(hm_config->mscp_ghes_base_apcore_offset == hm_config_test.mscp_ghes_base_apcore_offset);

    return 0;
}

int pre_ddr_setup_on_mcp(void** state)
{
    pre_ddr_setup(state);
    hm_config_t* hm_config = get_hm_config();
    hm_config->is_mcp = true;

    return 0;
}

int post_ddr_setup(void** state)
{
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_mscp_exp_spi_synchronize_dies, 0);
    expect_function_call(__wrap_gpio_set_output);

    pre_ddr_setup(state);
    hm_post_ddr_init();

    return 0;
}
}
