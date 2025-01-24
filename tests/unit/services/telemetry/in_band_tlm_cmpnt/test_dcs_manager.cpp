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
    uint8_t buffer[1000] = {0};

    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&dcs_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &dcs_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    // test notification of adding to empty list and notifying host
    will_return(__wrap_dcs_is_primary_instance, true);
    expect_function_call(__wrap_dcs_client_send_dcp_notification);
    dcs_manager_queue_tlm_package((uintptr_t)buffer, 1000);

    // active list isn't empty so no dcp notification
    will_return(__wrap_dcs_is_primary_instance, true);
    dcs_manager_queue_tlm_package((uintptr_t)buffer, 1000);

    // secondary instance will queue trp message to primary
    will_return(__wrap_dcs_is_primary_instance, false);
    expect_function_call(__wrap_dcs_client_send_new_trp_msg);
    dcs_manager_queue_tlm_package((uintptr_t)buffer, 1000);
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
    expect_function_call(__wrap_dcs_client_forward_trp_msg);
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

TEST_FUNCTION(test_dcs_manager_send_trp_package_helper, test_setup, nullptr)
{
    tlm_package_t tlm_pkg = {{0}};

    expect_function_call(__wrap_dcs_client_send_new_trp_msg);

    dcs_manager_send_trp_package_helper(&tlm_pkg, TRP_MSG_ID_READ_INTERCORE_BLOCK, 2, 3);
}

TEST_FUNCTION(test_dcs_manager_send_trp_pkg_notification_to_primary, test_setup, nullptr)
{
    tlm_package_t tlm_pkg{{0}};

    expect_function_call(__wrap_dcs_client_send_new_trp_msg);

    dcs_manager_send_trp_pkg_notification_to_primary(&tlm_pkg);
}

TEST_FUNCTION(test_dcs_manager_send_trp_read_complete, test_setup, nullptr)
{
    tlm_package_t tlm_pkg{{0}};

    expect_function_call(__wrap_dcs_client_send_new_trp_msg);

    dcs_manager_send_trp_read_complete(&tlm_pkg);
}

TEST_FUNCTION(test_dcs_manager_get_pkg_from_free_list, test_setup, nullptr)
{
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    // secondary instance should return null
    will_return(__wrap_dcs_is_primary_instance, false);
    p_tlm_package_t tlm_pkg = dcs_manager_get_pkg_from_free_list();
    assert_null(tlm_pkg);

    // primary instance with an empty active list should return null
    will_return(__wrap_dcs_is_primary_instance, true);
    tlm_pkg = dcs_manager_get_pkg_from_free_list();
    assert_null(tlm_pkg);

    // an entry in the active list will be removed and returned
    FpFwListEntryInitialize(&dcs_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[1].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[1].list_entry);

    will_return(__wrap_dcs_is_primary_instance, true);
    tlm_pkg = dcs_manager_get_pkg_from_free_list();
    assert_int_equal(tlm_pkg, &dcs_active_pkg_buffer[0]);

    will_return(__wrap_dcs_is_primary_instance, true);
    tlm_pkg = dcs_manager_get_pkg_from_free_list();
    assert_int_equal(tlm_pkg, &dcs_active_pkg_buffer[1]);
}

TEST_FUNCTION(test_dcs_manager_free_tlm_package_from_secondary_mcp, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};

    tlm_package_t tlm_pkg = {{0}};

    tlm_pkg.pkg.source_die_id = 1;
    tlm_pkg.pkg.source_cpu_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.atu_mapped_location = (uintptr_t)buffer;
    tlm_pkg.pkg.ddr_addr_offset = 0;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    dcs_active_pkg_buffer[2].pkg.source_die_id = 1;
    dcs_active_pkg_buffer[2].pkg.source_cpu_id = 2;
    dcs_active_pkg_buffer[2].pkg.reserved = 0;
    dcs_active_pkg_buffer[2].pkg.atu_mapped_location = (uintptr_t)buffer;
    dcs_active_pkg_buffer[2].pkg.ddr_addr_offset = 0;
    dcs_active_pkg_buffer[2].pkg.pkg_size = 1000;
    dcs_active_pkg_buffer[2].pkg.crc = 0x4267;

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[2].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[2].list_entry);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[1].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[1].list_entry);

    dcs_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);

    PFPFW_LIST_ENTRY entry = FpFwListRemoveHead(&pkg_free_list);
    assert_int_equal(entry, &dcs_active_pkg_buffer[2].list_entry);

    dcs_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_free_list)); // no match so was not added to free list

    // verify empty list causes no harm
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    dcs_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_free_list));
    assert_true(FpFwListIsEmpty(&pkg_active_list));
}

