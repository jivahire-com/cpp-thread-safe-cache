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

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <corebits.h>
#include <ddrss_runtime_api.h> // for DDRSS_MAX_MC_NUM_PER_DIE, DDRSS_MAX_PWR_TEL_EVT
#include <kng_soc_constants.h>
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

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_handle_enables_from_mcp_drop_count_only, test_setup, test_teardown)
{
    tlm_scp_record_enables_t test_enables = {{0}};

    // Enable vm_memory_pwr_en to take the positive path with all mocks
    // DDRSS_MAX_MC_NUM_PER_DIE MCs per die, each with DDRSS_MAX_PWR_TEL_EVT PMUs
    will_return(__wrap_mts_get_this_die_id, 0);

    // Mock ddrss_set_power_telemetry_config for each MC
    will_return_count(__wrap_ddrss_set_power_telemetry_config, 0, DDRSS_MAX_MC_NUM_PER_DIE); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_set_power_telemetry_config
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE);

    // Mock ddrss_set_power_telemetry_filter for each MC * event
    will_return_count(__wrap_ddrss_set_power_telemetry_filter, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_set_power_telemetry_filter
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Mock ddrss_pmu_init for each MC * event
    will_return_count(__wrap_ddrss_pmu_init, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_pmu_init
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Mock ddrss_pmu_set_event for each MC * event
    will_return_count(__wrap_ddrss_pmu_set_event, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_pmu_set_event
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Mock ddrss_pmu_enable for each MC * event
    will_return_count(__wrap_ddrss_pmu_enable, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_pmu_enable
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    test_enables.as_uint16 = 0x0002; // vm_memory_pwr_en is bit 1
    data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(test_enables);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0002);
    assert_int_equal(pwr_tlm_scp_record_enables.record.drop_count_en, 0);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 1);
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_handle_enables_from_mcp_all_disabled, test_setup, test_teardown)
{
    tlm_scp_record_enables_t test_enables = {{0}};

    // Disable all flags (DDRSS PMU disable calls expected for else path)
    // DDRSS_MAX_MC_NUM_PER_DIE MCs per die, each with DDRSS_MAX_PWR_TEL_EVT PMUs
    will_return(__wrap_mts_get_this_die_id, 0);

    // Mock ddrss_pmu_enable for disabling PMUs (else branch)
    will_return_count(__wrap_ddrss_pmu_enable, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT); // SILIBS_SUCCESS

    // Mock FPFW_RUNTIME_ASSERT_EXT for ddrss_pmu_enable
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

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

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_sync_enables_disabled, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH with disabled flags (else path)
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH;
    tlm_client_msg->payload.scp_records.as_uint16 = 0x0000; // Disable all flags

    // Mocks for data_proc_scp_tlm_cmpnt_handle_enables_from_mcp with vm_memory_pwr_en disabled (else path)
    will_return(__wrap_mts_get_this_die_id, 0);
    will_return_count(__wrap_ddrss_pmu_enable, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Mocks for data_proc_scp_tlm_cmpnt_init_core_vmin (called after handle_enables)
    // Mock all cores as disabled for simplicity
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        will_return(__wrap_core_info_get_enable_cores_result, 0x00000000);
    }
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_vmin, vmin_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_vmin, 1);

    mts_manager_scp_handle_trp_msg(&trp_msg);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0000);
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_sync_enables_vm_memory_enabled, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH with vm_memory_pwr_en enabled
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH;
    tlm_client_msg->payload.scp_records.as_uint16 = 0x0002; // vm_memory_pwr_en only

    // Mocks for data_proc_scp_tlm_cmpnt_handle_enables_from_mcp with vm_memory_pwr_en enabled
    will_return(__wrap_mts_get_this_die_id, 0);
    will_return_count(__wrap_ddrss_set_power_telemetry_config, 0, DDRSS_MAX_MC_NUM_PER_DIE);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE);
    will_return_count(__wrap_ddrss_set_power_telemetry_filter, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    will_return_count(__wrap_ddrss_pmu_init, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    will_return_count(__wrap_ddrss_pmu_set_event, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    will_return_count(__wrap_ddrss_pmu_enable, 0, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Mocks for data_proc_scp_tlm_cmpnt_init_core_vmin (called after handle_enables)
    // Mock all cores as disabled for simplicity
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        will_return(__wrap_core_info_get_enable_cores_result, 0x00000000);
    }
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_vmin, vmin_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_vmin, 1);

    mts_manager_scp_handle_trp_msg(&trp_msg);

    assert_int_equal(pwr_tlm_scp_record_enables.as_uint16, 0x0002);
    assert_int_equal(pwr_tlm_scp_record_enables.record.vm_memory_pwr_en, 1);
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_gen_pwr_package_both_disabled, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with both flags disabled
    // No data processing functions should be called
    pwr_tlm_scp_record_enables.as_uint16 = 0x0000;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    mts_manager_scp_handle_trp_msg(&trp_msg);

    // No assertions needed - test passes if no functions are called
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_gen_pwr_package_drop_count_only, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with drop_count_en only
    pwr_tlm_scp_record_enables.as_uint16 = 0x0001; // drop_count_en only

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    // Mock for data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_droop_counts, 1);

    mts_manager_scp_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_gen_pwr_package_vm_memory_only, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with vm_memory_pwr_en only
    pwr_tlm_scp_record_enables.as_uint16 = 0x0002; // vm_memory_pwr_en only

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    // Mocks for data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp
    will_return(__wrap_mts_get_this_die_id, 0);

    // Each ddrss_pmu_read_counter_snapshot call needs 2 return values: counter value and status
    for (uint16_t i = 0; i < DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT; i++)
    {
        will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // counter value
        will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // SILIBS_SUCCESS
    }

    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Expect write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, mpam_pmu_count_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, 1);

    mts_manager_scp_handle_trp_msg(&trp_msg);
}

