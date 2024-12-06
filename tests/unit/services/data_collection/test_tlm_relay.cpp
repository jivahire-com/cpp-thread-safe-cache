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

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
dcs_config_t config = {{0}};
dcp_msg_t dcp_msg;
trp_icc_endpoint_t icc_endpoint;

/*------------- Functions ----------------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    dcp_msg.hdr.client_id = 1;
    dcp_msg.hdr.seq_num = 4;

    icc_endpoint.async_recv_req.recv_cmd_code = ICC_COMMAND_DCP_MSG;
    icc_endpoint.async_recv_req.payload_buffer = (uint8_t*)&dcp_msg;

    config.trp_icc_config.number_of_endpoints = 0;
    config.trp_icc_config.endpoint_table = &icc_endpoint;
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

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_1, test_setup, nullptr)
{
    // test dcs_forward_trp_msg_to_client_from_drv_frmwk()
    icc_endpoint.async_recv_req.recv_cmd_code = ICC_COMMAND_DCP_MSG;
    config.trp_icc_config.number_of_endpoints = 1;

    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint, 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_2, test_setup, nullptr)
{
    // test dcs_forward_trp_msg_to_client_from_drv_frmwk()
    icc_endpoint.async_recv_req.recv_cmd_code = ICC_COMMAND_TRP_MSG;
    config.trp_icc_config.number_of_endpoints = 1;

    expect_any_always(__wrap_fpfw_icc_base_recv, icc_ctx);
    expect_any_always(__wrap_fpfw_icc_base_recv, params);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_FAIL);

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint, 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_tlm_relay_icc_recv_complete_notify_from_drv_frmwk_fails, test_setup, nullptr)
{
    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint, 0, FPFW_STATUS_FAIL);

    icc_endpoint.async_recv_req.recv_cmd_code = 0x48;
    config.trp_icc_config.number_of_endpoints = 1;

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint, 0, FPFW_STATUS_SUCCESS);

    icc_endpoint.async_recv_req.recv_cmd_code = ICC_COMMAND_TRP_MSG;
    config.trp_icc_config.endpoint_table = nullptr;
    config.trp_icc_config.number_of_endpoints = 1;

    tlm_relay_icc_recv_complete_notify_from_drv_frmwk(&icc_endpoint, 0, FPFW_STATUS_SUCCESS);
}