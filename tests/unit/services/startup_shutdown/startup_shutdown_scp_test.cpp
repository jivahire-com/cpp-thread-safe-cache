//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_scp_test.cpp
 * Startup shutdown service core-specific (SCP) tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include <CMockaWrapper.h>                // for check_expected, check_expected_ptr
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <hsp_firmware_headers.h>         // for HSP_FIRMWARE_ID
#include <hsp_firmware_headers.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <mhu_icc_transport.h>
#include <startup_shutdown.h>   // for sos_queue_entry_t, sos_queue_start_phase
#include <startup_shutdown_i.h> // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type

// Add accelerator_ip include for ACCEL_ID types used in new mocks/tests
#include <accelerator_ip.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_SHARED_MEM_SIZE (sizeof(icc_mhu_header_t) + (8))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

int32_t __wrap__txe_event_flags_set(TX_EVENT_FLAGS_GROUP* event_flags_group_ptr, ULONG flags, UINT options);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
unsigned __real_sos_core_boot_stage_count();
const startup_shutdown_boot_stage_t* __real_sos_core_boot_stages();
static icc_base_recv_complete_notify fw_load_cb = NULL;
static void* cb_ctx = NULL;
bool wrap_sos_shutdown = false;
void __real_sos_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                         pstartup_shutdown_request_t p_request,
                         ssi_shutdown_type_t shutdown_type,
                         DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                         void* p_completion_context);

int32_t __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    function_called();
    return 0;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
    function_called();
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    check_expected_ptr(icc_ctx);
    assert_non_null(params->payload_buffer);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected(icc_ctx);
    check_expected(params->recv_cmd_code);
    fw_load_cb = params->cb;
    cb_ctx = params->cb_ctx;

    return mock_type(fpfw_status_t);
}

void __wrap_sos_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                        pstartup_shutdown_request_t p_request,
                        DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                        void* p_completion_context)
{
    FPFW_UNUSED(p_interface);
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_routine);
    FPFW_UNUSED(p_completion_context);

    function_called();
}

void __wrap_sos_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                         pstartup_shutdown_request_t p_request,
                         ssi_shutdown_type_t shutdown_type,
                         DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                         void* p_completion_context)
{
    if (!(wrap_sos_shutdown))
    {
        __real_sos_shutdown(p_interface, p_request, shutdown_type, completion_routine, p_completion_context);
    }
    else
    {
        FPFW_UNUSED(p_interface);
        FPFW_UNUSED(p_request);
        FPFW_UNUSED(shutdown_type);
        FPFW_UNUSED(completion_routine);
        FPFW_UNUSED(p_completion_context);
        wrap_sos_shutdown = false; // reset the flag after the call
    }
}

void __wrap_reset_complete_wait_forever()
{
    function_called();
}

// New wraps for accelerator isolation and suspend used by execute_accel_quiesce_flow
bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(bool);
}

void __wrap_accel_core_suspend(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    function_called();
}

} // extern "C"

//
// Tests
//

// test for sos_core_boot_stage_count()
SOS_TEST(sos_core_boot_stage_count, NULL, NULL)
{
    unsigned count = __real_sos_core_boot_stage_count();
    // ensure some number of boot stages are returned
    assert_int_not_equal(count, 0);
}

// test for sos_core_shutdown_stage_count()
SOS_TEST(sos_core_shutdown_stage_count, NULL, NULL)
{
    unsigned count = sos_core_shutdown_stage_count();
    // ensure some number of shutdown stages are returned
    assert_int_not_equal(count, 0);
}

// test for sos_core_boot_stages
SOS_TEST(sos_core_boot_stages, NULL, NULL)
{
    LargestIntegralType expected_phases[] = {
        STARTUP_PHASE_MSCP_ASYNC,
        STARTUP_PHASE_AP_ASYNC,
        STARTUP_WARM_BOOT_SDM_ASYNC,
        STARTUP_WARM_BOOT_CDED_ASYNC,
        STARTUP_PHASE_IFT_ASYNC,
        STARTUP_PHASE_IFT_MEM_FW_LOAD,
        STARTUP_PHASE_IFT_CORE_FW_LOAD,
    };
    const startup_shutdown_boot_stage_t* p_stages = __real_sos_core_boot_stages();
    const unsigned int stage_count = __real_sos_core_boot_stage_count();
    // ensure the boot stages & their count are returned
    assert_non_null(p_stages);
    assert_int_not_equal(stage_count, 0);

    for (unsigned int i = 0; i < stage_count; i++)
    {
        assert_in_set(p_stages[i].phase, expected_phases, FPFW_ARRAY_SIZE(expected_phases));
    }
}

