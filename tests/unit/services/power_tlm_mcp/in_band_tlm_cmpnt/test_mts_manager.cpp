//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mts_manager.cpp
 * Test manager for data collection service
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <ddr_manager_i.h>
#include <in_band_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <mts_manager_i.h>
#include <package_creation_i.h>
#include <pwr_tlm_core_exchange.h>
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

TEST_FUNCTION(test_mts_manager_queue_tlm_package, test_setup, test_teardown)
{
    uint8_t buffer[1000] = {0};

    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    // test notification of adding to empty list and notifying host
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_dcp_notification);
    mts_manager_queue_tlm_package((uintptr_t)buffer, 1000);

    // active list isn't empty so no dcp notification
    will_return(__wrap_mts_is_primary_instance, true);
    mts_manager_queue_tlm_package((uintptr_t)buffer, 1000);

    // secondary instance will queue trp message to primary
    will_return(__wrap_mts_is_primary_instance, false);
    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_queue_tlm_package((uintptr_t)buffer, 1000);
}

TEST_FUNCTION(test_mts_manager_handle_record_enable_disable, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

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
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_SOC_PKG_MON;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].event_id = INST_TELEMETRY_ELEMENT_CORE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_true(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PKG_MON]);
    assert_true(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_DISABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[1].state = DCP_EVENTS_ENABLE_STATE_DISABLE;
    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_false(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PKG_MON]);
    assert_false(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    // Test enabling POWER_TELEMETRY_ELEMENT_CORE_DROOPS - should send to SCP
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 1;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_CORE_DROOPS;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_true(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_DROOPS]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    // Test disabling POWER_TELEMETRY_ELEMENT_CORE_DROOPS - should send to SCP
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_DISABLE;

    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_false(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_DROOPS]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    // Test enabling POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER - should send to SCP
    // Enable the MPAM VM memory reporting knob for this test
    mpam_vm_mem_reporting_knob_enable = true;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_true(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    // Test disabling POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER - should send to SCP
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_DISABLE;

    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_handle_record_enable_disable(&trp_msg);

    assert_false(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER]);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    // Reset the MPAM VM memory reporting knob
    mpam_vm_mem_reporting_knob_enable = false;
}