TEST_FUNCTION(test_dcs_manager_handle_trp_msg_pkg_compl, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    uint8_t buffer[1000] = {0};

    trp_msg.payload.package_notification.source_die_id = 0;
    trp_msg.payload.package_notification.source_cpu_id = 2;
    trp_msg.payload.package_notification.reserved = 0;
    trp_msg.payload.package_notification.atu_mapped_location = (uintptr_t)buffer;
    trp_msg.payload.package_notification.ddr_addr_offset = 0x4000;
    trp_msg.payload.package_notification.pkg_size = 1000;
    trp_msg.payload.package_notification.crc = 0x4267;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;

    dcs_active_pkg_buffer[2].pkg.source_die_id = 0;
    dcs_active_pkg_buffer[2].pkg.source_cpu_id = 2;
    dcs_active_pkg_buffer[2].pkg.reserved = 0;
    dcs_active_pkg_buffer[2].pkg.atu_mapped_location = (uintptr_t)buffer;
    dcs_active_pkg_buffer[2].pkg.ddr_addr_offset = 0x4000;
    dcs_active_pkg_buffer[2].pkg.pkg_size = 1000;
    dcs_active_pkg_buffer[2].pkg.crc = 0x4267;

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&dcs_active_pkg_buffer[2].list_entry);
    FpFwListInsertTail(&pkg_active_list, &dcs_active_pkg_buffer[2].list_entry);

    dcs_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_dcs_manager_free_tlm_package_from_primary_mcp, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    FpFwListInitialize(&pkg_free_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_cpu_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.atu_mapped_location = (uintptr_t)buffer;
    tlm_pkg.pkg.ddr_addr_offset = 0;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    dcs_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);

    tlm_pkg.pkg.source_die_id = 3;
    expect_function_call(__wrap_dcs_client_send_new_trp_msg);
    dcs_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_cpu_id = 1;
    expect_function_call(__wrap_dcs_client_send_new_trp_msg);
    dcs_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);
}

TEST_FUNCTION(test_dcs_manager_handle_read_complete_msg, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    trp_msg_t trp_msg = {{0}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_cpu_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.atu_mapped_location = (uintptr_t)buffer;
    tlm_pkg.pkg.ddr_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    in_flight_tlm_pkg = &tlm_pkg;

    trp_msg.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = 0x4000;
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_NONE);
    assert_null(in_flight_tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_free_list));

    tlm_package_t tlm_pkg_2 = {{0}};
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg_2.list_entry);
    in_flight_tlm_pkg = &tlm_pkg;
    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_null(in_flight_tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_free_list));
}

TEST_FUNCTION(test_dcs_manager_handle_read_complete_msg_fail, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    trp_msg_t trp_msg = {{0}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_cpu_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.atu_mapped_location = (uintptr_t)buffer;
    tlm_pkg.pkg.ddr_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    in_flight_tlm_pkg = nullptr;

    trp_msg.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = 0x4001;
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_BUFFER_DISCARD);

    in_flight_tlm_pkg = &tlm_pkg;
    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);
}

TEST_FUNCTION(test_dcs_manager_handle_read_msg, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    tlm_package_t tlm_pkg_2 = {{0}};
    trp_msg_t trp_msg = {{0}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_cpu_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.atu_mapped_location = (uintptr_t)buffer;
    tlm_pkg.pkg.ddr_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;
    in_flight_tlm_pkg = nullptr;

    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_start_addr, IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_buffer_size, IB_TELEMETRY_DDR_TOTAL_SIZE);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_addr_offset, in_flight_tlm_pkg->pkg.ddr_addr_offset);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_size, in_flight_tlm_pkg->pkg.pkg_size);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.crc, in_flight_tlm_pkg->pkg.crc);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_LAST);
    assert_true(in_flight_tlm_pkg == &tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_active_list));

    in_flight_tlm_pkg = nullptr;
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg_2.list_entry);

    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_start_addr, IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_buffer_size, IB_TELEMETRY_DDR_TOTAL_SIZE);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_addr_offset, in_flight_tlm_pkg->pkg.ddr_addr_offset);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_size, in_flight_tlm_pkg->pkg.pkg_size);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.crc, in_flight_tlm_pkg->pkg.crc);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_true(in_flight_tlm_pkg == &tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_active_list));
}

TEST_FUNCTION(test_dcs_manager_handle_read_msg_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    tlm_package_t tlm_pkg = {{0}};
    tlm_package_t tlm_pkg_2 = {{0}};

    // verify no data
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    dcs_manager_handle_read_msg(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_NONE);

    in_flight_tlm_pkg = &tlm_pkg_2;
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);

    dcs_manager_handle_read_msg(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_BUSY);
}

