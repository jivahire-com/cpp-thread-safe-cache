//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mts_platform.cpp
 * Tests platform specific functions for the Message Transfer Service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "mts_mock.h"

#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <data_collection_protocol.h> // for dcp_msg_t, dcp_msg_hdr_t
#include <fpfw_icc_base.h>            // for fpfw_icc_base_ctx_t, fpfw_i...
#include <fpfw_status.h>              // for FPFW_STATUS_SUCCESS, FPFW_S...
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <message_transfer_service.h>      // for mts_config_t
#include <message_transfer_service_i.h>    // for NEW_OUTBOUND_MSG
#include <mts_client.h>                    // for MTS_CLIENT_ID_DCP_SVC, MTS_...
#include <mts_platform_definitions.h>      // for MTS_PLATFORM_CORE_PRIMARY
#include <mts_platform_implementation_i.h> // for mts_platform_get_transport_i...
#include <mts_platform_specialization.h>   // for trp_msg_t, p_trp_icc_endpoint_t
#include <stdint.h>                        // for uint8_t
#include <stdio.h>                         // for sprintf
#include <transfer_relay_i.h>              // for transfer_rly_should_broadca...
#include <transfer_relay_protocol.h>       // for trp_msg_t, trp_route_t, trp...
#include <tx_api.h>                        // for TX_SUCCESS, TX_OR, TX_NO_WAIT
}

/*-- Symbolic Constant Macros (defines) --*/
#define FAKE_PAYLOAD_SIZE      (22)
#define FAKE_CLIENT_ID         (17)
#define FAKE_MAX_EXCEEDED_SIZE (600)

#define TRP_SEQ_NUM_NOT_INIT  (0x11)
#define TRP_SEQ_NUM_WITH_INIT (0x8100)
#define TRP_NEW_SEQ_NUM       (0x7D)