TEST_FUNCTION(test_mts_manager_handle_record_enable_disable_fail, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = DCP_MAX_ENABLE_DISABLE_EVENTS + 1;
    mts_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 1;

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = POWER_TELEMETRY_ELEMENT_ID_MAX;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;

    mts_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = INST_TELEMETRY_ELEMENT_ID_MAX;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;
    mts_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = 0xFFFF;
    mts_manager_handle_record_enable_disable(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_not_supported, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // not supported on secondary instance
    will_return(__wrap_mts_is_primary_instance, false);
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_E_UNSUPPORTED_MSG);

    will_return(__wrap_mts_is_primary_instance, false);
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_E_UNSUPPORTED_MSG);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_get_capabilities, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    mts_manager_handle_dcp_msg(&trp_msg);

    dcp_msg_get_caps_t exp_caps = {{{0}}};
    exp_caps.caps.DCP_MSG_ID_GET_CAPABILITIES = 1;
    exp_caps.caps.DCP_MSG_ID_GET_STATE = 1;
    exp_caps.caps.DCP_MSG_ID_EVENTS_ENABLE_DISABLE = 1;
    exp_caps.caps.DCP_MSG_ID_START_STOP = 1;
    exp_caps.caps.DCP_MSG_ID_READ_DATA = 1;
    exp_caps.caps.DCP_MSG_ID_READ_DATA_COMPLETE = 1;
    exp_caps.caps.DCP_MSG_ID_RESET = 1;
    p_dcp_msg_get_caps_t act_caps = &trp_msg.payload.dcp_msg.payload.get_caps;
    assert_int_equal(act_caps->caps.as_uint32, exp_caps.caps.as_uint32);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_get_caps_t));

    will_return(__wrap_mts_is_primary_instance, false);
    mts_manager_handle_dcp_msg(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_E_UNSUPPORTED_MSG);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_get_state, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return(__wrap_mts_is_primary_instance, true);
    will_return(exec_tlm_cmpnt_is_telemetry_publishing_enabled, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_get_client_state_t));
    assert_int_equal(trp_msg.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_RUNNING);

    will_return(__wrap_mts_is_primary_instance, true);
    will_return(exec_tlm_cmpnt_is_telemetry_publishing_enabled, false);
    expect_function_call(__wrap_mts_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_get_client_state_t));
    assert_int_equal(trp_msg.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_STOPPED);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_en_dis, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_EVENTS_ENABLE_DISABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = DCP_MAX_ENABLE_DISABLE_EVENTS + 1;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_forward_trp_msg);
    expect_function_call(__wrap_mts_client_send_trp_response);

    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].provider_id = EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].event_id = INST_TELEMETRY_ELEMENT_CORE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.events[0].state = DCP_EVENTS_ENABLE_STATE_ENABLE;
    trp_msg.payload.dcp_msg.payload.events_enable_disable.number_of_events = 1;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_start_stop, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return(__wrap_mts_is_primary_instance, false);
    expect_function_call(exec_tlm_cmpnt_change_telemetry_mode);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    trp_msg.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_STOP;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);

    will_return(__wrap_mts_is_primary_instance, false);
    expect_function_call(exec_tlm_cmpnt_change_telemetry_mode);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;
    trp_msg.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_START;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mts_manager_handle_dcp_msg_unsupported, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return(__wrap_mts_is_primary_instance, false);

    trp_msg.payload.dcp_msg.hdr.msg_id = 0xFFFF;

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_E_UNSUPPORTED_MSG);
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_mts_msg_dcp, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(__wrap_mts_is_primary_instance, true);
    will_return(exec_tlm_cmpnt_is_telemetry_publishing_enabled, false);

    expect_function_call(__wrap_mts_client_send_trp_response);

    expect_function_calls(__wrap__txe_block_release, 1);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    in_band_tlm_cmpnt_handle_incoming_mts_msgs();
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_mts_msg_trp, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
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

    in_band_tlm_cmpnt_handle_incoming_mts_msgs();
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_handle_incoming_mts_msg_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    p_trp_msg_t p_trp_msg = &trp_msg;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_any_always(__wrap__txe_queue_receive, wait_option);

    will_return(__wrap__txe_queue_receive, sizeof(p_trp_msg_t));
    will_return(__wrap__txe_queue_receive, &p_trp_msg);
    will_return(__wrap__txe_queue_receive, TX_SIZE_ERROR);

    in_band_tlm_cmpnt_handle_incoming_mts_msgs();
}

TEST_FUNCTION(test_mts_manager_send_trp_package_helper, test_setup, nullptr)
{
    tlm_package_t tlm_pkg = {{0}};

    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    mts_manager_send_trp_package_helper(&tlm_pkg, TRP_MSG_ID_READ_INTERCORE_BLOCK, 2, 3);
}

TEST_FUNCTION(test_mts_manager_send_trp_pkg_notification_to_primary, test_setup, nullptr)
{
    tlm_package_t tlm_pkg = {{0}};

    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    mts_manager_send_trp_pkg_notification_to_primary(&tlm_pkg);
}

TEST_FUNCTION(test_mts_manager_send_trp_read_complete, test_setup, nullptr)
{
    tlm_package_t tlm_pkg = {{0}};

    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    mts_manager_send_trp_read_complete(&tlm_pkg);
}

