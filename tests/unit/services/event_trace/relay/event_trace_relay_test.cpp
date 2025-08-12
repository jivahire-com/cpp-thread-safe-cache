//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_relay_test.cpp
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {

#include <ErrorHandler.h>
#include <FpFwLock.h>
#include <FpFwUtils.h>
#include <IFpFwEventTracingStatus.h>
#include <diag_decoder.h>
#include <error_handler.h>
#include <etr_init_config_i.h>
#include <event_trace_relay.h>
#include <event_trace_relay_i.h>
#include <fpfw_icc_base.h>
#include <idsw_kng.h>
#include <in_band_telemetry_ddr.h>
#include <thread_x_mocks.h>
#include <tx_api.h>
#include <tx_initialize.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_ASIC_COUNT (10)
#define TEST_HSP_COUNT  (10)

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

int test_setup(void** ppContext);
int test_teardown(void** ppContext);

/*-- Declarations (Statics and globals) --*/

extern "C" {

uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

uint8_t s_asic_ddr_memory[ASIC_BUFFER_PAYLOAD_SIZE * TEST_ASIC_COUNT] = {0};
uint8_t s_hsp_ddr_memory[HSP_BUFFER_PAYLOAD_SIZE * TEST_HSP_COUNT] = {0};
uint8_t s_test_stack[1024] = {0};

static uint32_t test_icc_base_ctx_hsp = 0;

// static fpfw_icc_base_ctx_t s_test_icc_base_hsp;

static etr_service_context_t s_test_context;

static etr_service_config_t s_test_config = {
    .soc_info =
        {
            .soc_id = 0,
            .die_id = 0,
        },
    .asic_ddr_config =
        {
            .base_addr = (uint64_t)&s_asic_ddr_memory[0],
            .size_bytes = sizeof(s_asic_ddr_memory),
        },
    .hsp_ddr_config =
        {
            .base_addr = (uint64_t)&s_hsp_ddr_memory[0],
            .size_bytes = sizeof(s_hsp_ddr_memory),
        },
    .thread_config =
        {
            .p_stack = s_test_stack,
            .stack_size = sizeof(s_test_stack),
            .priority = 10,
            .time_slice_option = TX_NO_TIME_SLICE,
        },
    .icc_config =
        {
            .p_hsp_icc_ctx = (fpfw_icc_base_ctx_t*)&test_icc_base_ctx_hsp,
        },
};

/*------------- Functions ----------------*/

void set_tx_queue_receive_value(VOID* destination_ptr)
{
    // Copy the parameter to the destination
    *(uint32_t*)destination_ptr = mock_type(int);
}

uint8_t __wrap_idsw_get_die_id(void)
{
    return DIE_0; // Mocked to return DIE_0 for testing purposes
}

uint32_t __wrap_atu_svc_accel_atu_addr(uint8_t accel_id)
{
    FPFW_UNUSED(accel_id);
    return 0;
}

void __wrap_mts_client_send_trp_response(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    // This function is mocked in the test
}

void __wrap_mts_client_send_new_trp_msg(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    // This function is mocked in the test
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return 0;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

UINT __wrap__txe_block_pool_create(TX_BLOCK_POOL* pool_ptr, CHAR* name_ptr, ULONG block_size, VOID* pool_start, ULONG pool_size, UINT pool_control_block_size)
{
    check_expected_ptr(pool_ptr);
    check_expected_ptr(name_ptr);
    check_expected(block_size);
    check_expected_ptr(pool_start);
    check_expected(pool_size);
    check_expected(pool_control_block_size);

    return mock_type(UINT);
}

UINT __wrap__txe_block_release(VOID* block_ptr)
{
    check_expected_ptr(block_ptr);
    return mock_type(UINT);
    ;
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(params);

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    FPFW_UNUSED(buffer_size);

    return mock_type(fpfw_status_t);
}

void __wrap_mts_client_register(mts_client_id_t id, p_mts_client_t client)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(client);
    // This function is mocked in the test
}

} // extern "C"

int test_setup(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    memset(&s_test_context, 0, sizeof(s_test_context));
    memset(&s_asic_ddr_memory, 0, sizeof(s_asic_ddr_memory));
    memset(&s_hsp_ddr_memory, 0, sizeof(s_hsp_ddr_memory));

    // Setup the threadx expectations
    expect_any(__wrap__txe_block_pool_create, pool_ptr);
    expect_value(__wrap__txe_block_pool_create, name_ptr, ETR_BLOCK_POOL_NAME);
    expect_value(__wrap__txe_block_pool_create, block_size, MAX_TRP_MSG_BLOCK_SIZE);
    expect_any(__wrap__txe_block_pool_create, pool_start);
    expect_any(__wrap__txe_block_pool_create, pool_size);
    expect_any(__wrap__txe_block_pool_create, pool_control_block_size);
    will_return(__wrap__txe_block_pool_create, TX_SUCCESS);

    expect_any(__wrap__txe_queue_create, queue_ptr);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_any(__wrap__txe_queue_create, queue_start);
    expect_any(__wrap__txe_queue_create, queue_size);
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_thread_create, thread_ptr, &s_test_context.worker_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, ETR_WORKER_THREAD_NAME);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_value(__wrap__txe_thread_create, stack_start, s_test_config.thread_config.p_stack);
    expect_value(__wrap__txe_thread_create, stack_size, s_test_config.thread_config.stack_size);
    expect_value(__wrap__txe_thread_create, priority, s_test_config.thread_config.priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    expect_any(__wrap__txe_event_flags_create, group_ptr);
    expect_any(__wrap__txe_event_flags_create, name_ptr);
    expect_any(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

    // Setup ICC expectations
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    etr_initialize(&s_test_context, &s_test_config);

    return TX_SUCCESS;
}

int test_teardown(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    return TX_SUCCESS;
}

// TEST PUBLIC ETR FUNCTIONS

TEST_FUNCTION(test_etr_init_null_param, nullptr, nullptr)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etr_initialize(nullptr, &s_test_config);
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, nullptr);
    }
}

