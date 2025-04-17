//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_health_monitor_init.cpp
 * Tests the init of the health monitor
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <accelip_id.h> // for ACCEL_ID_SDM, ACCEL_ID_CDED
#include <fpfw_init.h>  // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hm_svc;
extern fpfw_init_component_t _fpfw_component_hm_post_init;
extern fpfw_init_component_t _fpfw_component_hm_scp;
extern fpfw_init_component_t _fpfw_component_hm_mcp;
extern fpfw_init_component_t _fpfw_component_hm_hsp;
extern fpfw_init_component_t _fpfw_component_hm_sdm;
extern fpfw_init_component_t _fpfw_component_hm_cded;
extern fpfw_init_component_t _fpfw_component_hm_cli_init;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_hm_pre_ddr_init(hm_config_t* hm_config)
{
    FPFW_UNUSED(hm_config);
    assert_non_null(hm_config->mscp_ghes_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_ack_addr_table_base);
    assert_non_null(hm_config->mscp_ghes_error_record_addr_base);
    function_called();
}

void __wrap_hm_post_ddr_init()
{
    function_called();
}

void __wrap_hm_post_intercore_init(hm_intercore_type_t intercore_type, fpfw_icc_base_ctx_t* icc_ctx)
{
    check_expected(intercore_type);
    assert_non_null(icc_ctx);
    function_called();
}

void __wrap_hm_cli_init()
{
    function_called();
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    check_expected(accel_type);

    return mock_type(bool);
}
}

//
// Tests
//
TEST_FUNCTION(hm_svc, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, (KNG_DIE_ID)1);
    expect_function_call_any(__wrap_hm_pre_ddr_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_svc.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(hm_post_init, nullptr, nullptr)
{
    expect_function_call_any(__wrap_hm_post_ddr_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_post_init.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_scp, nullptr, nullptr)
{
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_SCP);
    expect_function_call_any(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_scp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_mcp, nullptr, nullptr)
{
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_MCP);
    expect_function_call_any(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_mcp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_hsp, nullptr, nullptr)
{
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_HSP);
    expect_function_call_any(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_hsp.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_cli_init, nullptr, nullptr)
{
    expect_function_call_any(__wrap_hm_cli_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_cli_init.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_sdm, nullptr, nullptr)
{
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_SDM);
    expect_function_call_any(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_sdm.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_cded, nullptr, nullptr)
{
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_value(__wrap_hm_post_intercore_init, intercore_type, HM_INTERCORE_CDED);
    expect_function_call_any(__wrap_hm_post_intercore_init);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_cded.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_sdm_isolation, nullptr, nullptr)
{
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_SDM);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_sdm.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(hm_cded_isolation, nullptr, nullptr)
{
    expect_value(__wrap_accel_is_isolation_enabled, accel_type, ACCEL_ID_CDED);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_hm_cded.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
