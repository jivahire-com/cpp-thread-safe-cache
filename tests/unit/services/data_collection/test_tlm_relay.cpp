//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tlm_relay.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...
#include <string>
extern "C" {
#ifndef BEGIN_EXTERN_C
    #define BEGIN_EXTERN_C
#endif
#ifndef END_EXTERN_C
    #define END_EXTERN_C
#endif

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <data_collection_protocol.h>
#include <data_collection_service.h>
#include <data_collection_service_i.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <kng_icc_shared.h>
#include <stdint.h> // for uint8_t
#include <telemetry_relay_protocol.h>
#include <tlm_relay_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/
#define NUM_ENDPOINTS (2)
#define NUM_ROUTES    (2)

#define ICC_CONTEXT_0 (0x1234)
#define ICC_CONTEXT_1 (0x5678)

#define DCP_ENDPOINT (0)
#define TRP_ENDPOINT (1)

#define THIS_DIE_ID (1)
#define THIS_CPU_ID (2)

#define DCP_OUTGOING_DIE_ID (2)
#define DCP_OUTGOING_CPU_ID (4)

#define TRP_OUTGOING_DIE_ID (3)
#define TRP_OUTGOING_CPU_ID (9)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
dcs_config_t config = {{0}};
dcp_msg_t dcp_msg;
trp_msg_t trp_msg;
icc_msg_t incoming_icc_msg;
trp_icc_endpoint_t icc_endpoint[NUM_ENDPOINTS];
trp_route_t trp_routing_table[NUM_ROUTES];

/*------------- Functions ----------------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    dcp_msg.hdr.client_id = 1;
    dcp_msg.hdr.seq_num = 4;

    sprintf(icc_endpoint[DCP_ENDPOINT].name, "DCP EP!");
    icc_endpoint[DCP_ENDPOINT].icc_payload_protocol = ICC_COMMAND_DCP_MSG;
    icc_endpoint[DCP_ENDPOINT].async_recv_buffer_size = sizeof(incoming_icc_msg);
    icc_endpoint[DCP_ENDPOINT].async_recv_buffer = (uint8_t*)&incoming_icc_msg;

    sprintf(icc_endpoint[TRP_ENDPOINT].name, "TRP EP!");
    icc_endpoint[TRP_ENDPOINT].icc_payload_protocol = ICC_COMMAND_TRP_MSG;
    icc_endpoint[TRP_ENDPOINT].async_recv_buffer_size = sizeof(incoming_icc_msg);
    icc_endpoint[TRP_ENDPOINT].async_recv_buffer = (uint8_t*)&incoming_icc_msg;

    trp_routing_table[0].dest.die_id = DCP_OUTGOING_DIE_ID;
    trp_routing_table[0].dest.cpu_id = DCP_OUTGOING_CPU_ID;
    trp_routing_table[0].icc_endpoint = &icc_endpoint[DCP_ENDPOINT];

    trp_routing_table[1].dest.die_id = TRP_OUTGOING_DIE_ID;
    trp_routing_table[1].dest.cpu_id = TRP_OUTGOING_CPU_ID;
    trp_routing_table[1].icc_endpoint = &icc_endpoint[TRP_ENDPOINT];

    config.trp_icc_config.routing_table = trp_routing_table;
    config.trp_icc_config.number_of_routes = NUM_ROUTES;
    config.trp_icc_config.number_of_endpoints = NUM_ENDPOINTS;
    config.trp_icc_config.endpoint_table = icc_endpoint;
    config.trp_icc_config.this_die_id = THIS_DIE_ID;
    config.trp_icc_config.this_cpu_id = THIS_CPU_ID;

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    tlm_relay_init(&config.trp_icc_config);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_tlm_relay_init, nullptr, nullptr)
{
    static trp_icc_endpoint_t s_trp_icc_endpoint_table[NUM_ENDPOINTS];
    static trp_route_t s_trp_routing_table[NUM_ROUTES];

    static dcs_config_t s_config = {
        .thread_config =
            {
                .p_stack = nullptr,
                .stack_size = 0,
                .priority = 1,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .trp_icc_config =
            {
                .routing_table = s_trp_routing_table,
                .number_of_routes = 1,
                .endpoint_table = s_trp_icc_endpoint_table,
                .number_of_endpoints = 1, // NUM_ENDPOINTS,
                .this_die_id = 2,
                .this_cpu_id = 3,
            },
    };

    s_trp_icc_endpoint_table[0].icc_base_ctx = (fpfw_icc_base_ctx_t*)ICC_CONTEXT_0;
    s_trp_icc_endpoint_table[0].icc_payload_protocol = ICC_COMMAND_DCP_MSG;
    s_trp_routing_table[0].icc_endpoint = &s_trp_icc_endpoint_table[0];

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, ICC_CONTEXT_0);
    expect_value(__wrap_fpfw_icc_base_recv, params, &s_trp_routing_table[0].icc_endpoint->async_recv_req);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    tlm_relay_init(&s_config.trp_icc_config);

    s_trp_icc_endpoint_table[0].icc_base_ctx = (fpfw_icc_base_ctx_t*)ICC_CONTEXT_0;
    s_trp_icc_endpoint_table[0].icc_payload_protocol = ICC_COMMAND_TRP_MSG;
    s_trp_routing_table[0].icc_endpoint = &s_trp_icc_endpoint_table[0];

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, ICC_CONTEXT_0);
    expect_value(__wrap_fpfw_icc_base_recv, params, &s_trp_routing_table[0].icc_endpoint->async_recv_req);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    tlm_relay_init(&s_config.trp_icc_config);
}

TEST_FUNCTION(test_tlm_relay_send_outgoing_msg_valid_dcp, test_setup, nullptr)
{
    trp_msg.hdr.dest_die_id = DCP_OUTGOING_DIE_ID;
    trp_msg.hdr.dest_cpu_id = DCP_OUTGOING_CPU_ID;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);

    tlm_relay_send_outgoing_msg(&trp_msg);

    trp_msg.hdr.dest_die_id = TRP_OUTGOING_DIE_ID;
    trp_msg.hdr.dest_cpu_id = TRP_OUTGOING_CPU_ID;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE;

    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_SUCCESS);

    tlm_relay_send_outgoing_msg(&trp_msg);
}

TEST_FUNCTION(test_tlm_relay_send_outgoing_msg_invalid_dcp, test_setup, nullptr)
{
    // dest die id doesn't match
    trp_msg.hdr.dest_die_id = 0xFF;
    trp_msg.hdr.dest_cpu_id = DCP_OUTGOING_CPU_ID;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    tlm_relay_send_outgoing_msg(&trp_msg);

    // dest cpu id doesn't match
    trp_msg.hdr.dest_die_id = DCP_OUTGOING_DIE_ID;
    trp_msg.hdr.dest_cpu_id = 0xFF;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    tlm_relay_send_outgoing_msg(&trp_msg);

    // icc send failure
    trp_msg.hdr.dest_die_id = DCP_OUTGOING_DIE_ID;
    trp_msg.hdr.dest_cpu_id = DCP_OUTGOING_CPU_ID;
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_STATUS_FAIL);

    tlm_relay_send_outgoing_msg(&trp_msg);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_dcp_msg, test_setup, nullptr)
{
    trp_msg_t allocated_trp_msg;
    // send invalid payload size dcp message
    // it fails validation and then gets a block allocated, queues the response and sends an event to the dcs thread
    incoming_icc_msg.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;
    incoming_icc_msg.dcp_msg.hdr.client_id = DCP_CLIENT_ID_DCS;
    incoming_icc_msg.dcp_msg.hdr.payload_size = 0xFF;

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &allocated_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint[DCP_ENDPOINT], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_trp_msg, test_setup, nullptr)
{
    trp_msg_t allocated_trp_msg;
    // send invalid payload size trp message
    // it fails validation and then gets a block allocated, queues the response and sends an event to the dcs thread
    incoming_icc_msg.trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;
    incoming_icc_msg.trp_msg.hdr.trp_msg_id = DCP_CLIENT_ID_DCS;
    incoming_icc_msg.trp_msg.hdr.payload_size = 0xFF;
    incoming_icc_msg.trp_msg.hdr.dest_die_id = THIS_DIE_ID;

    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, &allocated_trp_msg); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint[TRP_ENDPOINT], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_fail, test_setup, nullptr)
{

    // icc failed, data is invalid
    // it will loop through the endpoint contexts and re-register the icc command code
    // in this path, re-registering the icc command code will fail
    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);
    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint[TRP_ENDPOINT], 0, FPFW_STATUS_FAIL);

    // invalid command code, it will not re-register an invalid icc command code
    icc_endpoint[TRP_ENDPOINT].async_recv_req.recv_cmd_code = 0x48;
    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint[TRP_ENDPOINT], 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_trp_foward, test_setup, nullptr)
{

    // send invalid payload size trp message
    // it fails validation and then gets a block allocated, queues the response and sends an event to the dcs thread
    incoming_icc_msg.trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;
    incoming_icc_msg.trp_msg.hdr.trp_msg_id = DCP_CLIENT_ID_PWR_INST_TELEM;
    incoming_icc_msg.trp_msg.hdr.payload_size = 0xFF;
    incoming_icc_msg.trp_msg.hdr.dest_die_id = THIS_DIE_ID;
    incoming_icc_msg.trp_msg.hdr.dest_cpu_id = THIS_CPU_ID;

    // bypass most of dcs_queue_msg_from_drv_frmwk() since it's already tested
    expect_function_calls(__wrap__txe_block_allocate, 1);
    will_return(__wrap__txe_block_allocate, nullptr); // mocks the block allocate location
    will_return(__wrap__txe_block_allocate, TX_POOL_ERROR);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, NEW_OUTBOUND_MSG);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint[TRP_ENDPOINT], 0, FPFW_STATUS_SUCCESS);
}