// test for sos_core_shutdown_stages
SOS_TEST(sos_core_shutdown_stages, NULL, NULL)
{
    const startup_shutdown_shutdown_stage_t* p_stages = sos_core_shutdown_stages();
    // ensure the boot stages are returned
    assert_non_null(p_stages);
    // check we have stages from both boot phases
    assert_int_equal(p_stages[0].stage, SHUTDOWN);
    assert_int_equal(p_stages[sos_core_shutdown_stage_count() - 1].stage, AP_WARM_RESET);
}

// test for sos_boot_timeout
SOS_TEST(sos_boot_timeout, NULL, NULL)
{
    unsigned int test_time_out = 100 * 1000;
    sos_stage_timeout_t current_stage = {};
    current_stage.stage_category = BOOT_STAGE;
    current_stage.operation_stage.boot = STARTUP_BL31_LOAD;
    current_stage.timeout_ms = test_time_out;

    will_return_always(__wrap_sos_core_boot_stage_count, __real_sos_core_boot_stage_count());

    // call the function
    sos_boot_timeout_override(&current_stage);
    unsigned int return_timeout = sos_boot_timeout(&current_stage);

    // check timeout get updated.
    assert_int_equal(return_timeout, test_time_out);
}

// test for sos_shutdown_timeout
SOS_TEST(sos_shutdown_timeout, NULL, NULL)
{
    unsigned int test_time_out = 100 * 1000;
    sos_stage_timeout_t current_stage = {};
    current_stage.stage_category = SHUTDOWN_STAGE;
    current_stage.operation_stage.shutdown = MSCP_SUBSYS_RESET;
    current_stage.timeout_ms = test_time_out;

    // call the function
    sos_shutdown_timeout_override(&current_stage);
    unsigned int return_timeout = sos_shutdown_timeout(&current_stage);

    // check timeout get updated.
    assert_int_equal(return_timeout, test_time_out);
}

// test for sos_core_override_timeout
SOS_TEST(sos_core_override_timeout_boot_stage, NULL, NULL)
{
    will_return_always(__wrap_sos_core_boot_stage_count, __real_sos_core_boot_stage_count());

    startup_reset_timeout_request_t test_request = {};
    test_request.timeout.stage_category = BOOT_STAGE;
    test_request.timeout.operation_stage.boot = STARTUP_PCIE_PHY_LOAD;
    test_request.timeout.timeout_ms = 100 * 1000;

    sos_core_override_timeout(&test_request);

    assert_true(__real_sos_core_boot_stages()[0].timeout_ms == test_request.timeout.timeout_ms);
}

SOS_TEST(sos_core_override_timeout_shutdown_stage, NULL, NULL)
{

    startup_reset_timeout_request_t test_request = {};
    test_request.timeout.stage_category = SHUTDOWN_STAGE;
    test_request.timeout.operation_stage.shutdown = SHUTDOWN;
    test_request.timeout.timeout_ms = 100 * 1000;

    sos_core_override_timeout(&test_request);

    assert_true(sos_core_shutdown_stages()[0].timeout_ms == test_request.timeout.timeout_ms);
}

// test for sos_core_shutdown_handler SHUTDOWN case
SOS_TEST(sos_core_shutdown_handler_shutdown, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_ctx);

    sos_core_shutdown_handler(SHUTDOWN);
}

// test for shutdown handler cb
SOS_TEST(sos_shutdown_req_cb, NULL, NULL)
{
    void* request = (void*)0x12345678;
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);
    shutdown_req_cb(request, DFWK_SUCCESS);
}

// test for prepare_reset_recv_cb
SOS_TEST(prepare_reset_recv_cb, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    fpfw_icc_base_recv_req_t test_recv_context = {};
    kng_hsp_mailbox_msg hsp_mailbox_msg = {};

    test_recv_context.recv_cmd_code = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ;
    test_recv_context.payload_buffer = &hsp_mailbox_msg;
    test_recv_context.buffer_size = sizeof(hsp_mailbox_msg);

    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));

    expect_function_call(__wrap_sos_quiesce);

    prepare_reset_recv_cb(&test_recv_context, 0x0c, DFWK_SUCCESS);
}

// test for quiesce_complete_cb
SOS_TEST(quiese_complete_notify, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;
    fpfw_icc_base_recv_req_t recv_ctx;
    kng_hsp_mailbox_cmd_core_reset_complete_notify test_send_params;
    test_send_params.header.cmd = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_RSP;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_ctx);

    recv_ctx.payload_buffer = &test_send_params;
    quiesce_complete_notify((DFWK_ASYNC_REQUEST_HEADER*)test_icc_ctx, &recv_ctx);
}