TEST_FUNCTION(test_mts_manager_scp_handle_trp_msg_gen_pwr_package_both_enabled, test_setup, nullptr)
{
    trp_msg_t trp_msg = {{{{0}}}};

    // Test TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH with both flags enabled
    pwr_tlm_scp_record_enables.as_uint16 = 0x0003; // Both flags enabled

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_CLIENT_DEFINED;
    p_tlm_client_msg_t tlm_client_msg = (p_tlm_client_msg_t)trp_msg.payload.client_msg;
    tlm_client_msg->cmd = TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH;

    // Mock for data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_droop_counts, 1);

    // Mocks for data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp
    will_return(__wrap_mts_get_this_die_id, 0);

    // Each ddrss_pmu_read_counter_snapshot call needs 2 return values: counter value and status
    for (uint16_t i = 0; i < DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT; i++)
    {
        will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // counter value
        will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // SILIBS_SUCCESS
    }

    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Expect write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, mpam_pmu_count_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, 1);

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

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    will_return_always(__wrap__txe_block_pool_create, TX_SUCCESS);
    expect_function_calls(__wrap__txe_block_pool_create, 1);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    mts_client_register(MTS_CLIENT_ID_PWR_INST_TELEM, &s_pwr_tlm_mts_client_scp_test);
    mts_manager_scp_init();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp_die0, test_setup, test_teardown)
{
    // Test for die 0 (mc 0-11)
    will_return(__wrap_mts_get_this_die_id, 0);

    // Expect PMU counter reads for all MCs (0-11) and all PMU indices (0-7)
    for (uint16_t mc = 0; mc < DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        for (uint16_t pmu_idx = 0; pmu_idx < DDRSS_MAX_PWR_TEL_EVT; pmu_idx++)
        {
            // Return unique counter value for each MC/PMU combination for verification
            uint64_t counter_value = (uint64_t)mc * 1000 + pmu_idx;
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, counter_value);
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // SILIBS_SUCCESS
        }
    }

    // Expect FPFW_RUNTIME_ASSERT_EXT calls
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Expect the write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, mpam_pmu_count_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp_die1, test_setup, test_teardown)
{
    // Test for die 1 (mc 12-23)
    will_return(__wrap_mts_get_this_die_id, 1);

    // Expect PMU counter reads for all MCs on die 1 and all PMU indices
    for (uint16_t mc = 0; mc < DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        for (uint16_t pmu_idx = 0; pmu_idx < DDRSS_MAX_PWR_TEL_EVT; pmu_idx++)
        {
            // Return unique counter value
            uint64_t counter_value = (uint64_t)(mc + DDRSS_MAX_MC_NUM_PER_DIE) * 1000 + pmu_idx;
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, counter_value);
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // SILIBS_SUCCESS
        }
    }

    // Expect FPFW_RUNTIME_ASSERT_EXT calls
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Expect the write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, mpam_pmu_count_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp_counter_values, test_setup, test_teardown)
{
    // Test that counter values are properly accumulated in local array
    will_return(__wrap_mts_get_this_die_id, 0);

    // Set specific counter values to verify proper indexing
    uint64_t expected_counters[DDRSS_MAX_MC_NUM_PER_DIE][DDRSS_MAX_PWR_TEL_EVT];
    for (uint16_t mc = 0; mc < DDRSS_MAX_MC_NUM_PER_DIE; mc++)
    {
        for (uint16_t pmu_idx = 0; pmu_idx < DDRSS_MAX_PWR_TEL_EVT; pmu_idx++)
        {
            expected_counters[mc][pmu_idx] = 0x1000000000000000ULL + (mc << 8) + pmu_idx;
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, expected_counters[mc][pmu_idx]);
            will_return(__wrap_ddrss_pmu_read_counter_snapshot, 0); // SILIBS_SUCCESS
        }
    }

    // Expect FPFW_RUNTIME_ASSERT_EXT calls
    expect_function_calls(__wrap_FpFwAssertWithArgs, DDRSS_MAX_MC_NUM_PER_DIE * DDRSS_MAX_PWR_TEL_EVT);

    // Expect the write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, mpam_pmu_count_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_mpam_pmu_counts, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_init_core_vmin_all_cores_enabled, test_setup, test_teardown)
{
    // Test with all cores enabled
    // Mock core_info_get_enable_cores_result to return all cores enabled
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        will_return(__wrap_core_info_get_enable_cores_result, 0xFFFFFFFF);
    }

    // Mock power_runconfig_get_core_vmin_mv for each enabled core
    for (uint32_t core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        expect_value(__wrap_power_runconfig_get_core_vmin_mv, core_id, core_id);
        will_return(__wrap_power_runconfig_get_core_vmin_mv, 800 + core_id); // Unique Vmin per core
        will_return(__wrap_power_runconfig_get_core_vmin_mv, 0);             // Success status
    }

    // Expect FPFW_RUNTIME_ASSERT_EXT calls for each core
    expect_function_calls(__wrap_FpFwAssertWithArgs, NUM_AP_CORES_PER_DIE);

    // Expect write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_vmin, vmin_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_vmin, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_init_core_vmin();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_init_core_vmin_some_cores_disabled, test_setup, test_teardown)
{
    // Test with some cores disabled
    // Mock core_info_get_enable_cores_result to return pattern with some cores disabled
    // Enable cores 0, 2, 4, 6, etc. (even cores only)
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        will_return(__wrap_core_info_get_enable_cores_result, 0x55555555); // Alternating pattern
    }

    // Mock power_runconfig_get_core_vmin_mv only for enabled cores
    for (uint32_t core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        // Calculate bit position
        uint32_t word_idx = core_id / 32;
        uint32_t bit_idx = core_id % 32;
        bool is_enabled = (0x55555555 & (1U << bit_idx)) != 0;

        if (is_enabled && word_idx < BITTYPE_COUNT)
        {
            expect_value(__wrap_power_runconfig_get_core_vmin_mv, core_id, core_id);
            will_return(__wrap_power_runconfig_get_core_vmin_mv, 750 + core_id); // Unique Vmin per core
            will_return(__wrap_power_runconfig_get_core_vmin_mv, 0);             // Success status
        }
    }

    // Count enabled cores for FPFW_RUNTIME_ASSERT_EXT expectation
    uint32_t enabled_count = 0;
    for (uint32_t core_id = 0; core_id < NUM_AP_CORES_PER_DIE; core_id++)
    {
        uint32_t word_idx = core_id / 32;
        uint32_t bit_idx = core_id % 32;
        bool is_enabled = (0x55555555 & (1U << bit_idx)) != 0;
        if (is_enabled && word_idx < BITTYPE_COUNT)
        {
            enabled_count++;
        }
    }
    expect_function_calls(__wrap_FpFwAssertWithArgs, enabled_count);

    // Expect write to core exchange
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_vmin, vmin_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_vmin, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_init_core_vmin();
}

TEST_FUNCTION(test_data_proc_scp_tlm_cmpnt_init_core_vmin_all_cores_disabled, test_setup, test_teardown)
{
    // Test with all cores disabled
    // Mock core_info_get_enable_cores_result to return all cores disabled
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        will_return(__wrap_core_info_get_enable_cores_result, 0x00000000);
    }

    // No power_runconfig_get_core_vmin_mv calls expected since all cores are disabled

    // Expect write to core exchange with all zeros
    expect_any(__wrap_pwr_tlm_core_exch_scp_write_vmin, vmin_array);
    expect_function_calls(__wrap_pwr_tlm_core_exch_scp_write_vmin, 1);

    // Call the function under test
    data_proc_scp_tlm_cmpnt_init_core_vmin();
}
