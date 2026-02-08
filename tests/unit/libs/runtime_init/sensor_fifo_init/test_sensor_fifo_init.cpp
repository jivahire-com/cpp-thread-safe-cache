//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sds_init.cpp
 * Tests the init of the sds service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <boot_status.h> // for boot_status_notify_extd
#include <core_info.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <idsw_kng.h>
#include <scf_mhu_device.h>
#include <sensor_fifo_cli_service.h>
#include <sensor_fifo_driver_interface.h>
#include <sensor_fifo_service.h>
#include <sensor_fifo_service_init.h>
#include <startup_shutdown.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_sensor_fifo;

/*------------- Functions ----------------*/

//
// Mocks
//

void* __wrap_fpfw_init_get_handle(fpfw_init_component_id_t component_id)
{
    check_expected(component_id);
    return mock_type(void*);
}

void __wrap_scf_mhu_device_initialize(scf_mhu_device_t* device,
                                      void* schedule,
                                      sensor_fifo_device_properties_t* properties,
                                      size_t properties_count,
                                      scf_mhu_device_config_t* config)
{
    FPFW_UNUSED(device);
    FPFW_UNUSED(schedule);
    FPFW_UNUSED(properties);
    FPFW_UNUSED(properties_count);
    FPFW_UNUSED(config);
    function_called();
}

void __wrap_sensor_fifo_driver_inf_init(sensor_fifo_driver_interface_t* drv_interface, void* device)
{
    FPFW_UNUSED(drv_interface);
    FPFW_UNUSED(device);
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

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

void __wrap_sensor_fifo_cli_svc_initialize(sensor_fifo_driver_interface_t* driver_interface)
{
    FPFW_UNUSED(driver_interface);
    function_called();
}

void __wrap_sensor_fifo_svc_initialize(psensor_fifo_driver_interface_t driver_interface, sensor_fifo_svc_config_t* svc_config)
{
    FPFW_UNUSED(driver_interface);
    FPFW_UNUSED(svc_config);
    function_called();
}

corebits_t* __wrap_core_info_get_enable_cores_result()
{
    return mock_type(corebits_t*);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return PLATFORM_FPGA;
}

} // extern "C"

//
// Tests
//

#ifdef SCP_RUNTIME_INIT
TEST_FUNCTION(test_sensor_fifo_scp_init, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    // Mock fpfw_init_get_handle("dfwk") - called after core_info_get_enable_cores_result
    static DFWK_THREADX_HOST mock_dfwk = {{0}};
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, &mock_dfwk);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    // Mock fpfw_init_get_handle("icc_mscp2mscp") - called after sensor_fifo_driver_inf_init
    static int mock_icc_ctx = 0;
    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, &mock_icc_ctx);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sensor_fifo_scp_init_null_dfwk_handle, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    // Return NULL for dfwk handle - simulating failure
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test - documents that no error handling exists for NULL handles
    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sensor_fifo_scp_init_null_icc_handle, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    static DFWK_THREADX_HOST mock_dfwk = {{0}};
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, &mock_dfwk);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    // Return NULL for icc_mscp2mscp handle - simulating failure
    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
#endif

#ifdef MCP_RUNTIME_INIT
TEST_FUNCTION(test_sensor_fifo_mcp_init, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    static DFWK_THREADX_HOST mock_dfwk = {{0}};
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, &mock_dfwk);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    static int mock_icc_ctx = 0;
    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, &mock_icc_ctx);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sensor_fifo_mcp_init_null_dfwk_handle, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    // Return NULL for dfwk handle
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sensor_fifo_mcp_init_null_icc_handle, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    uint32_t expected_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, MCP_PRIMARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_START);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    static corebits_t test_cores = {0};
    will_return(__wrap_core_info_get_enable_cores_result, &test_cores);

    static DFWK_THREADX_HOST mock_dfwk = {{0}};
    expect_string(__wrap_fpfw_init_get_handle, component_id, "dfwk");
    will_return(__wrap_fpfw_init_get_handle, &mock_dfwk);

    expect_function_call(__wrap_scf_mhu_device_initialize);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);
    expect_function_call(__wrap_sensor_fifo_driver_inf_init);

    // Return NULL for icc handle
    expect_string(__wrap_fpfw_init_get_handle, component_id, "icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, NULL);

    expect_function_call(__wrap_sensor_fifo_svc_initialize);
    expect_function_call(__wrap_sensor_fifo_cli_svc_initialize);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_SENSOR_FIFO_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    fpfw_init_result_t result = _fpfw_component_sensor_fifo.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
#endif