TEST_FUNCTION(test_dcs_manager_handle_trp_msg_pkg_notif, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    tlm_package_t tlm_pkg = {{0}};
    uint8_t buffer[1000] = {0};

    // verify no data
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    FpFwListInsertTail(&pkg_free_list, &tlm_pkg.list_entry);

    trp_msg.payload.package_notification.source_die_id = 0;
    trp_msg.payload.package_notification.source_cpu_id = 2;
    trp_msg.payload.package_notification.reserved = 0;
    trp_msg.payload.package_notification.atu_mapped_location = (uintptr_t)buffer;
    trp_msg.payload.package_notification.ddr_addr_offset = 0x4000;
    trp_msg.payload.package_notification.pkg_size = 1000;
    trp_msg.payload.package_notification.crc = 0x4267;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION;

    will_return(__wrap_dcs_is_primary_instance, false);
    expect_function_call(__wrap_dcs_client_send_new_trp_msg);

    dcs_manager_handle_trp_msg(&trp_msg);

    PFPFW_LIST_ENTRY entry = NULL;
    entry = FpFwListRemoveHead(&pkg_active_list);
    p_tlm_package_t queued_pkg = CONTAINING_RECORD(entry, tlm_package_t, list_entry);

    assert_int_equal(queued_pkg->pkg.source_die_id, 0);
    assert_int_equal(queued_pkg->pkg.source_cpu_id, 2);
    assert_int_equal(queued_pkg->pkg.reserved, 0);
    assert_int_equal(queued_pkg->pkg.atu_mapped_location, (uintptr_t)buffer);
    assert_int_equal(queued_pkg->pkg.ddr_addr_offset, 0x4000);
    assert_int_equal(queued_pkg->pkg.pkg_size, 1000);
    assert_int_equal(queued_pkg->pkg.crc, 0x4267);

    // free list is now empty, test that read complete is sent back to sender
    will_return(__wrap_dcs_is_primary_instance, false);
    expect_function_call(__wrap_dcs_client_send_new_trp_msg);
    dcs_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_dcs_manager_handle_reset_msg, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{0}};
    tlm_package_t tlm_pkg[4] = {{{0}}};
    uint8_t buffer[1000] = {0};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    // setup two local packages and 2 remote
    for (int i = 0; i < 4; i++)
    {
        tlm_pkg[i].pkg.source_die_id = 0;
        tlm_pkg[i].pkg.source_cpu_id = 2;
        tlm_pkg[i].pkg.reserved = 0;
        tlm_pkg[i].pkg.atu_mapped_location = (uintptr_t)buffer;
        tlm_pkg[i].pkg.ddr_addr_offset = 0x4000 + (i * 1000);
        tlm_pkg[i].pkg.pkg_size = 1000;
        tlm_pkg[i].pkg.crc = 0x4267 + i;
        FpFwListEntryInitialize(&tlm_pkg[i].list_entry);
        FpFwListInsertTail(&pkg_active_list, &tlm_pkg[i].list_entry);
    }
    tlm_pkg[1].pkg.source_die_id = 1;
    tlm_pkg[3].pkg.source_cpu_id = 1;

    tlm_package_t in_flight;
    in_flight.pkg.atu_mapped_location = (uintptr_t)buffer;
    in_flight_tlm_pkg = &in_flight;

    expect_function_call(__wrap_dcs_client_flush_incoming_queue);
    will_return(__wrap_dcs_is_primary_instance, true);

    dcs_manager_handle_reset_msg(&trp_msg);

    assert_true(FpFwListIsEmpty(&pkg_active_list));
    assert_false(FpFwListIsEmpty(&pkg_free_list));

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    // setup two local packages and 2 remote
    for (int i = 0; i < 4; i++)
    {
        tlm_pkg[i].pkg.source_die_id = 1;
        tlm_pkg[i].pkg.source_cpu_id = 2;
        tlm_pkg[i].pkg.reserved = 0;
        tlm_pkg[i].pkg.atu_mapped_location = (uintptr_t)buffer;
        tlm_pkg[i].pkg.ddr_addr_offset = 0x4000 + (i * 1000);
        tlm_pkg[i].pkg.pkg_size = 1000;
        tlm_pkg[i].pkg.crc = 0x4267 + i;
        FpFwListEntryInitialize(&tlm_pkg[i].list_entry);
        FpFwListInsertTail(&pkg_active_list, &tlm_pkg[i].list_entry);
    }

    expect_function_call(__wrap_dcs_client_flush_incoming_queue);
    will_return(__wrap_dcs_is_primary_instance, false);

    dcs_manager_handle_reset_msg(&trp_msg);

    assert_true(FpFwListIsEmpty(&pkg_active_list));
    assert_false(FpFwListIsEmpty(&pkg_free_list));
}
