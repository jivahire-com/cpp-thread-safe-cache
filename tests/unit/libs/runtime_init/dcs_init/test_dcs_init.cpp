//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcs_init.cpp
 * ATU init tests
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <build_data.h> // for BUILD_ELF_SECTION_BINARY_METADATA
#include <data_collection_service.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_dcs_svc;

uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_dcs_init(p_dcs_config_t config)
{
    function_called();
    assert_non_null(config);
}

void __wrap_dcs_cli_svc_initialize(void)
{
    function_called();
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

//
// Tests
//
TEST_FUNCTION(test_dcs_init, nullptr, nullptr)
{
    uint32_t handle;

    will_return_always(__wrap_fpfw_init_get_handle, &handle);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    expect_function_call(__wrap_dcs_init);
    expect_function_call(__wrap_dcs_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_dcs_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_dcs_init_die1_scp, nullptr, nullptr)
{
    uint32_t handle;

    will_return_always(__wrap_fpfw_init_get_handle, &handle);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    expect_function_call(__wrap_dcs_init);
    expect_function_call(__wrap_dcs_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_dcs_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_dcs_init_die0_scp, nullptr, nullptr)
{
    uint32_t handle;

    will_return_always(__wrap_fpfw_init_get_handle, &handle);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_dcs_init);
    expect_function_call(__wrap_dcs_cli_svc_initialize);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_dcs_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
}