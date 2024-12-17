//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcs.cpp
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
#include <data_collection_service_i.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <stdint.h>      // for uint8_t
#include <telemetry_relay_protocol.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void notify_dcs_client_from_drv_frmwk(void);

/*-- Declarations (Statics and globals) --*/
TX_BLOCK_POOL client_block_pool;
TX_QUEUE client_rx_queue;

dcs_client_t client = {
    .rx_pool = client_block_pool,
    .rx_queue = client_rx_queue,
    .notify_from_drv_frmwk = notify_dcs_client_from_drv_frmwk,
};

/*------------- Functions ----------------*/

void notify_dcs_client_from_drv_frmwk(void)
{
    function_called();
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    dcs_register_client(DCP_CLIENT_ID_PWR_INST_TELEM, &client);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_dcs_init, test_setup, nullptr)
{
    dcs_config_t config = {{0}};
    config.trp_icc_config.number_of_endpoints = 0; // test tlm_relay_init standalone

    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);
    will_return_always(__wrap__txe_thread_create, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return_always(__wrap__txe_queue_create, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    will_return_always(__wrap__txe_block_pool_create, TX_SUCCESS);
    expect_function_calls(__wrap__txe_block_pool_create, 1);

    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 2);

    // for dcp_svc_client_init()
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    expect_function_calls(__wrap__txe_block_pool_create, 1);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_init(&config);
}

TEST_FUNCTION(test_dcs_register_client_bad_client, nullptr, nullptr)
{

    dcs_register_client(DCP_CLIENT_ID_MAX, &client);
}

TEST_FUNCTION(test_dcs_queue_msg_from_drv_frmwk_sunny, test_setup, nullptr)
{
    trp_msg_t src_trp_msg;
    src_trp_msg.hdr.trp_msg_status = 0xABCD;

    trp_msg_t dest_trp_msg = {{0}};

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    fpfw_status_t status = dcs_queue_msg_from_drv_frmwk(&client_block_pool, &client_rx_queue, &src_trp_msg);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    // verifies source was copied to the allocated destination block
    assert_int_equal(dest_trp_msg.hdr.trp_msg_status, 0xABCD);
}

TEST_FUNCTION(test_dcs_queue_msg_from_drv_frmwk_out_of_mem, test_setup, nullptr)
{
    trp_msg_t src_trp_msg;
    src_trp_msg.hdr.trp_msg_status = 0xABCD;

    trp_msg_t dest_trp_msg = {{0}};

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    fpfw_status_t status = dcs_queue_msg_from_drv_frmwk(&client_block_pool, &client_rx_queue, &src_trp_msg);

    assert_int_equal(status, FPFW_STATUS_OUT_OF_MEMORY);

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, NULL); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    status = dcs_queue_msg_from_drv_frmwk(&client_block_pool, &client_rx_queue, &src_trp_msg);

    assert_int_equal(status, FPFW_STATUS_OUT_OF_MEMORY);
}

TEST_FUNCTION(test_dcs_queue_msg_from_drv_frmwk_queue_fail, test_setup, nullptr)
{
    trp_msg_t src_trp_msg;
    src_trp_msg.hdr.trp_msg_status = 0xABCD;

    trp_msg_t dest_trp_msg = {{0}};

    // test queue failed and block was released

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    fpfw_status_t status = dcs_queue_msg_from_drv_frmwk(&client_block_pool, &client_rx_queue, &src_trp_msg);

    assert_int_equal(status, FPFW_STATUS_BUFFER_TOO_SMALL);

    // test queue failed and block release failed
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_POOL_ERROR);

    status = dcs_queue_msg_from_drv_frmwk(&client_block_pool, &client_rx_queue, &src_trp_msg);

    assert_int_equal(status, FPFW_STATUS_BUFFER_TOO_SMALL);
}

TEST_FUNCTION(test_dcs_queue_for_outbound_from_drv_frmwk, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    trp_msg.hdr.source_die_id = 0xAA;
    trp_msg.hdr.source_cpu_id = 0xA2;
    trp_msg.hdr.dest_die_id = 0xDD;
    trp_msg.hdr.dest_cpu_id = 0xD2;

    // bypass most of dcs_queue_msg_from_drv_frmwk() since it's already tested
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, nullptr); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_queue_for_outbound_from_drv_frmwk(&trp_msg, true);

    // check swap
    assert_int_equal(trp_msg.hdr.source_die_id, 0xDD);
    assert_int_equal(trp_msg.hdr.source_cpu_id, 0xD2);
    assert_int_equal(trp_msg.hdr.dest_die_id, 0xAA);
    assert_int_equal(trp_msg.hdr.dest_cpu_id, 0xA2);

    // test no swap
    trp_msg.hdr.source_die_id = 0xAA;
    trp_msg.hdr.source_cpu_id = 0xA2;
    trp_msg.hdr.dest_die_id = 0xDD;
    trp_msg.hdr.dest_cpu_id = 0xD2;

    // bypass most of dcs_queue_msg_from_drv_frmwk() since it's already tested
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, nullptr); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_queue_for_outbound_from_drv_frmwk(&trp_msg, false);

    // check no swap
    assert_int_equal(trp_msg.hdr.source_die_id, 0xAA);
    assert_int_equal(trp_msg.hdr.source_cpu_id, 0xA2);
    assert_int_equal(trp_msg.hdr.dest_die_id, 0xDD);
    assert_int_equal(trp_msg.hdr.dest_cpu_id, 0xD2);
}

