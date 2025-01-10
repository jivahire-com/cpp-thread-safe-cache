//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcs_manager.cpp
 * Test manager for data collection service
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <dcs_manager_i.h>
#include <ddr_manager_i.h>
#include <in_band_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------- Function Prototypes -----------*/

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

TEST_FUNCTION(test_dcs_manager_queue_tlm_package, test_setup, test_teardown)
{
    tlm_package_t tlm_pkg = {0xaabb, 0xaabb};

    // no receive data, successfully queues in pending
    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    dcs_manager_queue_tlm_package(0x1234, 0x5678);

    // receive data, de-allocates memory, fails to queue in pending

    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);

    dcs_manager_queue_tlm_package(0x1234, 0x5678);
}

TEST_FUNCTION(test_dcs_manager_handle_record_enable_disable, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }
    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 2;

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_SOC_PC3;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].event_id = INST_TELEMETRY_ELEMENT_CORE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    dcs_manager_handle_record_enable_disable(&trp_msg);

    assert_true(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3]);
    assert_true(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_DISABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].state = DCP_EVENTS_ENABLE_STATE_DISABLE;
    dcs_manager_handle_record_enable_disable(&trp_msg);

    assert_false(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3]);
    assert_false(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_dcs_manager_handle_record_enable_disable_fail, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = DCP_MAX_ENABLE_DISABLE_EVENTS + 1;
    dcs_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 1;

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_ID_MAX;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    dcs_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = INST_TELEMETRY_ELEMENT_ID_MAX;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;
    dcs_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = 0xFFFF;
    dcs_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);
}

TEST_FUNCTION(test_dcs_manager_handle_handle_trp, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    dcs_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_get_state, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, true);
    will_return(exec_tlm_cmpnt_is_telemetry_enabled, true);
    expect_function_call(__wrap_dcs_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_RUNNING);

    will_return(__wrap_dcs_is_primary_instance, true);
    will_return(exec_tlm_cmpnt_is_telemetry_enabled, false);
    expect_function_call(__wrap_dcs_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_STOPPED);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_en_dis, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, true);
    expect_function_call(__wrap_dcs_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_ENABLE_DISABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = DCP_MAX_ENABLE_DISABLE_EVENTS + 1;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    will_return(__wrap_dcs_is_primary_instance, true);
    expect_function_call(__wrap_dcs_client_send_trp_msg);
    expect_function_call(__wrap_dcs_client_send_trp_response);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = INST_TELEMETRY_ELEMENT_CORE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 1;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_start_stop, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, false);
    expect_function_call(exec_tlm_cmpnt_enable_disable_telemetry);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    trp_msg.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_START;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    will_return(__wrap_dcs_is_primary_instance, false);
    expect_function_call(exec_tlm_cmpnt_enable_disable_telemetry);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    trp_msg.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_STOP;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_read_data, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, false);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_INCOMPLETE_HANDLER);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_read_data_complete, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, false);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_INCOMPLETE_HANDLER);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_reset, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, false);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_RESET;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_INCOMPLETE_HANDLER);
}

TEST_FUNCTION(test_dcs_manager_handle_dcp_msg_unsupported, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{0}};

    will_return(__wrap_dcs_is_primary_instance, false);

    trp_msg.payload.dcp_msg.hdr.msg_id = 0xFFFF;

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_E_UNSUPPORTED_MSG);
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_dcs_msg_dcp, test_setup, nullptr)
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

    will_return(__wrap_dcs_is_primary_instance, false);
    will_return(exec_tlm_cmpnt_is_telemetry_enabled, false);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    in_band_tlm_cmpnt_handle_incoming_dcs_msgs();
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_dcs_msg_trp, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

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

    in_band_tlm_cmpnt_handle_incoming_dcs_msgs();
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_dcs_msg_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SIZE_ERROR);

    in_band_tlm_cmpnt_handle_incoming_dcs_msgs();
}