TEST_FUNCTION(test_etr_init_bad_ddr_config, nullptr, nullptr)
{
    etr_service_config_t bad_config = s_test_config;
    bad_config.hsp_ddr_config.base_addr = 0;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.base_addr = 0;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.asic_ddr_config.size_bytes = ASIC_BUFFER_PAYLOAD_SIZE - 1;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.size_bytes = HSP_BUFFER_PAYLOAD_SIZE - 1;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.asic_ddr_config.size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE + 1;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE + 1;

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }
}

TEST_FUNCTION(test_etr_init_nominal, test_setup, test_teardown)
{
    // validate that the ddr buffer information was setup
    for (uint32_t i = 0; i < TEST_ASIC_COUNT; i++)
    {
        ddr_buffer_info_t* buffer = &s_test_context.ddr_buffers[i];
        assert_int_equal(buffer->state, ETR_DDR_BUFFER_STATE_FREE);
        assert_int_equal(buffer->type, DIAG_PAYLOAD_PARSER_TRACE_DEVICE);
    }

    for (size_t i = ASIC_BUFFER_DDR_CAPACITY_MAX; i < ASIC_BUFFER_DDR_CAPACITY_MAX + TEST_HSP_COUNT; i++)
    {
        ddr_buffer_info_t* buffer = &s_test_context.ddr_buffers[i];
        assert_int_equal(buffer->state, ETR_DDR_BUFFER_STATE_FREE);
        assert_int_equal(buffer->type, DIAG_PAYLOAD_PARSER_HSP_TRACE);
    }
}

// TEST PRIVATE HEADER ETR FUNCTIONS

// Mock Structures to pass on to the ETR functions
FPFW_ET_CORE_BUFFER_HEADER fake_core_buffer = {
    .ManifestId = {0},
    .StartAsicTimeStamp = 0,
    .EndAsicTimeStamp = 100,
    .StartUTCTimeStamp = 0,
    .EndUTCTimeStamp = 100,
    .ETVersion = 0,
    // .CoreId = CPU_SDM,
    .ControllerId = 0,
    .BufferId = 0,
    .BufferSize = 1024,
    .UsedBytes = 0,
    .LostEvents = 0,
};