TEST_FUNCTION(test_mts_manager_get_pkg_from_free_list, test_setup, nullptr)
{
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    // secondary instance should return null
    will_return(__wrap_mts_is_primary_instance, false);
    p_tlm_package_t tlm_pkg = mts_manager_get_pkg_from_free_list();
    assert_null(tlm_pkg);

    // primary instance with an empty active list should return null
    will_return(__wrap_mts_is_primary_instance, true);
    tlm_pkg = mts_manager_get_pkg_from_free_list();
    assert_null(tlm_pkg);

    // an entry in the active list will be removed and returned
    FpFwListEntryInitialize(&mts_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[1].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[1].list_entry);

    will_return(__wrap_mts_is_primary_instance, true);
    tlm_pkg = mts_manager_get_pkg_from_free_list();
    assert_int_equal(tlm_pkg, &mts_active_pkg_buffer[0]);

    will_return(__wrap_mts_is_primary_instance, true);
    tlm_pkg = mts_manager_get_pkg_from_free_list();
    assert_int_equal(tlm_pkg, &mts_active_pkg_buffer[1]);
}

TEST_FUNCTION(test_mts_manager_free_tlm_package_from_secondary_mcp, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};

    tlm_package_t tlm_pkg = {{0}};

    tlm_pkg.pkg.source_die_id = 1;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    mts_active_pkg_buffer[2].pkg.source_die_id = 1;
    mts_active_pkg_buffer[2].pkg.source_core_id = 2;
    mts_active_pkg_buffer[2].pkg.reserved = 0;
    mts_active_pkg_buffer[2].pkg.local_mmap_addr = (uintptr_t)buffer;
    mts_active_pkg_buffer[2].pkg.phy_addr_offset = 0;
    mts_active_pkg_buffer[2].pkg.pkg_size = 1000;
    mts_active_pkg_buffer[2].pkg.crc = 0x4267;

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[2].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[2].list_entry);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[1].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[1].list_entry);

    mts_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);

    PFPFW_LIST_ENTRY entry = FpFwListRemoveHead(&pkg_free_list);
    assert_int_equal(entry, &mts_active_pkg_buffer[2].list_entry);

    mts_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_free_list)); // no match so was not added to free list

    // verify empty list causes no harm
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    mts_manager_free_tlm_package_from_secondary_mcp(&tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_free_list));
    assert_true(FpFwListIsEmpty(&pkg_active_list));
}

TEST_FUNCTION(test_mts_manager_handle_trp_msg_pkg_compl, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    uint8_t buffer[1000] = {0};

    trp_msg.payload.package_notification.source_die_id = 0;
    trp_msg.payload.package_notification.source_core_id = 2;
    trp_msg.payload.package_notification.reserved = 0;
    trp_msg.payload.package_notification.local_mmap_addr = (uintptr_t)buffer;
    trp_msg.payload.package_notification.phy_addr_offset = 0x4000;
    trp_msg.payload.package_notification.pkg_size = 1000;
    trp_msg.payload.package_notification.crc = 0x4267;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;

    mts_active_pkg_buffer[2].pkg.source_die_id = 0;
    mts_active_pkg_buffer[2].pkg.source_core_id = 2;
    mts_active_pkg_buffer[2].pkg.reserved = 0;
    mts_active_pkg_buffer[2].pkg.local_mmap_addr = (uintptr_t)buffer;
    mts_active_pkg_buffer[2].pkg.phy_addr_offset = 0x4000;
    mts_active_pkg_buffer[2].pkg.pkg_size = 1000;
    mts_active_pkg_buffer[2].pkg.crc = 0x4267;

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[0].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[0].list_entry);

    FpFwListEntryInitialize(&mts_active_pkg_buffer[2].list_entry);
    FpFwListInsertTail(&pkg_active_list, &mts_active_pkg_buffer[2].list_entry);

    mts_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_handle_trp_msg_client_defined, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_SET_MODE_PUSH;
    tlm_client_msg->payload.mode = TLM_OP_MODE_COLLECTING_DATA;
    expect_function_call(exec_tlm_cmpnt_change_telemetry_mode);

    mts_manager_handle_trp_msg(&trp_msg);

    tlm_client_msg->cmd = 0xFFFF;
    tlm_client_msg->payload.mode = TLM_OP_MODE_COLLECTING_DATA;

    mts_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_handle_trp_msg_client_defined_2, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_PRIM_MCP_2_SEC_MCP_PUSH;

    expect_function_call(data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core);

    mts_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_free_tlm_package_from_primary_mcp, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    FpFwListInitialize(&pkg_free_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    mts_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);

    tlm_pkg.pkg.source_die_id = 3;
    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 1;
    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_free_tlm_package_from_primary_mcp(&tlm_pkg);
}

