//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pwr_tlm_scp_service.cpp
 * Test manager for MTS for SCP power telemetry service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h>               // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <message_transfer_service.h> // for mts_client_flush_incoming_queue
#include <mts_client.h>               // for MTS_CLIENT_ID_PWR_INST_TELEM
#include <mts_manager_i.h>
#include <mts_platform_definitions.h> // for MTS_PLATFORM_CORE_OTHER, MTS...
#include <pwr_tlm_core_exchange.h>
#include <pwr_tlm_service_scp.h>
#include <pwr_tlm_service_scp_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

void test_pwr_tlm_scp_handle_incoming_mts_msgs(void);

/*-- Declarations (Statics and globals) --*/
TX_BLOCK_POOL client_block_pool;
TX_QUEUE client_rx_queue;

mts_client_t s_pwr_tlm_mts_client_scp_test = {
    .rx_pool = client_block_pool,
    .rx_queue = client_rx_queue,
    .notify_from_drv_frmwk = test_pwr_tlm_scp_handle_incoming_mts_msgs,
};
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

void test_pwr_tlm_scp_handle_incoming_mts_msgs(void)
{
    function_called();
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp, test_setup, test_teardown)
{
    // stub need to complete
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_handle_enables_from_mcp, test_setup, test_teardown)
{
    tlm_scp_record_enables_t test_enables = {{0}};

    // Test case 1: Enable drop_count_en only
    test_enables.as_uint16 = 0x0001;
    data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(test_enables);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0001);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 1);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 0);

    // Test case 2: Enable vm_memory_pwr_en only
    test_enables.as_uint16 = 0x0002;
    data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(test_enables);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0002);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 0);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 1);

    // Test case 3: Enable both flags
    test_enables.as_uint16 = 0x0003;
    data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(test_enables);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0003);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 1);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 1);

    // Test case 4: Disable all flags
    test_enables.as_uint16 = 0x0000;
    data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(test_enables);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0000);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 0);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 0);
}

TEST_FUNCTION(test_pwr_tlm_scp_handle_incoming_mts_msgs_dcp_invalid_case, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_POOL_ERROR);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    pwr_tlm_scp_handle_incoming_mts_msgs();
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_client_defined, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH
    // This tests that the message routing correctly calls data_proc_scp_tlm_cmpnt_handle_enables_from_mcp
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH;
    tlm_client_msg->payload.scp_records.as_uint16 = 0x0003; // Enable both flags

    // Reset enables to ensure the function call has an effect
    pwr_tlm_scp_record_enables.as_uint16 = 0x0000;

    mts_manager_scp_handle_trp_msg(&trp_msg);

    // Verify data_proc_scp_tlm_cmpnt_handle_enables_from_mcp was called and executed correctly
    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0003);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 1);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 1);

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with both flags enabled
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    // Since data_proc functions are not mocked, they will be called when flags are set
    // No assertions needed as we're just verifying the function executes without error
    mts_manager_scp_handle_trp_msg(&trp_msg);

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with flags disabled
    pwr_tlm_scp_record_enables.as_uint16 = 0x0000;
    mts_manager_scp_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_scp_init, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return_always(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap_FpFwAssertWithArgs, expression, true);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    will_return_always(__wrap__txe_block_pool_create, TX_SUCCESS);
    expect_function_calls(__wrap__txe_block_pool_create, 1);

    expect_value(__wrap_FpFwAssertWithArgs, expression, true);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    mts_client_register(MTS_CLIENT_ID_PWR_INST_TELEM, &s_pwr_tlm_mts_client_scp_test);
    mts_manager_scp_init();
}