trp_msg_t trp_msg = {
    .hdr =
        {

            .src_node =
                {
                    // .core_id = CPU_SDM,
                    .die_id = 0,
                },
            .dest_node =
                {
                    .core_id = CPU_MCP,
                    .die_id = 0,
                },
            .trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION,
            .payload_size = sizeof(fake_core_buffer),
        },
    .payload = {.intercore_block_notification =
                    {
                        .addr_offset = (uintptr_t)&fake_core_buffer - 0x80000, // Adjust for DTCM Offset for SDM
                        .block_size = sizeof(fake_core_buffer),
                    }},
};

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_available, test_setup, test_teardown)
{
    // Test with DM Core
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;

    etr_ddr_buffer_state_t pre_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t pre_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    etr_ddr_buffer_state_t post_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t post_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    assert_int_equal(pre_request_state, post_request_state);
    assert_true(pre_request_size < post_request_size);
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    assert_memory_equal(&fake_core_buffer, asic_buffer_addr, sizeof(fake_core_buffer));
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_maxed, test_setup, test_teardown)
{

    // Run test case with CDED Core.
    trp_msg.hdr.src_node.core_id = CPU_CDED_SDM;
    fake_core_buffer.CoreId = CPU_CDED_SDM;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Fake out the active buffer to be full
    ddr_buffer_info_t* p_old_asic_buffer = s_test_context.p_active_asic_buffer;
    p_old_asic_buffer->buffer.asic.asic_header.UsedBytes = ASIC_BUFFER_PAYLOAD_SIZE - sizeof(asic_buffer_info_t);

    fake_core_buffer.UsedBytes = sizeof(fake_core_buffer);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    ddr_buffer_info_t* p_new_asic_buffer = s_test_context.p_active_asic_buffer;

    assert_ptr_not_equal(p_old_asic_buffer, p_new_asic_buffer);

    // validate that the old buffer is now pending and that it's header was updated in memory
    assert_int_equal(p_old_asic_buffer->state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(p_new_asic_buffer->state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_memory_equal(&p_old_asic_buffer->buffer.asic, p_old_asic_buffer->payload_management.base_addr, sizeof(asic_buffer_info_t));
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_no_free_asics, test_setup, test_teardown)
{
    // Run test case with SCP Core
    trp_msg.hdr.src_node.core_id = CPU_SCP;
    trp_msg.payload.intercore_block_notification.addr_offset = (uintptr_t)&fake_core_buffer;
    fake_core_buffer.CoreId = CPU_SCP;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    assert_int_equal(s_test_context.health_stats.asic_buffers_reused, 0);

    // Fake out all asic buffers to be full
    for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
    {
        s_test_context.ddr_buffers[i].state = ETR_DDR_BUFFER_STATE_PENDING;
        s_test_context.ddr_buffers[i].buffer.asic.asic_header.UsedBytes =
            ASIC_BUFFER_PAYLOAD_SIZE - sizeof(asic_buffer_info_t);
    }

    // Setup the active buffer to be active
    s_test_context.p_active_asic_buffer = &s_test_context.ddr_buffers[1];
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;

    fake_core_buffer.UsedBytes = sizeof(fake_core_buffer);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    // validate that the old buffer is now pending and that first pending buffer is now active
    assert_int_equal(s_test_context.ddr_buffers[1].state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(s_test_context.ddr_buffers[0].state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_int_equal(s_test_context.health_stats.asic_buffers_reused, 1);
}

trp_msg_t trp_msg_host = {
    .hdr =
        {

            .src_node =
                {
                    .core_id = CPU_AP,
                    .die_id = 0,
                },
            .dest_node =
                {
                    .core_id = CPU_MCP,
                    .die_id = 0,
                },
            .trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE,
            .payload_size = sizeof(fake_core_buffer),
        },
    .payload = {.intercore_block_notification =
                    {
                        .addr_offset = (uintptr_t)&fake_core_buffer,
                        .block_size = sizeof(fake_core_buffer),
                    }},
};

TEST_FUNCTION(test_etr_process_request_host_read_bad_addr, test_setup, test_teardown)
{
    // Set an invalid/bad address for the host read request
    trp_msg_host.payload.intercore_block_notification.addr_offset = 0xDEADBEEF;

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        // Set Expectations for successful tx_event_flags_get
        expect_any_always(__wrap__txe_event_flags_get, group_ptr);
        expect_any_always(__wrap__txe_event_flags_get, requested_flags);
        expect_any_always(__wrap__txe_event_flags_get, get_option);
        expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
        expect_any_always(__wrap__txe_event_flags_get, wait_option);
        will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

        // Set up expectations for a successful tx_queue_receive
        expect_any_always(__wrap__txe_queue_receive, queue_ptr);
        expect_any_always(__wrap__txe_queue_receive, destination_ptr);
        expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
        will_return(__wrap__txe_queue_receive, TX_SUCCESS);

        will_return(set_tx_queue_receive_value, &trp_msg_host);
        set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

        etr_worker_thread_func((ULONG)&s_test_context);
    }
}

TEST_FUNCTION(test_etr_process_request_host_read_good_addr, test_setup, test_teardown)
{
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    // Set a good address for the host read request
    trp_msg_host.payload.intercore_block_notification.addr_offset =
        s_test_context.p_active_asic_buffer->payload_management.base_addr;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(s_test_context.p_active_asic_buffer->state, ETR_DDR_BUFFER_STATE_FREE);
}

TEST_FUNCTION(test_etr_process_request_host_read_good_addr_bad_state, test_setup, test_teardown)
{
    assert_int_equal(s_test_context.health_stats.delayed_host_reads, 0);

    // Set Buffer active state to pending
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(s_test_context.health_stats.delayed_host_reads, 1);
}

TEST_FUNCTION(test_etr_icc_handle_hsp, test_setup, test_teardown)
{
    // The HSP ICC interface simply handles a request, sends a response, and sets up another receive
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    etr_icc_handle_hsp((void*)&s_test_context, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
}
