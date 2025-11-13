//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_startup_shutdown_init.cpp
 * Tests the init of the sos service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h>      // for DFWK_SCHEDULE
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwUtils.h>       // for FPFW_UNUSED
#include <fpfw_init.h>       // for fpfw_init_result_t, fpfw_init_component_t
#include <hsp_firmware_headers.h>
#include <ift_fw.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_sos_svc;
extern fpfw_init_component_t _fpfw_component_sos_int;

/*------------- Functions ----------------*/
//
// Mocks
//

void __wrap_mcp_sos_icc_init(fpfw_icc_base_ctx_t* icc_ctx)
{
    check_expected_ptr(icc_ctx);
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_sos_init(psos_device_t p_device, PDFWK_SCHEDULE p_schedule)
{
    assert_non_null(p_device);
    check_expected_ptr(p_schedule);
}

void __wrap_sos_interface_init(psos_device_t p_device, psos_interface_t p_interface, bool shared)
{
    check_expected_ptr(p_device);
    assert_non_null(p_interface);
    check_expected(shared);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    check_expected(RequestSize);
}

int32_t __wrap_sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface,
                                pstartup_ssi_registration_t p_registration,
                                PDFWK_INTERFACE_HEADER p_ssi_interface)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_registration);
    check_expected_ptr(p_ssi_interface);
    return mock_type(int32_t);
}

void __wrap_sos_start_phase(PDFWK_INTERFACE_HEADER p_interface,
                            pstartup_start_phase_request_t p_request,
                            ssi_startup_type_t boot_type,
                            ssi_startup_stage_t startup_stage,
                            DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                            void* p_completion_context)
{
    check_expected_ptr(p_interface);
    check_expected_ptr(p_request);
    check_expected(boot_type);
    check_expected(startup_stage);
    check_expected_ptr(completion_routine);
    check_expected_ptr(p_completion_context);
}

void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    function_called();
}
fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected(icc_ctx);
    check_expected(params->recv_cmd_code);

    return mock_type(fpfw_status_t);
}

bool __wrap_ift_is_enabled(void)
{
    return mock_type(bool);
}
}

//
// Tests
//

TEST_FUNCTION(sos_init_sos_svc, nullptr, nullptr)
{
    //! Set up expectations
    DFWK_THREADX_HOST test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &test_host); //! driver fmwk host handle
    expect_value(__wrap_sos_init, p_schedule, &(test_host.Schedule));

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_sos_svc.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

#ifdef SCP_RUNTIME_INIT
TEST_FUNCTION(sos_init_sos_int, nullptr, nullptr)
{
    // Set up expectations
    sos_device_t sos_device = {};

    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, true);

    // interface init is called twice
    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, false);

    will_return(__wrap_fpfw_init_get_handle, &sos_device);

    // now the ssi registration
    expect_not_value(__wrap_sos_register_ssi, p_interface, NULL);
    // p_registration and p_ssi_interface come from static allocations in the function
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    // intercore communication init (sos_icc_init)
    will_return(__wrap_fpfw_init_get_handle, (void*)0x1111); // icc_hspmbx
    will_return(__wrap_fpfw_init_get_handle, (void*)0x2222); // icc_die2die
    will_return(__wrap_fpfw_init_get_handle, (void*)0x3333); // icc_sdm_mbx
    expect_function_call(__wrap_FpFwLockInitialize);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    // now handle unit test of phase start requests
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_start_phase_request_t));

    // Check if IFT is enabled
    will_return(__wrap_ift_is_enabled, false);

    // sos_start_phase calls
    // synchronous
    expect_any(__wrap_sos_start_phase, p_interface);
    expect_value(__wrap_sos_start_phase, p_request, NULL);
    expect_value(__wrap_sos_start_phase, boot_type, COLD_BOOT);
    expect_value(__wrap_sos_start_phase, startup_stage, STARTUP_PHASE_MSCP_ASYNC);
    expect_any(__wrap_sos_start_phase, completion_routine);
    expect_any(__wrap_sos_start_phase, p_completion_context);
    // asynchronous
    expect_any(__wrap_sos_start_phase, p_interface);
    expect_not_value(__wrap_sos_start_phase, p_request, NULL);
    expect_value(__wrap_sos_start_phase, boot_type, COLD_BOOT);
    expect_value(__wrap_sos_start_phase, startup_stage, STARTUP_PHASE_AP_ASYNC);
    expect_not_value(__wrap_sos_start_phase, completion_routine, NULL);
    expect_any(__wrap_sos_start_phase, p_completion_context);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_sos_int.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