TEST_FUNCTION(test_mts_manager_handle_read_complete_msg, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    trp_msg_t trp_msg = {{{{0}}}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    in_flight_tlm_pkg = &tlm_pkg;

    trp_msg.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = 0x4000;
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    will_return(__wrap_mts_is_primary_instance, false);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_NONE);
    assert_null(in_flight_tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_free_list));

    tlm_package_t tlm_pkg_2 = {{0}};
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg_2.list_entry);
    in_flight_tlm_pkg = &tlm_pkg;
    will_return(__wrap_mts_is_primary_instance, false);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_null(in_flight_tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_free_list));
}

TEST_FUNCTION(test_mts_manager_handle_read_complete_msg_fail, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    trp_msg_t trp_msg = {{{{0}}}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    in_flight_tlm_pkg = nullptr;

    trp_msg.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = 0x4001;
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;
    will_return(__wrap_mts_is_primary_instance, false);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_BUFFER_DISCARD);

    in_flight_tlm_pkg = &tlm_pkg;
    will_return(__wrap_mts_is_primary_instance, false);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DCP_STATUS_E_PARAM);
}

TEST_FUNCTION(test_mts_manager_handle_read_msg, test_setup, nullptr)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    tlm_package_t tlm_pkg_2 = {{0}};
    trp_msg_t trp_msg = {{{{0}}}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;
    in_flight_tlm_pkg = nullptr;

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_start_addr, IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_buffer_size, IB_TELEMETRY_DDR_TOTAL_SIZE);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_addr_offset, in_flight_tlm_pkg->pkg.phy_addr_offset);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_size, in_flight_tlm_pkg->pkg.pkg_size);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.crc, in_flight_tlm_pkg->pkg.crc);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_LAST);
    assert_true(in_flight_tlm_pkg == &tlm_pkg);
    assert_true(FpFwListIsEmpty(&pkg_active_list));

    in_flight_tlm_pkg = nullptr;
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg_2.list_entry);

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_start_addr, IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.physical_buffer_size, IB_TELEMETRY_DDR_TOTAL_SIZE);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_addr_offset, in_flight_tlm_pkg->pkg.phy_addr_offset);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.rd_data_size, in_flight_tlm_pkg->pkg.pkg_size);
    assert_int_equal(trp_msg.payload.dcp_msg.payload.read_data.crc, in_flight_tlm_pkg->pkg.crc);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_true(in_flight_tlm_pkg == &tlm_pkg);
    assert_false(FpFwListIsEmpty(&pkg_active_list));
}

TEST_FUNCTION(test_mts_manager_handle_read_msg_fail, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    tlm_package_t tlm_pkg = {{0}};
    tlm_package_t tlm_pkg_2 = {{0}};

    // verify no data
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    mts_manager_handle_read_msg(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_NONE);

    in_flight_tlm_pkg = &tlm_pkg_2;
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);

    mts_manager_handle_read_msg(&trp_msg);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
}