TEST_FUNCTION(test_dcs_is_valid_trp_msg_from_drv_frmwk_valid, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};

    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    bool valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, true);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_SUCCESS);

    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, true);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_SUCCESS);

    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, true);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_SUCCESS);

    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, true);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_SUCCESS);

    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, true);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_dcs_is_valid_trp_msg_from_drv_frmwk_notvalid, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};

    // test size
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t) + 1;

    bool valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, false);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_E_SIZE);

    // test unknown client
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_MAX;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, false);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_E_UNK_CLIENT);

    // test unregistered client
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_EVENT_TRACE;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, false);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_E_UNK_CLIENT);

    // test unknown message
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_MAX;
    trp_msg.hdr.payload_size = sizeof(trp_msg_read_data_t);

    valid = dcs_is_valid_trp_msg_from_drv_frmwk(&trp_msg);

    assert_int_equal(valid, false);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_E_UNK_MSG);
}

TEST_FUNCTION(test_dcs_is_valid_dcp_msg_from_drv_frmwk_valid, test_setup, nullptr)
{
    dcp_msg_t dcp_msg = {{0}};

    // DCP_MSG_ID_GET_CAPABILITIES
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    dcp_msg.hdr.payload_size = 0;

    bool valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_GET_STATE
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_GET_SCHEMA
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_SCHEMA;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_READ_DATA
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_RESET
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_RESET;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_GET_PLAT_INFO
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_PLAT_INFO;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_EVENTS_ENABLE_DISABLE
    //  events are 4 bytes per event, and number of events is 2 bytes
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_ENABLE_DISABLE;
    dcp_msg.hdr.payload_size = 18;
    dcp_msg.payload.events_enable_disable.number_of_events = 4;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_START_STOP
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    dcp_msg.hdr.payload_size = sizeof(dcp_msg_start_stop_t);

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);

    // DCP_MSG_ID_READ_DATA_COMPLETE
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    dcp_msg.hdr.payload_size = sizeof(dcp_msg_read_data_complete_t);

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, true);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_dcs_is_valid_dcp_msg_from_drv_frmwk_invalid, test_setup, nullptr)
{
    dcp_msg_t dcp_msg = {{0}};

    // DCP_CLIENT_ID_MAX
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_MAX;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    dcp_msg.hdr.payload_size = 0;

    bool valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_UNK_CLIENT);

    // unregistered client
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_EVENT_TRACE;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_UNK_CLIENT);

    // DCP_MSG_ID_GET_CAPABILITIES bad size
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    dcp_msg.hdr.payload_size = 2;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_SIZE);

    // DCP_MSG_ID_EVENTS_ENABLE_DISABLE bad size
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_ENABLE_DISABLE;
    dcp_msg.hdr.payload_size = 2;
    dcp_msg.payload.events_enable_disable.number_of_events = 4;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_SIZE);

    // DCP_MSG_ID_START_STOP bad size
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    dcp_msg.hdr.payload_size = sizeof(dcp_msg_start_stop_t) + 2;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_SIZE);

    // DCP_MSG_ID_READ_DATA_COMPLETE bad size
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    dcp_msg.hdr.payload_size = sizeof(dcp_msg_read_data_complete_t) + 3;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_SIZE);
}

TEST_FUNCTION(test_dcs_is_valid_dcp_msg_from_drv_frmwk_deprecated, test_setup, nullptr)
{
    dcp_msg_t dcp_msg = {{0}};

    // DCP_MSG_ID_EVENTS_DISABLE
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_DISABLE;
    dcp_msg.hdr.payload_size = 0;

    bool valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_DEPRECATED_MSG);

    // DCP_MSG_ID_STOP
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_STOP;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_DEPRECATED_MSG);

    // DCP_MSG_ID_UTC_SYNC
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_UTC_SYNC;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_DEPRECATED_MSG);

    // unknown message
    dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    dcp_msg.hdr.msg_id = DCP_MSG_ID_MAX;
    dcp_msg.hdr.payload_size = 0;

    valid = dcs_is_valid_dcp_msg_from_drv_frmwk(&dcp_msg);
    assert_int_equal(valid, false);
    assert_int_equal(dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_UNK_MSG);
}

TEST_FUNCTION(test_dcs_service_client_notification_from_drv_frmwk, test_setup, nullptr)
{
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_DCS_CLIENT_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_service_client_notification_from_drv_frmwk();
}

