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

#include <CMockaWrapper.h>        // for check_expected, check_expected_ptr
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <hsp_firmware_headers.h>
#include <icc_platform_defines.h>
#include <startup_shutdown.h>   // for sos_queue_entry_t, sos_queue_start_phase
#include <startup_shutdown_i.h> // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

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
    const startup_shutdown_boot_stage_t* p_stages = __real_sos_core_boot_stages();
    // ensure the boot stages are returned
    assert_non_null(p_stages);
    // check we have stages from both boot phases
    assert_int_equal(p_stages[0].phase, STARTUP_PHASE_MSCP_ASYNC);
    assert_int_equal(p_stages[__real_sos_core_boot_stage_count() - 1].phase, STARTUP_PHASE_AP_ASYNC);
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
    sos_stage_timeout_t current_stage = {.stage_category = BOOT_STAGE,
                                         .operation_stage.boot = STARTUP_BL31_LOAD,
                                         .timeout_ms = test_time_out};

    will_return_always(__wrap_sos_core_boot_stage_count, __real_sos_core_boot_stage_count());

    // call the function
    sos_boot_timeout_override(current_stage);
    unsigned int return_timeout = sos_boot_timeout(current_stage);

    // check timeout get updated.
    assert_int_equal(return_timeout, test_time_out);
}

// test for sos_shutdown_timeout
SOS_TEST(sos_shutdown_timeout, NULL, NULL)
{
    unsigned int test_time_out = 100 * 1000;
    sos_stage_timeout_t current_stage = {.stage_category = SHUTDOWN_STAGE,
                                         .operation_stage.shutdown = MSCP_SUBSYS_RESET,
                                         .timeout_ms = test_time_out};

    // call the function
    sos_shutdown_timeout_override(current_stage);
    unsigned int return_timeout = sos_shutdown_timeout(current_stage);

    // check timeout get updated.
    assert_int_equal(return_timeout, test_time_out);
}

// test for sos_core_shutdown_handler SHUTDOWN case
SOS_TEST(sos_core_shutdown_handler_shutdown, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx);

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

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);
    sos_icc_init(test_icc_ctx);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);
    shutdown_req_cb(request, DFWK_SUCCESS);
}

// test for prepare_reset_recv_cb
SOS_TEST(prepare_reset_recv_cb, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);
    sos_icc_init(test_icc_ctx);

    kng_hsp_mailbox_cmd_core_reset_complete_notify test_send_params = {
        .header.cmd = HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_RSP,
    };

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_ctx);

    prepare_reset_recv_cb(&test_send_params, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_RSP, DFWK_SUCCESS);
}

// test for sos_core_shutdown_handler COLD_RESET case
SOS_TEST(sos_core_shutdown_handler_cold_reset, NULL, NULL)
{
    fpfw_icc_base_ctx_t* test_icc_ctx = (fpfw_icc_base_ctx_t*)0xBADDBEEF;
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0xBADDBEEF);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_CMD_PREPARE_FOR_CORE_RESET_REQ);
    will_return(__wrap_fpfw_icc_base_recv, DFWK_SUCCESS);

    sos_icc_init(test_icc_ctx);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, test_icc_ctx);

    sos_core_shutdown_handler(COLD_RESET);
}