#define DCP_SEQ_NUM     (0x3A)
#define DCP_NEW_SEQ_NUM (0x24)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(test_mts_platform_get_transport_info, nullptr, nullptr)
{
    mts_platform_transport_info_t info;
    mts_platform_get_transport_info(TRP_TRANSPORT_TYPE_ICC_ARM_MHU, &info);

    assert_int_equal(info.header_size, sizeof(icc_mhu_header_t));
    assert_int_equal(info.icc_dcp_command, ICC_COMMAND_DCP_MSG);
    assert_int_equal(info.icc_trp_command, ICC_COMMAND_TRP_MSG);

    mts_platform_get_transport_info(TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX, &info);

    assert_int_equal(info.header_size, sizeof(large_fifo_mailbox_msg_header));
    assert_int_equal(info.icc_dcp_command, LARGE_FIFO_MAILBOX_MSG_DCP);
    assert_int_equal(info.icc_trp_command, LARGE_FIFO_MAILBOX_MSG_TRP);

    // test out of range
    expect_value(__wrap_FpFwAssert, expression, false);
    mts_platform_get_transport_info(0xFF, &info);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_mhu, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    // send a DCP message, incoming endpoint is null, verify seq number is incremented
    // verify outgoing message is dcp header with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    icc_mhu_header_t* icc_mhu_hdr = (icc_mhu_header_t*)outgoing_icc_buffer;
    assert_int_equal(icc_mhu_hdr->msg_header.command, ICC_COMMAND_DCP_MSG);
    assert_int_equal(icc_mhu_hdr->msg_header.payload_size, sizeof(dcp_msg_hdr_t) + FAKE_PAYLOAD_SIZE);

    // verify message is located at the correct location after the transport header
    p_dcp_msg_t dcp_msg = (p_dcp_msg_t)(outgoing_icc_buffer + sizeof(icc_mhu_header_t));
    assert_int_equal(dcp_msg->hdr.seq_num, TRP_SEQ_NUM_NOT_INIT);
    assert_int_equal(dcp_msg->hdr.client_id, FAKE_CLIENT_ID);
    assert_int_equal(dcp_msg->hdr.payload_size, FAKE_PAYLOAD_SIZE);

    // not a DCP message, verify outgoing message is trp_msg_t with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;
    trp_msg.hdr.source_seq_num.as_uint16 = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;

    will_return(__wrap_transfer_rly_should_send_trp_msg, true);
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    icc_mhu_hdr = (icc_mhu_header_t*)outgoing_icc_buffer;
    assert_int_equal(icc_mhu_hdr->msg_header.command, ICC_COMMAND_TRP_MSG);
    assert_int_equal(icc_mhu_hdr->msg_header.payload_size, sizeof(trp_msg_hdr_t) + FAKE_PAYLOAD_SIZE);

    p_trp_msg_t trp_msg_out = (p_trp_msg_t)(outgoing_icc_buffer + sizeof(icc_mhu_header_t));
    assert_int_equal(trp_msg_out->hdr.trp_msg_id, TRP_MSG_ID_READ_PACKAGE_RESPONSE);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_large_mbox, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    // send a DCP message, incoming endpoint is null, verify seq number is incremented
    // verify outgoing message is dcp header with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    large_fifo_mailbox_msg_header* icc_lrg_mbox_hdr = (large_fifo_mailbox_msg_header*)outgoing_icc_buffer;
    assert_int_equal(icc_lrg_mbox_hdr->cmd, LARGE_FIFO_MAILBOX_MSG_DCP);

    p_dcp_msg_t dcp_msg = (p_dcp_msg_t)(outgoing_icc_buffer + sizeof(large_fifo_mailbox_msg_header));
    assert_int_equal(dcp_msg->hdr.seq_num, TRP_SEQ_NUM_NOT_INIT);
    assert_int_equal(dcp_msg->hdr.client_id, FAKE_CLIENT_ID);
    assert_int_equal(dcp_msg->hdr.payload_size, FAKE_PAYLOAD_SIZE);

    // not a DCP message, verify outgoing message is trp_msg_t with large_fifo_mailbox_msg_header header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;
    trp_msg.hdr.source_seq_num.as_uint16 = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;

    will_return(__wrap_transfer_rly_should_send_trp_msg, true);
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    icc_lrg_mbox_hdr = (large_fifo_mailbox_msg_header*)outgoing_icc_buffer;
    assert_int_equal(icc_lrg_mbox_hdr->cmd, LARGE_FIFO_MAILBOX_MSG_TRP);

    p_trp_msg_t trp_msg_out = (p_trp_msg_t)(outgoing_icc_buffer + sizeof(large_fifo_mailbox_msg_header));
    assert_int_equal(trp_msg_out->hdr.trp_msg_id, TRP_MSG_ID_READ_PACKAGE_RESPONSE);
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_dropped, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];
    uint8_t expect_zero_buffer[MAX_ICC_BUFFER_SIZE];
    memset(expect_zero_buffer, 0, MAX_ICC_BUFFER_SIZE);

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    // send a DCP message, incoming endpoint is null, verify seq number is incremented
    // verify outgoing message is dcp header with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, false);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // did not send message, verify outgoing buffer is zeroed out
    assert_memory_equal(outgoing_icc_buffer, expect_zero_buffer, sizeof(outgoing_icc_buffer));

    // not a DCP message, verify outgoing message is trp_msg_t with large_fifo_mailbox_msg_header header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;
    trp_msg.hdr.source_seq_num.as_uint16 = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;

    will_return(__wrap_transfer_rly_should_send_trp_msg, false);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // did not send message, verify outgoing buffer is zeroed out
    assert_memory_equal(outgoing_icc_buffer, expect_zero_buffer, sizeof(outgoing_icc_buffer));
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_max_size, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];
    uint8_t expect_zero_buffer[MAX_ICC_BUFFER_SIZE];
    memset(expect_zero_buffer, 0, MAX_ICC_BUFFER_SIZE);

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    // send a DCP message, incoming endpoint is null, verify seq number is incremented
    // verify outgoing message is dcp header with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_MAX_EXCEEDED_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // did not send message, verify outgoing buffer is zeroed out
    assert_memory_equal(outgoing_icc_buffer, expect_zero_buffer, sizeof(outgoing_icc_buffer));

    // also check large mbox max size
    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_LARGE_MBOX;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // did not send message, verify outgoing buffer is zeroed out
    assert_memory_equal(outgoing_icc_buffer, expect_zero_buffer, sizeof(outgoing_icc_buffer));
}

