//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_corebits.cpp
 * Corebits tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwLock.h>             // for FPFW_LOCK, FpFwLockAcquire, FpFwLockRelease
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <boot_status.h>          // for boot_status_notify
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_sync, fpfw...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <idsw_kng.h>             // for idsw_get_cpu_type
#include <stdbool.h>
#include <system_info.h> // for system_info_is_init_complete

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static icc_base_send_complete_notify s_stored_send_cb = NULL;
static void* s_stored_send_context = NULL;
static uint32_t test_boot_status_ex = 0xFFFFFFFFUL;

/*------------- Mock Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);

    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)payload_buffer;
    assert_int_equal(output_message->header.cmd, HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY);
    //! Store the boot status_ex value for verification
    test_boot_status_ex = output_message->boot_stat_notif.boot_status_ex.boot_status_int;
    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(icc_ctx);
    assert_non_null(params);

    // save the passed in callback
    s_stored_send_cb = params->cb;
    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)params->payload_buffer;
    assert_int_equal(output_message->header.cmd, HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY);
    //! Store the boot status_ex value for verification
    test_boot_status_ex = output_message->boot_stat_notif.boot_status_ex.boot_status_int;
    s_stored_send_context = params->cb_ctx;

    function_called();
    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_init_complete()
{
    return mock_type(bool);
    ;
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return ((FPFW_LOCK_STATE)0);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

static int setup(void** state)
{
    FPFW_UNUSED(state);
    // Initialize any resources needed for the tests
    boot_status_init((fpfw_icc_base_ctx_t*)0x1234);
    return 0;
}

static int cleanup(void** state)
{
    FPFW_UNUSED(state);
    // Reset any resources needed for the tests
    s_stored_send_cb = NULL;
    s_stored_send_context = NULL;
    test_boot_status_ex = 0xFFFFFFFFUL;
    return 0;
}

/*------------- Test Cases ----------------*/
/**
 * @brief Verify boot status code based on current core.
 * SCP's boot status codes range from 0x40 upto but not including scp max (or 0x5F, whichever is smaller)
 * MCP's boot status codes range from 0x60 upto but not including mcp max (or 0x7F, whichever is smaller)
 * MSCP boot status range :0b01000000 upto but not including 0b01111111
 */
TEST_FUNCTION(test_boot_status_notify_invalid_args, nullptr, nullptr)
{
    //! Set up expectations for SCP
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 4);
    //! Call FUT & Expect failures
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_MAX), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_MCP_OK), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify((boot_status_code_t)0), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_MAX), FPFW_STATUS_INVALID_ARGS);

    //! Set up expectations for MCP
    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 4);
    //! Call FUT & Expect failures
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_MAX), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify((boot_status_code_t)0), FPFW_STATUS_INVALID_ARGS);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_MAX), FPFW_STATUS_INVALID_ARGS);
}

/**
 * @brief Test for current state of the system & verify all pre reqs are met
 * for boot status notify. This includes initializing the icc base ctx,
 * hsp presence.
 */
TEST_FUNCTION(test_boot_status_notify_invalid_state, nullptr, nullptr)
{
    //! Set up expectations, hsp is not present & icc ctx is not set
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_system_info_is_hsp_present, false);
    //! Call FUT & Expect failure
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_INVALID_STATE);

    //! Set up expectations, but hsp is present but icc ctx is not set
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_system_info_is_hsp_present, true);
    //! Call FUT & Expect failure
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_INVALID_STATE);
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during init
 *
 */
TEST_FUNCTION(test_boot_status_notify_sync, setup, cleanup)
{
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 2);
    will_return_count(__wrap_system_info_is_hsp_present, true, 2);
    will_return_count(__wrap_system_info_is_init_complete, false, 2);

    //! Simulate icc base sync send failure
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect failure
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);

    //! Simulate icc base sync send success
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect success
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_SUCCESS);
}

/**
 * @brief Test to verify status_ex value is updated correctly based on the
 * type of status code
 */
TEST_FUNCTION(test_boot_status_notify_status_ex_update, setup, cleanup)
{
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase & Simulate icc base sync send success
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 2);
    will_return_count(__wrap_system_info_is_hsp_present, true, 2);
    will_return_count(__wrap_system_info_is_init_complete, false, 2);
    will_return_count(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS, 2);

    //! Call FUT & Expect success
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_SUCCESS);
    //! Verify the boot status_ex value is set to complete for progress codes, indicating success
    assert_int_equal(test_boot_status_ex, HSP_MAILBOX_BOOT_STATUS_EX_COMPLETE);

    //! Call FUT & Expect success
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_E_BOOT_CONFIG), FPFW_STATUS_SUCCESS);
    //! Verify the boot status_ex value is fatal for error status codes
    assert_int_equal(test_boot_status_ex, HSP_MAILBOX_BOOT_STATUS_EX_FATAL);
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during runtime
 *
 */
TEST_FUNCTION(test_boot_status_notify_async, setup, cleanup)
{
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 4);
    will_return_count(__wrap_system_info_is_hsp_present, true, 4);
    will_return_count(__wrap_system_info_is_init_complete, true, 3);

    //! Simulate icc base async send failure
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect failure
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_SUCCESS);
    //! @note: The success here indicates the async send request was queued in the dfwk successfully
    //! The actual send completion is verified in the send completion callback
    //! If the callback isn't raised, the next invocation of boot_status_notify will fail with busy

    //! Call FUT & Expect failure, prev send wasn't completed yet
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_BUSY);

    //! Simulate raising the send completion callback
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    assert_int_equal(boot_status_notify(BOOT_STATUS_CODE_SCP_OK), FPFW_STATUS_SUCCESS);
}

} // extern "C"