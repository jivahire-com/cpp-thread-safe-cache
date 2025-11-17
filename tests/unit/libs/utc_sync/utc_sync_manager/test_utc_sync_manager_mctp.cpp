//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_utc_sync_manager_mctp.cpp
 * @brief Unit tests for utc_sync_manager_mctp.c public API functions.
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>
#include <error_handler.h>
#include <fpfw_mctp.h>
#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <utc_common.h>
#include <utc_sync_manager_lib.h>
#include <utc_sync_manager_mctp_i.h>

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_SYS_COUNT_FREQ_HZ   (1000000000ULL)
#define TEST_SYS_COUNT_INCREMENT (TEST_SYS_COUNT_FREQ_HZ / 4)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint64_t test_system_count = 0;

static fpfw_mctp test_mctp_ctx = {};
static fpfw_mctp_recv_msg_info test_recv_msg_info = {};

/*------------- Functions ----------------*/

fpfw_status_t __wrap_fpfw_mctp_send_msg(fpfw_mctp* mctp_ctx, fpfw_mctp_send_msg_info* send_msg_info)
{
    check_expected_ptr(mctp_ctx);
    check_expected_ptr(send_msg_info);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_mctp_recv_msg(fpfw_mctp* mctp_ctx, fpfw_mctp_recv_msg_info* recv_msg_info)
{
    check_expected_ptr(mctp_ctx);
    check_expected_ptr(recv_msg_info);
    return mock_type(fpfw_status_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

uint64_t __wrap_gtimer_prodfw_get_counter(void)
{
    uint64_t ret = test_system_count;
    test_system_count += TEST_SYS_COUNT_INCREMENT;
    return ret;
}

void __wrap_utc_sync_manager_notify_timestamp_ready(const utc_timestamp_bundle_t* bundle)
{
    check_expected_ptr(bundle);
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    // Init any common dependencies test cases will need
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);
    utc_sync_manager_mctp_init(&test_mctp_ctx);

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);

    // Reset the recv pending flag by completing it, but with a failed status
    on_mctp_msg_receive(&test_recv_msg_info, FPFW_STATUS_FAIL);

    return 0;
}

//
// Test Public Functions
//

TEST_FUNCTION(test_utc_sync_manager_mctp_init, NULL, NULL)
{
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_SYS_COUNT_FREQ_HZ);
    utc_sync_manager_mctp_init(&test_mctp_ctx);
}

TEST_FUNCTION(test_utc_sync_manager_mctp_init_fail, NULL, NULL)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_STATUS_FAIL);

    if (!set_error_handler_return())
    {
        utc_sync_manager_mctp_init(NULL);
    }
}

TEST_FUNCTION(test_utc_sync_manager_request_utc_timestamp, test_setup, test_teardown)
{
    // Test that a request is successful when we need to queue a new mctp receive

    // Setup expectations for queueing a new mctp receive
    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_recv_msg, recv_msg_info);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_SUCCESS);

    // Setup expectations for sending a new mctp send
    expect_value(__wrap_fpfw_mctp_send_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_send_msg, send_msg_info);
    will_return(__wrap_fpfw_mctp_send_msg, FPFW_STATUS_SUCCESS);

    fpfw_status_t sc = utc_sync_manager_request_utc_timestamp();

    assert_true(sc == FPFW_STATUS_SUCCESS);

    // Test that a request is successful when we already have a mctp receive setup
    // and we successfully send a new request

    // Setup expectations for sending a new mctp send
    expect_value(__wrap_fpfw_mctp_send_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_send_msg, send_msg_info);
    will_return(__wrap_fpfw_mctp_send_msg, FPFW_STATUS_SUCCESS);

    sc = utc_sync_manager_request_utc_timestamp();

    assert_true(sc == FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_request_utc_timestamp_fail, test_setup, test_teardown)
{
    // Test that a request is failed to setup a mctp receive

    // Setup expectations for queueing a new mctp receive
    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_recv_msg, recv_msg_info);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_FAIL);

    fpfw_status_t sc = utc_sync_manager_request_utc_timestamp();

    assert_true(sc == FPFW_STATUS_FAIL);

    // Test that a request is setup a recv successfully but failed to send

    // Setup expectations for queueing a new mctp receive
    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_recv_msg, recv_msg_info);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_SUCCESS);

    // Setup expectations for sending a new mctp send
    expect_value(__wrap_fpfw_mctp_send_msg, mctp_ctx, &test_mctp_ctx);
    expect_any(__wrap_fpfw_mctp_send_msg, send_msg_info);
    will_return(__wrap_fpfw_mctp_send_msg, FPFW_STATUS_FAIL);

    sc = utc_sync_manager_request_utc_timestamp();

    assert_true(sc == FPFW_STATUS_FAIL);
}