TEST_FUNCTION(test_mts_platform_send_msg_via_transport_error, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];
    uint8_t expect_zero_buffer[MAX_ICC_BUFFER_SIZE];
    memset(expect_zero_buffer, 0, MAX_ICC_BUFFER_SIZE);

    trp_icc_endpoint.base_endpt.transport_type = (trp_transport_type_t)0xFF; // invalid transport type
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    // send a DCP message, incoming endpoint is null, verify seq number is incremented
    // verify outgoing message is dcp header with icc_mhu_header_t header
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);
    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // did not send message, verify outgoing buffer is zeroed out
    assert_memory_equal(outgoing_icc_buffer, expect_zero_buffer, sizeof(outgoing_icc_buffer));

    // icc base send fails, will log trace event
    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU; // valid transport type

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);

    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);
}

/**
 * @brief Test ICC send retry on timeout - verifies retry logic triggers on timeout error
 *
 * This test verifies that when fpfw_icc_base_send_sync returns a timeout error,
 * the function retries up to MTS_ICC_SEND_MAX_RETRIES times with delays between attempts.
 */
TEST_FUNCTION(test_mts_platform_send_msg_via_transport_retry_on_timeout, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);

    // First attempt: timeout error - should trigger retry
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    will_return(__wrap__tx_thread_sleep, TX_SUCCESS); // delay before retry

    // Second attempt: timeout error - should trigger another retry
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    will_return(__wrap__tx_thread_sleep, TX_SUCCESS); // delay before retry

    // Third attempt: success - should exit loop
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);

    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);
    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);

    // Verify message was sent correctly
    icc_mhu_header_t* icc_mhu_hdr = (icc_mhu_header_t*)outgoing_icc_buffer;
    assert_int_equal(icc_mhu_hdr->msg_header.command, ICC_COMMAND_DCP_MSG);
}

/**
 * @brief Test ICC send exhausts all retries on persistent timeout
 *
 * This test verifies that when all retry attempts result in timeout errors,
 * the function logs failures and exits after MTS_ICC_SEND_MAX_RETRIES attempts.
 */
TEST_FUNCTION(test_mts_platform_send_msg_via_transport_retry_exhausted, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);

    // All 3 attempts timeout (MTS_ICC_SEND_MAX_RETRIES = 3)
    // First attempt: timeout
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    will_return(__wrap__tx_thread_sleep, TX_SUCCESS);

    // Second attempt: timeout
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    will_return(__wrap__tx_thread_sleep, TX_SUCCESS);

    // Third attempt: timeout (last attempt, no sleep after)
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SYNC_TIMEOUT_ERR);
    // No sleep expected after last attempt

    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);
    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);
}

/**
 * @brief Test ICC send does not retry on non-timeout errors
 *
 * This test verifies that non-timeout errors (e.g., FPFW_STATUS_FAIL) do not
 * trigger retry logic - the function should exit immediately after logging.
 */
TEST_FUNCTION(test_mts_platform_send_msg_via_transport_no_retry_on_other_errors, nullptr, nullptr)
{
    trp_msg_t trp_msg;
    trp_icc_endpoint_t trp_icc_endpoint;
    uint8_t outgoing_icc_buffer[MAX_ICC_BUFFER_SIZE];

    trp_icc_endpoint.base_endpt.transport_type = TRP_TRANSPORT_TYPE_ICC_ARM_MHU;
    trp_icc_endpoint.base_endpt.seq_number.as_uint16 = TRP_SEQ_NUM_NOT_INIT;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.hdr.dest_node.die_id = 0;
    trp_msg.hdr.dest_node.core_id = MTS_PLATFORM_CORE_AP;
    trp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE + sizeof(dcp_msg_hdr_t);

    trp_msg.payload.dcp_msg.hdr.client_id = FAKE_CLIENT_ID;
    trp_msg.payload.dcp_msg.hdr.seq_num = TRP_SEQ_NUM_NOT_INIT;
    trp_msg.payload.dcp_msg.hdr.payload_size = FAKE_PAYLOAD_SIZE;
    trp_msg.hdr.incoming_endpt = nullptr;

    will_return(__wrap_transfer_rly_get_this_die_id, 0);
    will_return(__wrap_transfer_rly_should_send_dcp_msg, true);

    // Only one send attempt expected - non-timeout error should not retry
    will_return(__wrap_fpfw_icc_base_send_sync, outgoing_icc_buffer);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);
    // No tx_thread_sleep expected - no retry on non-timeout errors

    memset(outgoing_icc_buffer, 0, MAX_ICC_BUFFER_SIZE);
    mts_platform_send_msg_via_transport(&trp_msg, (p_trp_endpoint_t)&trp_icc_endpoint);
}