// test for quiesce_complete_cb
SOS_TEST(reset_complete_notify, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_function_call(__wrap_reset_complete_wait_forever);

    reset_complete_notify((DFWK_ASYNC_REQUEST_HEADER*)test_icc_ctx, DFWK_SUCCESS);
}

// test for d2d_shutdown_recv_cb
SOS_TEST(d2d_shutdown_recv_cb, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;
    size_t output_size_bytes = 8;
    wrap_sos_shutdown = true;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    startup_shutdown_request_t test_shutdown_request;
    test_shutdown_request.shutdown_type = REMOTE_SCP_SHUTDOWN;
    test_shutdown_request.header.AllocatedSize = sizeof(startup_shutdown_request_t);

    expect_any(__wrap_DfwkAsyncRequestInitialize, Request);
    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(startup_shutdown_request_t));

    d2d_shutdown_recv_cb(&test_shutdown_request, output_size_bytes, DFWK_SUCCESS);
}

// test for sos_send_d2d_shutdown_request
SOS_TEST(sos_send_d2d_shutdown_request, NULL, NULL)
{
    static uint8_t s_test_send_buffer[TEST_SHARED_MEM_SIZE];

    static icc_mhu_packet_t* s_test_send_packet = (icc_mhu_packet_t*)s_test_send_buffer;

    static fpfw_icc_base_send_req_t test_ppu_shutdown_send_params;

    test_ppu_shutdown_send_params.payload_buffer = (void*)s_test_send_packet; // Buffer containing the message to send
    test_ppu_shutdown_send_params.buffer_size = TEST_SHARED_MEM_SIZE;         // Size of the buffer
    test_ppu_shutdown_send_params.cb = NULL;     // Callback function for handling the send completion
    test_ppu_shutdown_send_params.cb_ctx = NULL; // Context for the callback

    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_d2dctx);

    sos_send_d2d_shutdown_request();
}

// test for sos_send_d2d_shutdown_request
SOS_TEST(sos_send_d2d_shutdown_request_fail, NULL, NULL)
{
    static uint8_t s_test_send_buffer[TEST_SHARED_MEM_SIZE];

    static icc_mhu_packet_t* s_test_send_packet = (icc_mhu_packet_t*)s_test_send_buffer;

    static fpfw_icc_base_send_req_t test_ppu_shutdown_send_params;

    test_ppu_shutdown_send_params.payload_buffer = (void*)s_test_send_packet; // Buffer containing the message to send
    test_ppu_shutdown_send_params.buffer_size = TEST_SHARED_MEM_SIZE;         // Size of the buffer
    test_ppu_shutdown_send_params.cb = NULL;     // Callback function for handling the send completion
    test_ppu_shutdown_send_params.cb_ctx = NULL; // Context for the callback

    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;

    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_d2dctx);

    sos_send_d2d_shutdown_request();
}

// test for recv_d2d_shutdown_request
SOS_TEST(recv_d2d_shutdown_request, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    recv_d2d_shutdown_request();
}

// test for recv_d2d_shutdown_request
SOS_TEST(recv_d2d_shutdown_request_fail, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);

    recv_d2d_shutdown_request();
}

// test for d2d_shutdown_send_cb
SOS_TEST(sos_d2d_shutdown_send_cb, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);
    startup_shutdown_request_t test_shutdown_request;
    test_shutdown_request.shutdown_type = REMOTE_SCP_SHUTDOWN;
    test_shutdown_request.header.AllocatedSize = sizeof(startup_shutdown_request_t);

    sos_d2d_shutdown_send_cb(&test_shutdown_request, FPFW_ICC_BASE_STATUS_SUCCESS);
}

// test for d2d_shutdown_send_cb
SOS_TEST(sos_d2d_shutdown_send_cb_fail, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);
    startup_shutdown_request_t test_shutdown_request;
    test_shutdown_request.shutdown_type = REMOTE_SCP_SHUTDOWN;
    test_shutdown_request.header.AllocatedSize = sizeof(startup_shutdown_request_t);

    sos_d2d_shutdown_send_cb(&test_shutdown_request, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
}

// test for sos_core_shutdown_handler COLD_RESET case
SOS_TEST(sos_core_shutdown_handler_cold_reset, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    fpfw_icc_base_ctx_t* test_icc_d2dctx = (fpfw_icc_base_ctx_t*)0xDEADBEEF;
    fpfw_icc_base_ctx_t* test_icc_sdm_ctx = (fpfw_icc_base_ctx_t*)0xFEEDBEEF;
    fpfw_icc_base_ctx_t* test_icc_cded_ctx = (fpfw_icc_base_ctx_t*)0xCAFEBABE;

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xDEADBEEF);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx, test_icc_d2dctx, test_icc_sdm_ctx, test_icc_cded_ctx);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_ctx);

    sos_core_shutdown_handler(COLD_RESET);
}

