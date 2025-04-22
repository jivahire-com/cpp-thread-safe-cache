//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_boot_status.cpp
 * Implements test cases for Boot Status APIs
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
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <system_info.h> // for system_info_is_init_complete

/*-- Symbolic Constant Macros (defines) --*/
#define BUG_CHECK_RETURN_VALUE 0x1
#define BUGCHECK_MOCK_RETURN   (setjmp(cd_mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static icc_base_send_complete_notify s_stored_send_cb = NULL;
static void* s_stored_send_context = NULL;
jmp_buf cd_mock_jump_buf;

/*------------- Mock Functions ----------------*/
fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    assert_non_null(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);

    kng_hsp_mailbox_msg* output_message = (kng_hsp_mailbox_msg*)payload_buffer;
    assert_int_equal(output_message->header.cmd, HSP_MAILBOX_CMD_BOOT_STATUS_EXTD_NOTIFY);
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
    s_stored_send_context = params->cb_ctx;

    function_called();
    return mock_type(fpfw_status_t);
}

bool __wrap_system_info_is_init_complete()
{
    return mock_type(bool);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

bool __wrap_system_info_is_hsp_present()
{
    return mock_type(bool);
}

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, BUG_CHECK_RETURN_VALUE);
}

/*------------- Functions ----------------*/
static int setup(void** state)
{
    FPFW_UNUSED(state);
    uint32_t dummy = 0;
    boot_status_icc_ctx_t boot_status_ctx = {0};
    //! Initialize the ctx to be a dummy non null for tests
    boot_status_ctx.hsp_mbx_ctx = (fpfw_icc_base_ctx_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[0].lfifo_mbx_ctx = (fpfw_icc_base_ctx_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[0].send_params = (fpfw_icc_base_send_req_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[0].recv_params = (fpfw_icc_base_recv_req_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[1].lfifo_mbx_ctx = (fpfw_icc_base_ctx_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[1].send_params = (fpfw_icc_base_send_req_t*)&dummy;
    boot_status_ctx.accel_icc_ctx[1].recv_params = (fpfw_icc_base_recv_req_t*)&dummy;

    // Initialize any resources needed for the tests
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    boot_status_init(&boot_status_ctx);
    return 0;
}

static int cleanup(void** state)
{
    FPFW_UNUSED(state);
    // Reset any resources needed for the tests
    s_stored_send_cb = NULL;
    s_stored_send_context = NULL;
    boot_status_reset();
    return 0;
}

static void test_boot_status_notify_extd_cb(void* ctx)
{
    FPFW_UNUSED(ctx);
    function_called();
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_boot_status_notify_extd_invalid_args, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    mscp_boot_status_code_t unused = MSCP_BOOT_STATUS_CODE_UNUSED; //! hsp doesn't check boot status
    uint32_t err_boot_status_ex = 0;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        boot_status_notify_extd(NULL, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! cmp group HSP is not supported
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_HSP, MSCP_GENERIC, SCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! Comp grp & instance don't match
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, MCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! invalid subgroup, out of range
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_SUBGROUP_MAX, SCP_PRIMARY);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! invalid instance, out of range instance
        err_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, MAX_COMPONENT_INSTANCE);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! out of range status, not supported for SCP led status
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_CDED1_HW_FAULT + 1);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
    if (!bugcheck_mock_return())
    {
        //! led status (is for die1) mismatch for component grp SCP_PRIMARY (die 0)
        err_boot_status_ex =
            GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP1_MESH_INIT_START);
        boot_status_notify_extd(&test_req_mem, unused, err_boot_status_ex);
    }
}

/**
 * @brief Test for current state of the system & verify all pre reqs are met
 * for boot status notify. This includes initializing the icc base ctx,
 * hsp presence.
 */
TEST_FUNCTION(test_boot_status_notify_extd_invalid_state, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    uint32_t valid_boot_status_ex = GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY);

    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! hsp is not present & no init
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! boot status init hasn't been called prior, but hsp is present
    will_return(__wrap_system_info_is_hsp_present, true);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during init
 *
 */
TEST_FUNCTION(test_boot_status_notify_extd_sync, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;

    //! Set up expectations, module initialized, scp core, hsp is present
    uint32_t valid_boot_status_ex =
        GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base sync send failure
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! Simulate icc base sync send success
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_function_call(test_boot_status_notify_extd_cb);
    //! Call FUT & Expect success
    boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
}

/**
 * @brief Test to verify the boot status notify API behavior when invoked during runtime
 *
 */
TEST_FUNCTION(test_boot_status_notify_extd_async, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    uint32_t valid_boot_status_ex =
        GEN_BOOT_STATUS_EX_LED_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, SCP_PRIMARY, BOOT_STATUS_CODE_SCP0_OK);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base async send failure
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    }

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    boot_status_notify_extd(&test_req_mem, MSCP_BOOT_STATUS_CODE_SCP_OK, valid_boot_status_ex);
    //! @note: The success here indicates the async send request was queued in the dfwk successfully
    //! The actual send completion is verified in the send completion callback
    //! Simulate raising the ICC internal send completion callback, this will call the user provided cb
    expect_function_call(test_boot_status_notify_extd_cb);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_post_led_status_invalid_args, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    //! Set up expectations for SCP
    will_return_count(__wrap_idsw_get_cpu_type, CPU_SCP, 3);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    led_status_codes_t led_status = LED_STATUS_CODE_MAX;

    //! Call FUT & Expect failures
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        //! Null param
        post_led_status(NULL, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! led status is invalid & out of range
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! Led status doesn't match current core supported status
        //! current core is SCP, led status is for MCP
        led_status = LED_STATUS_CODE_MCP_OK;
        post_led_status(&test_req_mem, led_status);
    }

    //! Update current core to MCP for further tests
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);
    if (!bugcheck_mock_return())
    {
        //! led status applicable for only SCP core, but current core is MCP
        led_status = LED_STATUS_CODE_SCP_MESH_INIT_START;
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! Again only SCP core collects the led status from accel cores, current core is MCP
        led_status = LED_STATUS_CODE_SDM_BOOT_COMPLETE;
        post_led_status(&test_req_mem, led_status);
    }
    if (!bugcheck_mock_return())
    {
        //! SCP error code, but current core is MCP
        led_status = LED_STATUS_CODE_SCP_E_DDR0_TRAINING;
        post_led_status(&test_req_mem, led_status);
    }
}