TEST_FUNCTION(test_mts_manager_handle_trp_msg_pkg_notif, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};
    tlm_package_t tlm_pkg = {{0}};
    uint8_t buffer[1000] = {0};

    // verify no data
    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    FpFwListInsertTail(&pkg_free_list, &tlm_pkg.list_entry);

    trp_msg.payload.package_notification.source_die_id = 0;
    trp_msg.payload.package_notification.source_core_id = 2;
    trp_msg.payload.package_notification.reserved = 0;
    trp_msg.payload.package_notification.local_mmap_addr = (uintptr_t)buffer;
    trp_msg.payload.package_notification.phy_addr_offset = 0x4000;
    trp_msg.payload.package_notification.pkg_size = 1000;
    trp_msg.payload.package_notification.crc = 0x4267;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION;

    will_return(__wrap_mts_is_primary_instance, false);
    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    mts_manager_handle_trp_msg(&trp_msg);

    PFPFW_LIST_ENTRY entry = NULL;
    entry = FpFwListRemoveHead(&pkg_active_list);
    p_tlm_package_t queued_pkg = CONTAINING_RECORD(entry, tlm_package_t, list_entry);

    assert_int_equal(queued_pkg->pkg.source_die_id, 0);
    assert_int_equal(queued_pkg->pkg.source_core_id, 2);
    assert_int_equal(queued_pkg->pkg.reserved, 0);
    assert_int_equal(queued_pkg->pkg.local_mmap_addr, (uintptr_t)buffer);
    assert_int_equal(queued_pkg->pkg.phy_addr_offset, 0x4000);
    assert_int_equal(queued_pkg->pkg.pkg_size, 1000);
    assert_int_equal(queued_pkg->pkg.crc, 0x4267);

    // free list is now empty, test that read complete is sent back to sender
    will_return(__wrap_mts_is_primary_instance, false);
    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_free_publish_resources, test_setup, nullptr)
{
    tlm_package_t tlm_pkg[4] = {{{0}}};
    uint8_t buffer[1000] = {0};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    // setup two local packages and 2 remote
    for (int i = 0; i < 4; i++)
    {
        tlm_pkg[i].pkg.source_die_id = 0;
        tlm_pkg[i].pkg.source_core_id = 2;
        tlm_pkg[i].pkg.reserved = 0;
        tlm_pkg[i].pkg.local_mmap_addr = (uintptr_t)buffer;
        tlm_pkg[i].pkg.phy_addr_offset = 0x4000 + (i * 1000);
        tlm_pkg[i].pkg.pkg_size = 1000;
        tlm_pkg[i].pkg.crc = 0x4267 + i;
        FpFwListEntryInitialize(&tlm_pkg[i].list_entry);
        FpFwListInsertTail(&pkg_active_list, &tlm_pkg[i].list_entry);
    }
    tlm_pkg[1].pkg.source_die_id = 1;
    tlm_pkg[3].pkg.source_core_id = 1;

    tlm_package_t in_flight;
    in_flight.pkg.local_mmap_addr = (uintptr_t)buffer;
    in_flight_tlm_pkg = &in_flight;

    expect_function_call(__wrap_mts_client_flush_incoming_queue);
    will_return(__wrap_mts_is_primary_instance, true);

    mts_manager_free_publish_resources();

    assert_true(FpFwListIsEmpty(&pkg_active_list));
    assert_false(FpFwListIsEmpty(&pkg_free_list));

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);
    // setup two local packages and 2 remote
    for (int i = 0; i < 4; i++)
    {
        tlm_pkg[i].pkg.source_die_id = 1;
        tlm_pkg[i].pkg.source_core_id = 2;
        tlm_pkg[i].pkg.reserved = 0;
        tlm_pkg[i].pkg.local_mmap_addr = (uintptr_t)buffer;
        tlm_pkg[i].pkg.phy_addr_offset = 0x4000 + (i * 1000);
        tlm_pkg[i].pkg.pkg_size = 1000;
        tlm_pkg[i].pkg.crc = 0x4267 + i;
        FpFwListEntryInitialize(&tlm_pkg[i].list_entry);
        FpFwListInsertTail(&pkg_active_list, &tlm_pkg[i].list_entry);
    }

    expect_function_call(__wrap_mts_client_flush_incoming_queue);
    will_return(__wrap_mts_is_primary_instance, false);

    mts_manager_free_publish_resources();

    assert_true(FpFwListIsEmpty(&pkg_active_list));
    assert_false(FpFwListIsEmpty(&pkg_free_list));
}

TEST_FUNCTION(test_mts_manager_handle_reset, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    will_return_always(__wrap_mts_is_primary_instance, false);
    expect_function_call(exec_tlm_cmpnt_change_telemetry_mode);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_RESET;
    mts_manager_handle_dcp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_send_mode_to_sec_cores, test_setup, nullptr)
{
    will_return(__wrap_mts_is_primary_instance, false);
    mts_manager_send_mode_to_sec_cores(TLM_OP_MODE_PUBLISHING);

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_forward_trp_msg);
    mts_manager_send_mode_to_sec_cores(TLM_OP_MODE_PUBLISHING);
}