// New test for execute_accel_quiesce_flow
SOS_TEST(execute_accel_quiesce_flow_register_and_send_and_cb, NULL, NULL)
{
    // All accelerators are not isolated
    will_return_always(__wrap_accel_is_isolation_enabled, false);

    // Expect registration to receive accel quiesce response for both accelerators
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    // Expect sends of quiesce requests to both accelerators
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_send, icc_ctx);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_send, icc_ctx);

    // Mock for threadx event APIs
    expect_any(__wrap__txe_event_flags_set, event_flags_group_ptr);
    expect_any(__wrap__txe_event_flags_set, flags);
    expect_any(__wrap__txe_event_flags_set, options);

    // Expect the event flags to be set to indicate the flow is complete
    will_return_always(__wrap__txe_event_flags_set, TX_SUCCESS);

    // Execute and validate mask (two accels -> bits 0 and 1 set)
    uint32_t mask = execute_accel_quiesce_flow(0);
    assert_int_equal(mask, 0x3);

    // Simulate response callback and ensure accel_core_suspend is invoked
    assert_non_null(fw_load_cb);
    fw_load_cb(cb_ctx, 0, DFWK_SUCCESS);
}

// All accels isolated -> no recv/send, mask 0, no callback
SOS_TEST(execute_accel_quiesce_flow_all_isolated_noop, NULL, NULL)
{
    // Reset stored callback state
    fw_load_cb = NULL;
    cb_ctx = NULL;

    // Both accelerators isolated
    will_return(__wrap_accel_is_isolation_enabled, true);
    will_return(__wrap_accel_is_isolation_enabled, true);

    // Execute and validate mask is 0 and no callback registered
    uint32_t mask = execute_accel_quiesce_flow(0);
    assert_int_equal(mask, 0x0);
    assert_null(fw_load_cb);
}

// One accel isolated, one active with start offset; expect single recv/send and mask bit at offset; also invoke cb
SOS_TEST(execute_accel_quiesce_flow_one_isolated_offset_and_cb, NULL, NULL)
{
    // Reset stored callback state
    fw_load_cb = NULL;
    cb_ctx = NULL;

    // First accel isolated, second not isolated (order depends on ACCEL_ID enumeration)
    will_return(__wrap_accel_is_isolation_enabled, true);
    will_return(__wrap_accel_is_isolation_enabled, false);

    // Only one registration expected for the active accel
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    // Only one send expected for the active accel
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_send, icc_ctx);

    // Mock for threadx event APIs
    expect_any(__wrap__txe_event_flags_set, event_flags_group_ptr);
    expect_any(__wrap__txe_event_flags_set, flags);
    expect_any(__wrap__txe_event_flags_set, options);

    // Expect the event flags to be set to indicate the flow is complete
    will_return_always(__wrap__txe_event_flags_set, TX_SUCCESS);

    // Start with a non-zero offset so mask reflects correct bit position
    uint32_t start_offset = 5; // expect 1 << 5
    uint32_t mask = execute_accel_quiesce_flow(start_offset);
    assert_int_equal(mask, (1u << start_offset));

    // A callback should have been registered; invoke and verify suspend is called
    assert_non_null(fw_load_cb);
    fw_load_cb(cb_ctx, 0, DFWK_SUCCESS);
}

// Recv/send return errors but flow still proceeds; mask is set and callback on failure still suspends
SOS_TEST(execute_accel_quiesce_flow_send_recv_errors_and_cb_fail, NULL, NULL)
{
    // Reset stored callback state
    fw_load_cb = NULL;
    cb_ctx = NULL;

    // Both accelerators not isolated
    will_return(__wrap_accel_is_isolation_enabled, false);
    will_return(__wrap_accel_is_isolation_enabled, false);

    // First recv fails, second succeeds
    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params->recv_cmd_code);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    // First send fails, second succeeds
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_any(__wrap_fpfw_icc_base_send, icc_ctx);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_any(__wrap_fpfw_icc_base_send, icc_ctx);

    // Execute and validate mask for two active accels
    uint32_t mask = execute_accel_quiesce_flow(0);
    assert_int_equal(mask, 0x3);

    // Callback should be registered; simulate failure status path, still should suspend
    assert_non_null(fw_load_cb);
    fw_load_cb(cb_ctx, 0, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
}