TEST_FUNCTION(sos_init_sos_ift_enabled_int, nullptr, nullptr)
{
    // Set up expectations
    sos_device_t sos_device = {};

    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, true);

    // interface init is called twice
    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, false);

    will_return(__wrap_fpfw_init_get_handle, &sos_device);

    // now the ssi registration
    expect_not_value(__wrap_sos_register_ssi, p_interface, NULL);
    // p_registration and p_ssi_interface come from static allocations in the function
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    // intercore communication init (sos_icc_init)
    will_return(__wrap_fpfw_init_get_handle, (void*)0x1111); // icc_hspmbx
    will_return(__wrap_fpfw_init_get_handle, (void*)0x2222); // icc_die2die
    will_return(__wrap_fpfw_init_get_handle, (void*)0x3333); // icc_sdm_mbx
    expect_function_call(__wrap_FpFwLockInitialize);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    // now handle unit test of phase start requests
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_start_phase_request_t));

    // Check if IFT is enabled
    will_return(__wrap_ift_is_enabled, true);

    // asynchronous
    expect_any(__wrap_sos_start_phase, p_interface);
    expect_not_value(__wrap_sos_start_phase, p_request, NULL);
    expect_value(__wrap_sos_start_phase, boot_type, IFT_BOOT);
    expect_value(__wrap_sos_start_phase, startup_stage, STARTUP_PHASE_IFT_ASYNC);
    expect_not_value(__wrap_sos_start_phase, completion_routine, NULL);
    expect_any(__wrap_sos_start_phase, p_completion_context);

    //! Call the function under test
    fpfw_init_result_t result = _fpfw_component_sos_int.init_fn();

    //! Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}

#endif

#ifdef MCP_RUNTIME_INIT
TEST_FUNCTION(sos_init_sos_int_mcp, nullptr, nullptr)
{
    // Set up expectations for MCP initialization
    sos_device_t sos_device = {};

    // First sos_interface_init call (public interface)
    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, true);

    // Second sos_interface_init call (private SSI interface)
    will_return(__wrap_fpfw_init_get_handle, &sos_device);
    expect_value(__wrap_sos_interface_init, p_device, &sos_device);
    expect_value(__wrap_sos_interface_init, shared, false);

    // mcp_sos_icc_init call
    will_return(__wrap_fpfw_init_get_handle, (void*)0x12345);
    expect_value(__wrap_mcp_sos_icc_init, icc_ctx, (void*)0x12345);
    expect_function_call(__wrap_mcp_sos_icc_init);

    // SSI registration
    expect_not_value(__wrap_sos_register_ssi, p_interface, NULL);
    expect_any(__wrap_sos_register_ssi, p_registration);
    expect_any(__wrap_sos_register_ssi, p_ssi_interface);
    will_return(__wrap_sos_register_ssi, FPFW_INIT_STATUS_SUCCESS);

    // DfwkAsyncRequestInitialize call
    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_start_phase_request_t));

    // sos_start_phase call - MCP only makes ONE call (synchronous)
    expect_any(__wrap_sos_start_phase, p_interface);
    expect_value(__wrap_sos_start_phase, p_request, NULL); // NULL for synchronous
    expect_value(__wrap_sos_start_phase, boot_type, COLD_BOOT);
    expect_value(__wrap_sos_start_phase, startup_stage, STARTUP_PHASE_MSCP_ASYNC);
    expect_value(__wrap_sos_start_phase, completion_routine, NULL);   // NULL for synchronous
    expect_value(__wrap_sos_start_phase, p_completion_context, NULL); // NULL for synchronous

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_sos_int.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_non_null(result.associated_handle);
}
#endif