TEST_FUNCTION(test_post_led_status_invalid_state, nullptr, nullptr)
{
    boot_status_req_t test_req_mem;
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;

    //! Set up expectations for SCP
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! hsp is not present & boot status init not called
    will_return(__wrap_system_info_is_hsp_present, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! boot status init hasn't been called prior, but hsp is present
    will_return(__wrap_system_info_is_hsp_present, true);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }
}

TEST_FUNCTION(test_post_led_status_sync, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;

    //! Set up expectations, module initialized, scp core, hsp is present
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;

    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is still in init phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, false);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base sync send failure
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! Simulate icc base sync send success
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_function_call(test_boot_status_notify_extd_cb);
    //! Call FUT & Expect success
    post_led_status(&test_req_mem, led_status);
}

TEST_FUNCTION(test_post_led_status_async, setup, cleanup)
{
    boot_status_req_t test_req_mem;
    test_req_mem.cb = test_boot_status_notify_extd_cb;
    test_req_mem.cb_ctx = NULL;
    led_status_codes_t led_status = LED_STATUS_CODE_SCP_OK;
    //! Set up expectations, module initialized, scp core, hsp is present
    //! system is in runtime phase
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_is_hsp_present, true);
    will_return_always(__wrap_system_info_is_init_complete, true);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    //! Simulate icc base async send failure
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_ASYNC_SEND_REQ_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect failure
    if (!bugcheck_mock_return())
    {
        post_led_status(&test_req_mem, led_status);
    }

    //! Simulate icc base async send success
    will_return(__wrap_fpfw_icc_base_send, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);
    //! Call FUT & Expect success
    post_led_status(&test_req_mem, led_status);
    //! @note: The success here indicates the async send request was queued in the dfwk successfully
    //! The actual send completion is verified in the send completion callback
    //! Simulate raising the ICC internal send completion callback, this will call the user provided cb
    expect_function_call(test_boot_status_notify_extd_cb);
    s_stored_send_cb(s_stored_send_context, FPFW_STATUS_SUCCESS);
}

} // extern "C"
