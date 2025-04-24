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
#include <telemetry_cli_service.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern fpfw_init_component_t _fpfw_component_tlm_svc;

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

power_tlm_knobs_t __wrap_config_get_pwr_tlm_knobs(void)
{
    return *((power_tlm_knobs_t*)mock_type(power_tlm_knobs_t*));
}

void __wrap_telemetry_service_init(uint8_t die_id, uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms, uint16_t inst_samples_per_pkg)
{
    check_expected(die_id);
    check_expected(pwr_pkg_period_ms);
    check_expected(inst_pkg_sample_period_ms);
    check_expected(inst_samples_per_pkg);
}

void __wrap_telemetry_cli_svc_initialize(void)
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

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_value(__wrap_telemetry_service_init, die_id, DIE_0);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 30000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 20);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 10);

    expect_function_call(__wrap_telemetry_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_tlm_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_tlm_svc_init_other_branches, nullptr, nullptr)
{
    power_tlm_knobs_t pwr_tlm_knobs;
    pwr_tlm_knobs.prod_pkg_period = PWR_TLM_PROD_PKG_PERIOD_1_MIN;
    pwr_tlm_knobs.inst_sample_period = PWR_TLM_INST_SAMPLE_PERIOD_100_MS;
    pwr_tlm_knobs.inst_samples_per_pkg = 21; // Exceeding the max limit

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_1);

    expect_value(__wrap_telemetry_service_init, die_id, DIE_1);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 60000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 100);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 20);

    expect_function_call(__wrap_telemetry_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_tlm_svc.init_fn();

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

    will_return(__wrap_config_get_pwr_tlm_knobs, &pwr_tlm_knobs);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    expect_value(__wrap_telemetry_service_init, die_id, DIE_0);
    expect_value(__wrap_telemetry_service_init, pwr_pkg_period_ms, 120000);
    expect_value(__wrap_telemetry_service_init, inst_pkg_sample_period_ms, 20);
    expect_value(__wrap_telemetry_service_init, inst_samples_per_pkg, 10);

    expect_function_call(__wrap_telemetry_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_tlm_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}