TEST_FUNCTION(test_dcs_forward_trp_msg_to_client_from_drv_frmwk_dcp_valid, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    trp_msg_t dest_trp_msg = {{0}};

    // setup valid dcp message forward
    trp_msg.hdr.source_die_id = 0xAA;
    trp_msg.hdr.source_cpu_id = 0xA2;
    trp_msg.hdr.dest_die_id = 0xDD;
    trp_msg.hdr.dest_cpu_id = 0xD2;
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    trp_msg.payload.dcp_msg.hdr.payload_size = 0;

    // set up dcs_queue_msg_from_drv_frmwk() to return success
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_function_calls(notify_dcs_client_from_drv_frmwk, 1);

    dcs_forward_trp_msg_to_client_from_drv_frmwk(&trp_msg);

    // doesn't swap on success
    assert_int_equal(trp_msg.hdr.source_die_id, 0xAA);
    assert_int_equal(trp_msg.hdr.source_cpu_id, 0xA2);
    assert_int_equal(trp_msg.hdr.dest_die_id, 0xDD);
    assert_int_equal(trp_msg.hdr.dest_cpu_id, 0xD2);
}

TEST_FUNCTION(test_dcs_forward_trp_msg_to_client_from_drv_frmwk_trp_invalid, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};

    // setup valid dcp message forward
    trp_msg.hdr.source_die_id = 0xAA;
    trp_msg.hdr.source_cpu_id = 0xA2;
    trp_msg.hdr.dest_die_id = 0xDD;
    trp_msg.hdr.dest_cpu_id = 0xD2;
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_MAX;

    // bypass most of dcs_queue_msg_from_drv_frmwk() since it's already tested
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, nullptr); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_forward_trp_msg_to_client_from_drv_frmwk(&trp_msg);

    // does swap on failure
    assert_int_equal(trp_msg.hdr.source_die_id, 0xDD);
    assert_int_equal(trp_msg.hdr.source_cpu_id, 0xD2);
    assert_int_equal(trp_msg.hdr.dest_die_id, 0xAA);
    assert_int_equal(trp_msg.hdr.dest_cpu_id, 0xA2);
}

TEST_FUNCTION(test_dcs_forward_trp_msg_to_client_from_drv_frmwk_dcp_incomplete, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    trp_msg_t dest_trp_msg = {{0}};

    // setup valid dcp message forward
    trp_msg.hdr.source_die_id = 0xAA;
    trp_msg.hdr.source_cpu_id = 0xA2;
    trp_msg.hdr.dest_die_id = 0xDD;
    trp_msg.hdr.dest_cpu_id = 0xD2;
    trp_msg.hdr.dcp_client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.client_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    trp_msg.payload.dcp_msg.hdr.payload_size = 0;

    // set up dcs_queue_msg_from_drv_frmwk() to return fail
    // this causes the forward to the client to fail, so below it will try to queue
    // in the DCs queue with the error message
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &dest_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    // bypass most of dcs_queue_msg_from_drv_frmwk() since it's already tested
    // this one is called from dcs_queue_for_outbound_from_drv_frmwk
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, nullptr); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_forward_trp_msg_to_client_from_drv_frmwk(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, (uint16_t)DCP_STATUS_E_INCOMPLETE_HANDLER);
    assert_int_equal(trp_msg.hdr.trp_msg_status, (uint16_t)TRP_STATUS_E_INCOMPLETE_HANDLER);

    // does swap on failure
    assert_int_equal(trp_msg.hdr.source_die_id, 0xDD);
    assert_int_equal(trp_msg.hdr.source_cpu_id, 0xD2);
    assert_int_equal(trp_msg.hdr.dest_die_id, 0xAA);
    assert_int_equal(trp_msg.hdr.dest_cpu_id, 0xA2);
}

TEST_FUNCTION(test_dcs_handle_outbound_msgs, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    // will cause tlm_relay_send_outgoing_msg to fail since no route set up in this test
    trp_msg.hdr.dest_die_id = 0xAA;
    trp_msg.hdr.dest_cpu_id = 0xbb;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    p_trp_msg_t p_trp_msg = &trp_msg;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcs_handle_outbound_msgs();
}

TEST_FUNCTION(test_dcs_handle_outbound_msgs_block_rel_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    // will cause tlm_relay_send_outgoing_msg to fail since no route set up in this test
    trp_msg.hdr.dest_die_id = 0xAA;
    trp_msg.hdr.dest_cpu_id = 0xbb;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    p_trp_msg_t p_trp_msg = &trp_msg;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_POOL_ERROR);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcs_handle_outbound_msgs();
}

TEST_FUNCTION(test_dcs_handle_outbound_msgs_QUEUE_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    // will cause tlm_relay_send_outgoing_msg to fail since no route set up in this test
    trp_msg.hdr.dest_die_id = 0xAA;
    trp_msg.hdr.dest_cpu_id = 0xbb;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    p_trp_msg_t p_trp_msg = &trp_msg;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_ERROR);

    dcs_handle_outbound_msgs();
}

TEST_FUNCTION(test_dcs_handle_outbound_msgs_queue_null_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    // will cause tlm_relay_send_outgoing_msg to fail since no route set up in this test
    trp_msg.hdr.dest_die_id = 0xAA;
    trp_msg.hdr.dest_cpu_id = 0xbb;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    p_trp_msg_t p_trp_msg = &trp_msg;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, nullptr);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    dcs_handle_outbound_msgs();
}