//
// Test Private Functions
//

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_success_path, test_setup, test_teardown)
{
    // Setup a valid response message that will exercise the synchronized time calculation
    utc_sync_manager_mctp_message_receive_t valid_message = {
        .header =
            {
                .base_header =
                    {
                        .msg_type = MCTP_MVDP_MSG_TYPE,
                        .pci_vendor_id = {MCTP_MVDP_PCI_VENDOR_ID_0, MCTP_MVDP_PCI_VENDOR_ID_1},
                        .escape_seq1 = MCTP_MVDP_ESCAPE_SEQ_0,
                        .escape_seq2 = MCTP_MVDP_ESCAPE_SEQ_1,
                        .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID,
                        .protocol_version = {MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_0, MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_1},
                        .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID,
                    },
                .completion_code = MCTP_MVDP_COMPLETION_CODE_SUCCESS,
            },
        .t0 = 1000, // Client send time
        .t1 = 1002, // Server receive time (2ms network delay)
        .t2 = 1003  // Server send time (1ms processing)
        // t3 will be calculated when the function is called
    };

    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &valid_message,
        .msg_buffer_size = sizeof(valid_message),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    // Expect the callback to UTC sync manager with calculated timestamp
    expect_any(__wrap_utc_sync_manager_notify_timestamp_ready, bundle);

    // Call the function - this will exercise:
    // - Message validation logic
    // - get_synchronized_ntp_time_epoch_ms() calculation
    // - get_current_unix_time_estimate_epoch_ms() for t3
    // - get_system_counter() call
    // - NTP sync context update
    on_mctp_msg_receive(&recv_info, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_null_msg, test_setup, test_teardown)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_STATUS_FAIL);
    if (!set_error_handler_return())
    {
        on_mctp_msg_receive(NULL, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_failed_status, test_setup, test_teardown)
{
    utc_sync_manager_mctp_message_receive_t dummy_message = {};

    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &dummy_message,
        .msg_buffer_size = sizeof(dummy_message),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    // Test that function handles failed receive status gracefully
    // Should not call notify function
    on_mctp_msg_receive(&recv_info, FPFW_STATUS_FAIL);

    // Function should return early without calling notify
    // No expectations needed since function should exit early
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_invalid_command_set, test_setup, test_teardown)
{
    // Test message validation branch - invalid command set
    utc_sync_manager_mctp_message_receive_t invalid_message = {
        .header =
            {
                .base_header =
                    {
                        .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID + 1,
                        .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID,
                    },
                .completion_code = MCTP_MVDP_COMPLETION_CODE_SUCCESS,
            },
    };

    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &invalid_message,
        .msg_buffer_size = sizeof(invalid_message),
        .msg_type = MCTP_MVDP_MSG_TYPE,
    };

    // Should not call notify function due to validation failure
    on_mctp_msg_receive(&recv_info, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_invalid_command, test_setup, test_teardown)
{
    // Test message validation branch - invalid command
    utc_sync_manager_mctp_message_receive_t invalid_message = {
        .header =
            {
                .base_header =
                    {
                        .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID,
                        .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID + 1,
                    },
                .completion_code = MCTP_MVDP_COMPLETION_CODE_SUCCESS,
            },
    };

    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &invalid_message,
        .msg_buffer_size = sizeof(invalid_message),
        .msg_type = MCTP_MVDP_MSG_TYPE,
    };

    on_mctp_msg_receive(&recv_info, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_msg_receive_invalid_completion_code, test_setup, test_teardown)
{
    // Test message validation branch - invalid completion code
    utc_sync_manager_mctp_message_receive_t invalid_message = {
        .header =
            {
                .base_header =
                    {
                        .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID,
                        .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID,
                    },
                .completion_code = MCTP_MVDP_COMPLETION_CODE_SUCCESS + 1,
            },
    };

    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &invalid_message,
        .msg_buffer_size = sizeof(invalid_message),
        .msg_type = MCTP_MVDP_MSG_TYPE,
    };

    on_mctp_msg_receive(&recv_info, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_send_msg_complete_success, test_setup, test_teardown)
{
    // Test send completion callback with success status
    fpfw_mctp_send_msg_info send_info = {
        .dest_eid = MCTP_BMC_EID,
        .msg_tag = FPFW_MCTP_MSG_TAG_0,
        .tag_owner = true,
        .msg = (void*)0x12345678,
        .msg_size = sizeof(utc_sync_manager_mctp_message_send_t),
        .cb = on_mctp_send_msg_complete,
        .cb_ctx = NULL,
    };

    // Function primarily logs completion - ensure it doesn't crash
    on_mctp_send_msg_complete(&send_info, FPFW_STATUS_SUCCESS);

    // Test passes if no assertion failures occur
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_send_msg_complete_null_msg, test_setup, test_teardown)
{

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_STATUS_FAIL);
    if (!set_error_handler_return())
    {
        on_mctp_send_msg_complete(NULL, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(test_utc_sync_manager_on_mctp_send_msg_complete_failure, test_setup, test_teardown)
{
    // Test send completion callback with failure status
    fpfw_mctp_send_msg_info send_info = {
        .dest_eid = MCTP_BMC_EID,
        .msg_tag = FPFW_MCTP_MSG_TAG_1,
        .tag_owner = false,
        .msg = (void*)0x87654321,
        .msg_size = sizeof(utc_sync_manager_mctp_message_send_t),
        .cb = on_mctp_send_msg_complete,
        .cb_ctx = NULL,
    };

    // Function primarily logs completion - ensure it doesn't crash
    on_mctp_send_msg_complete(&send_info, FPFW_STATUS_FAIL);

    // Test passes if no assertion failures occur
}

TEST_FUNCTION(test_utc_sync_manager_mctp_send_success, test_setup, test_teardown)
{
    // Test successful send operation
    // This will exercise get_current_unix_time_estimate_epoch_ms() to set t0

    utc_sync_manager_mctp_message_send_t send_message = {
        .base_header =
            {
                .msg_type = MCTP_MVDP_MSG_TYPE,
                .pci_vendor_id = {MCTP_MVDP_PCI_VENDOR_ID_0, MCTP_MVDP_PCI_VENDOR_ID_1},
                .escape_seq1 = MCTP_MVDP_ESCAPE_SEQ_0,
                .escape_seq2 = MCTP_MVDP_ESCAPE_SEQ_1,
                .command_set = MCTP_MVDP_1P_BMC_CMD_SET_ID,
                .protocol_version = {MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_0, MCTP_MVDP_1P_BMC_CMD_SET_PROTO_VER_1},
                .command = MCTP_MVDP_1P_BMC_CMD_SET_NTP_CMD_ID,
            },
        .t0 = 0,
    };

    fpfw_mctp_send_msg_info send_info = {
        .dest_eid = MCTP_BMC_EID,
        .msg_tag = FPFW_MCTP_MSG_TAG_0,
        .tag_owner = true,
        .msg = &send_message,
        .msg_size = sizeof(send_message),
        .cb = on_mctp_send_msg_complete,
        .cb_ctx = NULL,
    };

    // Setup expectation for underlying MCTP send
    expect_value(__wrap_fpfw_mctp_send_msg, mctp_ctx, &test_mctp_ctx);
    expect_value(__wrap_fpfw_mctp_send_msg, send_msg_info, &send_info);
    will_return(__wrap_fpfw_mctp_send_msg, FPFW_STATUS_SUCCESS);

    fpfw_status_t result = utc_sync_manager_mctp_send(&send_info);

    assert_int_equal(result, FPFW_STATUS_SUCCESS);

    // Verify that t0 timestamp was filled in (should be non-zero after time estimation)
    assert_true(send_message.t0 > 0);
}

TEST_FUNCTION(test_utc_sync_manager_mctp_send_null_ctx, test_setup, test_teardown)
{

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_STATUS_FAIL);
    if (!set_error_handler_return())
    {
        utc_sync_manager_mctp_send(NULL);
    }
}

TEST_FUNCTION(test_utc_sync_manager_mctp_send_failure, test_setup, test_teardown)
{
    // Test failed send operation
    utc_sync_manager_mctp_message_send_t send_message = {{0}};

    fpfw_mctp_send_msg_info send_info = {
        .dest_eid = MCTP_BMC_EID,
        .msg_tag = FPFW_MCTP_MSG_TAG_0,
        .tag_owner = true,
        .msg = &send_message,
        .msg_size = sizeof(send_message),
        .cb = on_mctp_send_msg_complete,
        .cb_ctx = NULL,
    };

    // Setup expectation for underlying MCTP send to fail
    expect_value(__wrap_fpfw_mctp_send_msg, mctp_ctx, &test_mctp_ctx);
    expect_value(__wrap_fpfw_mctp_send_msg, send_msg_info, &send_info);
    will_return(__wrap_fpfw_mctp_send_msg, FPFW_STATUS_FAIL);

    fpfw_status_t result = utc_sync_manager_mctp_send(&send_info);

    assert_int_equal(result, FPFW_STATUS_FAIL);

    // Verify that t0 timestamp was still filled in even on failure
    assert_true(send_message.t0 > 0);
}

TEST_FUNCTION(test_utc_sync_manager_mctp_read_success, test_setup, test_teardown)
{
    // Test successful read setup when no read is pending
    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &test_recv_msg_info,
        .msg_buffer_size = sizeof(utc_sync_manager_mctp_message_receive_t),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    // Setup expectation for underlying MCTP read
    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_value(__wrap_fpfw_mctp_recv_msg, recv_msg_info, &recv_info);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_SUCCESS);

    fpfw_status_t result = utc_sync_manager_mctp_read(&recv_info);

    assert_int_equal(result, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_utc_sync_manager_mctp_read_null_ctx, test_setup, test_teardown)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_STATUS_FAIL);
    if (!set_error_handler_return())
    {
        utc_sync_manager_mctp_read(NULL);
    }
}

