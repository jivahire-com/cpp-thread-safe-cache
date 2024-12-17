//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcp_svc_client.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <data_collection_protocol.h>
#include <data_collection_service.h>
#include <dcp_service_client_i.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <stdint.h>      // for uint8_t
#include <telemetry_relay_protocol.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_dcp_svc_client_handle_incoming_msgs, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_incoming_msgs_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    // QUEUE receive fail
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SIZE_ERROR);

    dcp_svc_client_handle_incoming_msgs();

    // null ptr returned
    p_trp_msg = nullptr;

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();

    // not DCP message
    p_trp_msg = &trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();

    // not DCP message and block release fails
    p_trp_msg = &trp_msg;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_POOL_ERROR);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_get_capabilities_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_get_state_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_get_schema_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_SCHEMA;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_events_en_dis_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_ENABLE_DISABLE;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_events_start_stop_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_events_read_data_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_events_read_data_compl_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_events_reset_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_RESET;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_msg_id_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_NOTIFICATION;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_get_plat_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_PLAT_INFO;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}

TEST_FUNCTION(test_dcp_svc_client_handle_invalid_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_MAX;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcp_svc_client_handle_incoming_msgs();
}