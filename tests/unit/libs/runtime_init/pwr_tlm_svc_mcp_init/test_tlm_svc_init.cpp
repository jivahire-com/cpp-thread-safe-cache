//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tlm_svc_init.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <pwr_tlm_cli_service.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern fpfw_init_component_t _fpfw_component_pwr_tlm_svc_mcp;

/*-- Declarations (Statics and globals) --*/
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/*------------- Functions ----------------*/

//
// Mocks
//

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

power_tlm_knobs_t __wrap_config_get_pwr_tlm_knobs(void)
{
    return *((power_tlm_knobs_t*)mock_type(power_tlm_knobs_t*));
}

power_tlm_mpam_mcp_knobs_t __wrap_config_get_pwr_tlm_mpam_mcp_knobs(void)
{
    return *((power_tlm_mpam_mcp_knobs_t*)mock_type(power_tlm_mpam_mcp_knobs_t*));
}

void __wrap_telemetry_service_init(uint8_t die_id,
                                   uint32_t pwr_pkg_period_ms,
                                   uint32_t inst_pkg_sample_period_ms,
                                   uint16_t inst_samples_per_pkg,
                                   uint32_t _24_hr_pkg_sample_period_ms,
                                   uint32_t mpam_vm_mem_fixed_pwr_mW,
                                   bool mpam_vm_mem_enable,
                                   bool all_zero_filtering_enable,
                                   bool is_single_die_system)
{
    check_expected(die_id);
    check_expected(pwr_pkg_period_ms);
    check_expected(inst_pkg_sample_period_ms);
    check_expected(inst_samples_per_pkg);
    check_expected(_24_hr_pkg_sample_period_ms);
    check_expected(mpam_vm_mem_fixed_pwr_mW);
    check_expected(mpam_vm_mem_enable);
    check_expected(all_zero_filtering_enable);
    check_expected(is_single_die_system);
}

void __wrap_pwr_tlm_cli_svc_init(void)
{
    function_called();
}
}
//
// Tests
//
TEST_FUNCTION(test_tlm_svc_init, nullptr, nullptr)
{
    power_tlm_knobs_t pwr_tlm_knobs;
    pwr_tlm_knobs.prod_pkg_period = PWR_TLM_PROD_PKG_PERIOD_30_SEC;
    pwr_tlm_knobs.inst_sample_period = PWR_TLM_INST_SAMPLE_PERIOD_20_MS;
    pwr_tlm_knobs.inst_samples_per_pkg = 10;
    pwr_tlm_knobs._24hr_sample_period = PWR_TLM_24HR_PKG_PERIOD_60_MIN; // 1 hour period
    pwr_tlm_knobs.all_zero_filtering_enable = true;

    power_tlm_mpam_mcp_knobs_t mpam_knobs = {0};
    mpam_knobs.fixed_pwr_mW = 1000;
    mpam_knobs.mem_pwr_primary_enable = true;

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_config_get_pwr_tlm_mpam_mcp_knobs, &mpam_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idhw_is_single_die_boot_en, false); // dual die system

    expect_value(__wrap_telemetry_service_init, die_id, DIE_0);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 30000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 20);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 10);
    expect_value(__wrap_telemetry_service_init, _24_hr_pkg_sample_period_ms, 3600000); // 1 hour in ms
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_fixed_pwr_mW, 1000);
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_enable, true);
    expect_value(__wrap_telemetry_service_init, all_zero_filtering_enable, true);
    expect_value(__wrap_telemetry_service_init, is_single_die_system, false);

    expect_function_call(__wrap_pwr_tlm_cli_svc_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_tlm_svc_mcp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_tlm_svc_init_other_branches, nullptr, nullptr)
{
    power_tlm_knobs_t pwr_tlm_knobs;
    pwr_tlm_knobs.prod_pkg_period = PWR_TLM_PROD_PKG_PERIOD_1_MIN;
    pwr_tlm_knobs.inst_sample_period = PWR_TLM_INST_SAMPLE_PERIOD_100_MS;
    pwr_tlm_knobs.inst_samples_per_pkg = 21;                                   // Exceeding the max limit
    pwr_tlm_knobs._24hr_sample_period = (power_tlm_24hr_package_period_t)0xFF; // Exceeding the max duration
    pwr_tlm_knobs.all_zero_filtering_enable = false;

    power_tlm_mpam_mcp_knobs_t mpam_knobs = {0};
    mpam_knobs.fixed_pwr_mW = 2000;
    mpam_knobs.mem_pwr_primary_enable = false;

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_config_get_pwr_tlm_mpam_mcp_knobs, &mpam_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_idhw_is_single_die_boot_en, true); // single die system

    expect_value(__wrap_telemetry_service_init, die_id, DIE_1);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 60000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 100);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 20);
    expect_value(__wrap_telemetry_service_init, _24_hr_pkg_sample_period_ms, 86400000); // default value
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_fixed_pwr_mW, 2000);
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_enable, false);
    expect_value(__wrap_telemetry_service_init, all_zero_filtering_enable, false);
    expect_value(__wrap_telemetry_service_init, is_single_die_system, true);

    expect_function_call(__wrap_pwr_tlm_cli_svc_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_tlm_svc_mcp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_tlm_svc_init_range_limit, nullptr, nullptr)
{
    power_tlm_knobs_t pwr_tlm_knobs;
    pwr_tlm_knobs.prod_pkg_period = _power_tlm_prod_package_period_t_PADDING;
    pwr_tlm_knobs.inst_sample_period = PWR_TLM_INST_SAMPLE_PERIOD_20_MS;
    pwr_tlm_knobs.inst_samples_per_pkg = 10;
    pwr_tlm_knobs._24hr_sample_period = PWR_TLM_24HR_PKG_PERIOD_30_MIN; // 1 hour period
    pwr_tlm_knobs.all_zero_filtering_enable = true;

    power_tlm_mpam_mcp_knobs_t mpam_knobs = {0};
    mpam_knobs.fixed_pwr_mW = 1500;
    mpam_knobs.mem_pwr_primary_enable = true;

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_config_get_pwr_tlm_mpam_mcp_knobs, &mpam_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idhw_is_single_die_boot_en, false); // dual die system

    expect_value(__wrap_telemetry_service_init, die_id, DIE_0);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 120000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 20);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 10);
    expect_value(__wrap_telemetry_service_init, _24_hr_pkg_sample_period_ms, 1800000); // 30 minutes in ms
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_fixed_pwr_mW, 1500);
    expect_value(__wrap_telemetry_service_init, mpam_vm_mem_enable, true);
    expect_value(__wrap_telemetry_service_init, all_zero_filtering_enable, true);
    expect_value(__wrap_telemetry_service_init, is_single_die_system, false);

    expect_function_call(__wrap_pwr_tlm_cli_svc_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_pwr_tlm_svc_mcp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}