TEST_FUNCTION(test_utc_sync_manager_mctp_read_already_pending, test_setup, test_teardown)
{
    // Setup first read to make it pending
    fpfw_mctp_recv_msg_info recv_info1 = {
        .msg = &test_recv_msg_info,
        .msg_buffer_size = sizeof(utc_sync_manager_mctp_message_receive_t),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_value(__wrap_fpfw_mctp_recv_msg, recv_msg_info, &recv_info1);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_SUCCESS);

    fpfw_status_t result1 = utc_sync_manager_mctp_read(&recv_info1);
    assert_int_equal(result1, FPFW_STATUS_SUCCESS);

    // Now try to setup another read while first is pending
    fpfw_mctp_recv_msg_info recv_info2 = {
        .msg = &test_recv_msg_info,
        .msg_buffer_size = sizeof(utc_sync_manager_mctp_message_receive_t),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    fpfw_status_t result2 = utc_sync_manager_mctp_read(&recv_info2);

    // Should return PENDING since a read is already queued
    assert_int_equal(result2, FPFW_STATUS_PENDING);
}

TEST_FUNCTION(test_utc_sync_manager_mctp_read_failure, test_setup, test_teardown)
{
    // Test failed read setup
    fpfw_mctp_recv_msg_info recv_info = {
        .msg = &test_recv_msg_info,
        .msg_buffer_size = sizeof(utc_sync_manager_mctp_message_receive_t),
        .msg_type = MCTP_MVDP_MSG_TYPE,
        .cb = on_mctp_msg_receive,
        .cb_ctx = NULL,
    };

    // Setup expectation for underlying MCTP read to fail
    expect_value(__wrap_fpfw_mctp_recv_msg, mctp_ctx, &test_mctp_ctx);
    expect_value(__wrap_fpfw_mctp_recv_msg, recv_msg_info, &recv_info);
    will_return(__wrap_fpfw_mctp_recv_msg, FPFW_STATUS_FAIL);

    fpfw_status_t result = utc_sync_manager_mctp_read(&recv_info);

    assert_int_equal(result, FPFW_STATUS_FAIL);
}

} // extern "C"