TEST_FUNCTION(test_mts_manager_send_prep_pwr_pkg_notification_to_sec_mcps, test_setup, test_teardown)
{
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_forward_trp_msg);
    mts_manager_send_prep_pwr_pkg_notification_to_sec_mcps();
}

TEST_FUNCTION(test_mts_manager_send_prep_pwr_pkg_notification_to_scp, test_setup, test_teardown)
{
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_new_trp_msg);
    mts_manager_send_prep_pwr_pkg_notification_to_scp();
}

TEST_FUNCTION(test_mts_manager_handle_dcp_read_data_response_size_valid_data, test_setup, test_teardown)
{
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    trp_msg_t trp_msg = {{{{0}}}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    trp_msg.payload.dcp_msg.hdr.msg_status = DCP_STATUS_SUCCESS;
    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;
    in_flight_tlm_pkg = nullptr;

    // Test case where msg_status = DATA_COLLECTION_RD_DATA_VALID_LAST (>= threshold)
    // Should set response_dcp_payload_size = sizeof(dcp_msg_read_data_t)
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_LAST);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));

    // Reset for next test
    in_flight_tlm_pkg = nullptr;
    FpFwListInitialize(&pkg_active_list);
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);

    // Add another package to make it return DATA_COLLECTION_RD_DATA_VALID_MORE
    tlm_package_t tlm_pkg_2 = {{0}};
    FpFwListInsertTail(&pkg_active_list, &tlm_pkg_2.list_entry);

    // Test case where msg_status = DATA_COLLECTION_RD_DATA_VALID_MORE (>= threshold)
    // Should set response_dcp_payload_size = sizeof(dcp_msg_read_data_t)
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
}

TEST_FUNCTION(test_mts_manager_handle_dcp_read_data_response_size_error_conditions, test_setup, test_teardown)
{
    trp_msg_t trp_msg = {{{{0}}}};

    FpFwListInitialize(&pkg_free_list);
    FpFwListInitialize(&pkg_active_list);

    trp_msg.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;
    in_flight_tlm_pkg = nullptr;

    // Test case 1: Empty list - should set msg_status to DATA_COLLECTION_RD_DATA_NONE (< threshold)
    // Should set response_dcp_payload_size = 0
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_NONE);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, 0);

    // Test case 2: In-flight package already exists - should set msg_status to DCP_STATUS_E_BUSY (<
    // threshold) Should set response_dcp_payload_size = 0
    uint8_t buffer[1000] = {0};
    tlm_package_t tlm_pkg = {{0}};
    tlm_package_t in_flight_pkg = {{0}};

    tlm_pkg.pkg.source_die_id = 0;
    tlm_pkg.pkg.source_core_id = 2;
    tlm_pkg.pkg.reserved = 0;
    tlm_pkg.pkg.local_mmap_addr = (uintptr_t)buffer;
    tlm_pkg.pkg.phy_addr_offset = 0x4000;
    tlm_pkg.pkg.pkg_size = 1000;
    tlm_pkg.pkg.crc = 0x4267;

    FpFwListInsertTail(&pkg_active_list, &tlm_pkg.list_entry);
    in_flight_tlm_pkg = &in_flight_pkg; // Set an in-flight package

    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_trp_response);

    mts_manager_handle_dcp_msg(&trp_msg);

    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.payload_size, sizeof(dcp_msg_read_data_t));
}

TEST_FUNCTION(test_mts_manager_send_record_enables_to_scp, test_setup, test_teardown)
{
    tlm_scp_record_enables_t enables = {{0}};
    enables.record.drop_count_en = 1;
    enables.record.vm_memory_pwr_en = 1;

    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    mts_manager_send_record_enables_to_scp(&enables);
}