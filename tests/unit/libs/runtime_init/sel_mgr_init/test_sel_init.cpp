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
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_pldm_service.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_sel_mgr;
extern fpfw_init_component_t _fpfw_component_sel_m2m;
extern fpfw_init_component_t _fpfw_component_sel_d2d;
#ifdef MCP_RUNTIME_INIT
extern fpfw_init_component_t _fpfw_component_sel_m2as;
extern fpfw_init_component_t _fpfw_component_sel_pldm;
#endif
extern fpfw_init_component_t _fpfw_component_sel_dfwk;
extern fpfw_init_component_t _fpfw_component_sel_cli;

/*------------- Functions ----------------*/

//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    check_expected_ptr(id);

    function_called();

    return mock_type(void*);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected(icc_ctx);
    assert_non_null(params);

    function_called();

    return mock_type(fpfw_status_t);
}

#ifdef MCP_RUNTIME_INIT
fpfw_status_t __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* p_notification)
{
    assert_non_null(p_notification);

    function_called();

    return mock_type(fpfw_status_t);
}
#endif

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);
    function_called();
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    assert_non_null(Queue);
    assert_non_null(Device);
    assert_non_null(DispatchRoutine);
    assert_non_null(DispatchContext);
    assert_true(QueueType == DfwkQueueType_SerializedDispatch);

    function_called();
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    assert_non_null(Interface);
    assert_non_null(Device);
    assert_non_null(DispatchQueue);
    assert_null(DispatchSync);

    function_called();
}

int32_t __wrap_DfwkInterfaceOpen(PDFWK_INTERFACE_HEADER Interface, PDFWK_DEVICE_HEADER ClientDevice)
{
    assert_non_null(Interface);
    assert_non_null(ClientDevice);

    function_called();

    return mock_type(int32_t);
}

void __wrap_FpFwCliRegisterTable(PFPFW_CLI_COMMAND pTable, size_t tableLength)
{
    assert_non_null(pTable);
    assert_true(tableLength > 0);

    function_called();
}

void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    check_expected(OldState);

    function_called();
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    function_called();

    return mock_type(bool);
}
} // extern "C"

//
// Tests
//
TEST_FUNCTION(test_sel_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_FpFwLockInitialize);
    will_return(__wrap_FpFwLockAcquire, 123);
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_value(__wrap_FpFwLockRelease, OldState, 123);
    expect_function_call(__wrap_FpFwLockRelease);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_mgr.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sel_m2m_init, nullptr, nullptr)
{
    fpfw_icc_base_ctx_t* dummy_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;

    // Set up expectations
    expect_value(__wrap_fpfw_init_get_handle, id, (void*)"icc_mscp2mscp");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, dummy_icc_ctx);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_m2m.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sel_d2d_init, nullptr, nullptr)
{
    fpfw_icc_base_ctx_t* dummy_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;

    // Set up expectations
    will_return_always(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_idhw_is_single_die_boot_en);
    expect_value(__wrap_fpfw_init_get_handle, id, (void*)"icc_die2die");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, dummy_icc_ctx);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_d2d.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

#ifdef MCP_RUNTIME_INIT
TEST_FUNCTION(test_sel_m2as_init, nullptr, nullptr)
{
    fpfw_icc_base_ctx_t* dummy_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;

    // Set up expectations
    expect_value(__wrap_fpfw_init_get_handle, id, (void*)"icc_mcp2aps");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, dummy_icc_ctx);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_m2as.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sel_pldm_init, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    will_return(__wrap_fpfw_pldm_service_register_platform_event_ready_notification, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_pldm.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
#endif

TEST_FUNCTION(test_sel_dfwk_init, nullptr, nullptr)
{
    // Set up expectations
    DFWK_THREADX_HOST* dummy_handle = (DFWK_THREADX_HOST*)0xBADDBEEF;
    expect_value(__wrap_fpfw_init_get_handle, id, (void*)"dfwk");
    will_return(__wrap_fpfw_init_get_handle, dummy_handle);
    expect_function_call(__wrap_fpfw_init_get_handle);

    expect_function_call(__wrap_DfwkDeviceInitialize);
    expect_function_call(__wrap_DfwkQueueInitialize);
    expect_function_call(__wrap_DfwkInterfaceInitialize);
    will_return(__wrap_DfwkInterfaceOpen, DFWK_SUCCESS);
    expect_function_call(__wrap_DfwkInterfaceOpen);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_dfwk.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_sel_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_FpFwCliRegisterTable);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sel_cli.init_fn();
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
