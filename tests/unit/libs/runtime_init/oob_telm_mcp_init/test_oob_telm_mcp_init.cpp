//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for i3c target ininitialization on mcp.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <DfwkHost.h>        // for PDFWK_DEVICE_HEADER, PDFWK_SCHE...
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <fpfw_dw_i3c.h>     // for fpfw_dw_i3c_config_t, fpfw_dw_i3c_device_t
#include <fpfw_init.h>
#include <fpfw_mctp.h>             // for fpfw_mctp_config, fpfw_mctp
#include <fpfw_mctp_i3c_binding.h> // for fpfw_mctp_i3c_binding_ctx, fpfw_mctp_i3c_binding_config
#include <fpfw_pldm_service.h>     // for pldm_service_config_t, pldm_platform_event_ready_notification
#include <idsw.h>
#include <idsw_kng.h> // for KNG_DIE_ID
#include <mcp_exp_top_regs.h>
#include <pldm_pdr.h> // for pldm_pdr_timestamp_t
#include <silibs_mcp_top_regs.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_i3c_target;
extern fpfw_init_component_t _fpfw_component_mctp;
extern fpfw_init_component_t _fpfw_component_pdr_repo;
extern fpfw_init_component_t _fpfw_component_pldm;

/*------------- Functions ----------------*/
/* Mocks */

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

fpfw_status_t __wrap_fpfw_dw_i3c_initialize(fpfw_dw_i3c_device_t* device, fpfw_dw_i3c_config_t* config)
{
    assert_non_null(device);
    assert_non_null(config);

    return mock_type(fpfw_status_t);
}

void __wrap_fpfw_dw_i3c_interface_initialize(fpfw_dw_i3c_device_t* device, fpfw_dw_i3c_interface_t* dwfk_interface)
{
    assert_non_null(device);
    assert_non_null(dwfk_interface);
    function_called();
}

fpfw_status_t __wrap_fpfw_mctp_i3c_binding_init(fpfw_mctp_i3c_binding_ctx* ctx, fpfw_mctp_i3c_binding_config* config)
{
    assert_non_null(ctx);
    assert_non_null(config);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_mctp_init(fpfw_mctp* mctp_ctx, fpfw_mctp_config* config)
{
    assert_non_null(mctp_ctx);
    assert_non_null(config);
    return mock_type(fpfw_status_t);
}

pldm_pdr_timestamp_t __wrap_pldm_pdr_get_timestamp(void)
{
    pldm_pdr_timestamp_t timestamp;
    timestamp.as_raw = {{0}};
    function_called();
    return timestamp;
}

fpfw_status_t __wrap_fpfw_pldm_service_init(pldm_service_config_t* config)
{
    assert_non_null(config);
    function_called();
    return mock_type(fpfw_status_t);
}

void __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* notification)
{
    assert_non_null(notification);
    function_called();
}

/* Tests */
TEST_FUNCTION(test_i3c_target_init_pass, NULL, NULL)
{
    will_return(__wrap_idsw_get_die_id, 0);
    will_return(__wrap_fpfw_dw_i3c_initialize, FPFW_INIT_STATUS_SUCCESS);

    DFWK_THREADX_HOST test_host = {};
    will_return(__wrap_fpfw_init_get_handle, &test_host);
    expect_function_call(__wrap_DfwkDeviceInitialize);

    fpfw_init_result_t result = _fpfw_component_i3c_target.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_i3c_target_init_fail, NULL, NULL)
{
    will_return(__wrap_idsw_get_die_id, 0);
    will_return(__wrap_fpfw_dw_i3c_initialize, FPFW_STATUS_FAIL);
    fpfw_init_result_t result = _fpfw_component_i3c_target.init_fn();

    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_i3c_target_init_pass_die1, NULL, NULL)
{
    will_return(__wrap_idsw_get_die_id, 1);
    fpfw_init_result_t result = _fpfw_component_i3c_target.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mctp_init_pass, NULL, NULL)
{
    static fpfw_dw_i3c_device_t mock_i3c_device;
    will_return(__wrap_fpfw_init_get_handle, &mock_i3c_device);

    expect_function_call(__wrap_fpfw_dw_i3c_interface_initialize);

    will_return(__wrap_fpfw_mctp_i3c_binding_init, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_mctp_init, FPFW_STATUS_SUCCESS);

    fpfw_init_result_t result = _fpfw_component_mctp.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(test_mctp_init_fail_binding_init_early, NULL, NULL)
{
    static fpfw_dw_i3c_device_t mock_i3c_device;
    will_return(__wrap_fpfw_init_get_handle, &mock_i3c_device);
    expect_function_call(__wrap_fpfw_dw_i3c_interface_initialize);

    will_return(__wrap_fpfw_mctp_i3c_binding_init, FPFW_STATUS_FAIL);

    fpfw_init_result_t result = _fpfw_component_mctp.init_fn();

    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_mctp_init_fail_mctp_init, NULL, NULL)
{
    // Mock the dependency handle
    static fpfw_dw_i3c_device_t mock_i3c_device;
    will_return(__wrap_fpfw_init_get_handle, &mock_i3c_device);

    // Set up expectations
    expect_function_call(__wrap_fpfw_dw_i3c_interface_initialize);

    will_return(__wrap_fpfw_mctp_i3c_binding_init, FPFW_STATUS_SUCCESS);

    will_return(__wrap_fpfw_mctp_init, FPFW_STATUS_FAIL);

    fpfw_init_result_t result = _fpfw_component_mctp.init_fn();

    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_pldm_init, NULL, NULL)
{
    // Mock the timestamp function
    expect_function_call(__wrap_pldm_pdr_get_timestamp);

    // Mock dependency handles
    static fpfw_mctp mock_mctp_ctx;
    static void* mock_pdr_repo;

    // First call is for mctp handle, second is for pdr_repo handle
    will_return(__wrap_fpfw_init_get_handle, &mock_mctp_ctx);
    will_return(__wrap_fpfw_init_get_handle, &mock_pdr_repo);

    // Mock successful PLDM service initialization
    will_return(__wrap_fpfw_pldm_service_init, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_init);

    // Expect platform event notification registration
    expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);

    fpfw_init_result_t result = _fpfw_component_pldm.init_fn();

    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle); // PLDM init returns NULL handle
}

TEST_FUNCTION(test_pldm_init_fail_service_init, NULL, NULL)
{
    // Mock the timestamp function
    expect_function_call(__wrap_pldm_pdr_get_timestamp);

    // Mock dependency handles
    static fpfw_mctp mock_mctp_ctx;
    static void* mock_pdr_repo;

    // First call is for mctp handle, second is for pdr_repo handle
    will_return(__wrap_fpfw_init_get_handle, &mock_mctp_ctx);
    will_return(__wrap_fpfw_init_get_handle, &mock_pdr_repo);

    // Mock failed PLDM service initialization
    will_return(__wrap_fpfw_pldm_service_init, FPFW_STATUS_FAIL);
    expect_function_call(__wrap_fpfw_pldm_service_init);

    fpfw_init_result_t result = _fpfw_component_pldm.init_fn();

    assert_true(result.status != FPFW_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

} /* extern "C" */
