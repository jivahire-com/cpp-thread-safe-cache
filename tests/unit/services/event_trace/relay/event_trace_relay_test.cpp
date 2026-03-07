//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_relay_test.cpp
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {

#include <DfwkClient.h>
#include <DfwkHost.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <accelip_id.h>
#include <etr_init_config_i.h>
#include <event_trace_relay.h>
#include <event_trace_relay_i.h>
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <startup_shutdown.h>
#include <stdnoreturn.h>
#include <thread_x_mocks.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_ASIC_COUNT (10)
#define TEST_HSP_COUNT  (4)

#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

int test_setup_die0(void** ppContext);
int test_teardown(void** ppContext);

/*-- Declarations (Statics and globals) --*/

extern "C" {

uint8_t s_asic_ddr_memory[ASIC_BUFFER_PAYLOAD_SIZE * TEST_ASIC_COUNT] = {0};
uint8_t s_hsp_ddr_memory[HSP_BUFFER_PAYLOAD_SIZE * TEST_HSP_COUNT] = {0};
uint8_t s_test_stack[1024] = {0};
// Global variable used to set __wrap_accel_is_isolation_enabled return value
bool accel_isolation = false;

static jmp_buf mock_jump_buf;

static uint32_t test_icc_base_ctx_hsp = 0;

static etr_service_context_t s_test_context;

static etr_service_config_t s_test_config = {
    .soc_info =
        {
            .soc_id = {0},
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

uint8_t __wrap_mts_get_this_die_id(void)
{
    return mock_type(uint8_t);
}

uint8_t __wrap_idsw_get_die_id(void)
{
    return mock_type(uint8_t);
}

uint32_t __wrap_mts_get_this_core_id(void)
{
    return CPU_MCP;
}

bool __wrap_transfer_rly_is_primary_node(void)
{
    return mock_type(bool);
}

uint32_t __wrap_atu_svc_accel_atu_addr(uint8_t accel_id)
{
    FPFW_UNUSED(accel_id);
    return 0;
}

void __wrap_mts_client_send_trp_response(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);

    // Do Nothing - just an empty mock
}

void __wrap_mts_client_send_new_trp_msg(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);

    function_called();
}

void __wrap_mts_client_forward_trp_msg(p_trp_msg_t trp_msg, trp_broadcast_t broadcast_option)
{
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(broadcast_option);

    // Do Nothing - just an empty mock
}

void __wrap_mts_client_send_dcp_notification(mts_client_id_t client_id, dcp_notification_type_t notification)
{
    FPFW_UNUSED(client_id);
    FPFW_UNUSED(notification);
    function_called();
}

void __wrap_SCB_InvalidateDCache_by_Addr(uint32_t* addr, int32_t dsize)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(dsize);

    function_called();
}

void __wrap_SCB_CleanDCache_by_Addr(uint32_t* addr, int32_t dsize)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(dsize);

    function_called();
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

    // Do Nothing - just an empty mock
}

uint64_t __wrap_config_get_hsp_buffers_drop_rpt_thresh(void)
{
    return 100;
}

uint64_t __wrap_config_get_asic_buffers_reused_rpt_thresh(void)
{
    return 100;
}

uint64_t __wrap_config_get_delayed_host_reads_rpt_thresh(void)
{
    return 100;
}

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    longjmp(mock_jump_buf, 1);
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(bool);
}

void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    check_expected(OldState);

    function_called();
}

uint64_t __wrap_utc_sync_client_get_current_time_epoch_ms(void)
{
    return mock_type(uint64_t);
}

uint64_t __wrap_gtimer_get_timestamp_ms(void)
{
    return mock_type(uint64_t);
}

/* Mask applied to event flags returned by the mock. Default 0xFFFFFFFF passes all flags through.
 * Tests can call set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER) to exclude HSP handler. */
static ULONG s_event_flags_mask = 0xFFFFFFFF;

void set_event_flags_mask(ULONG mask)
{
    s_event_flags_mask = mask;
}

/* Local override of the shared ThreadX event_flags_get mock.
 * Applies s_event_flags_mask so that non-HSP tests can exclude
 * ETR_EVENT_FLAG_PROCESS_HSP_BUFFER from the returned flags. */
UINT __wrap__txe_event_flags_get(TX_EVENT_FLAGS_GROUP* group_ptr, ULONG requested_flags, UINT get_option, ULONG* actual_flags_ptr, ULONG wait_option)
{
    check_expected_ptr(group_ptr);
    check_expected(requested_flags);
    check_expected(get_option);
    check_expected_ptr(actual_flags_ptr);
    check_expected(wait_option);

    static int count = 0x0;

    if (count == 0)
    {
        *actual_flags_ptr = requested_flags & s_event_flags_mask;
        count++;
    }
    else
    {
        *actual_flags_ptr = ~requested_flags;
        count = 0x0;
    }

    return mock_type(UINT);
}

} // extern "C"

// Forward declare globals so test_setup_common can reset them
extern FPFW_ET_CORE_BUFFER_HEADER fake_core_buffer;
extern trp_msg_t trp_msg;

static inline void test_setup_common()
{
    memset(&s_test_context, 0, sizeof(s_test_context));
    memset(&s_asic_ddr_memory, 0, sizeof(s_asic_ddr_memory));
    memset(&s_hsp_ddr_memory, 0, sizeof(s_hsp_ddr_memory));

    // Reset cross-test globals/statics
    accel_isolation = false;

    // Reset fake_core_buffer fields that may be modified by previous tests
    fake_core_buffer.StartAsicTimeStamp = 50;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 50;
    fake_core_buffer.EndUTCTimeStamp = 100;
    fake_core_buffer.UsedBytes = 1000;

    // Reset trp_msg addr_offset - may be modified by SCP tests that use a different offset
    trp_msg.payload.intercore_block_notification.addr_offset = (uintptr_t)&fake_core_buffer - SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;

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
}

int test_setup_die0(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    will_return_always(__wrap_transfer_rly_is_primary_node, true);
    will_return_always(__wrap_mts_get_this_die_id, DIE_0);

    test_setup_common();

    return TX_SUCCESS;
}

int test_setup_die1(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    will_return_always(__wrap_transfer_rly_is_primary_node, false);
    will_return_always(__wrap_mts_get_this_die_id, DIE_1);

    test_setup_common();

    return TX_SUCCESS;
}

int test_teardown(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    /* Reset event flags mask so subsequent tests get all flags by default */
    set_event_flags_mask(0xFFFFFFFF);

    return TX_SUCCESS;
}

// TEST PUBLIC ETR FUNCTIONS

TEST_FUNCTION(test_etr_init_null_param, nullptr, nullptr)
{
    if (!bugcheck_mock_return())
    {
        etr_initialize(nullptr, &s_test_config);
    }

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, nullptr);
    }
}

TEST_FUNCTION(test_etr_init_bad_ddr_config, nullptr, nullptr)
{
    etr_service_config_t bad_config = s_test_config;
    bad_config.hsp_ddr_config.base_addr = 0;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.base_addr = 0;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.asic_ddr_config.size_bytes = ASIC_BUFFER_PAYLOAD_SIZE - 1;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.size_bytes = HSP_BUFFER_PAYLOAD_SIZE - 1;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.asic_ddr_config.size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE + 1;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }

    bad_config = s_test_config;
    bad_config.hsp_ddr_config.size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE + 1;

    if (!bugcheck_mock_return())
    {
        etr_initialize(&s_test_context, &bad_config);
    }
}

TEST_FUNCTION(test_etr_init_nominal, test_setup_die0, test_teardown)
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
    .StartAsicTimeStamp = 50,
    .EndAsicTimeStamp = 100,
    .StartUTCTimeStamp = 50,
    .EndUTCTimeStamp = 100,
    .ETVersion = 0,
    // .CoreId = CPU_SDM,
    .ControllerId = 0,
    .BufferId = 0,
    .BufferSize = 1024,
    .UsedBytes = 1000,
    .LostEvents = 0,
};

trp_msg_t trp_msg_host = {.hdr = {

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
                              .trp_msg_id = TRP_MSG_ID_DCP_FORWARD,
                              .payload_size = sizeof(fake_core_buffer),
                          }};

TEST_FUNCTION(test_etr_process_request_host_request_capabilities, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_CAPABILITIES;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to success
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_set_state_stopped, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;

    trp_msg_host.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_STOP;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_request_state_stopped, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);
    assert_int_equal(trp_msg_host.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_STOPPED);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_set_state_started, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_START_STOP;

    trp_msg_host.payload.dcp_msg.payload.start_stop.state = DCP_START_STOP_STATE_START;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Override number of pending buffers to 1 */
    set_num_buffers_pending(1);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_request_state_running, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_GET_STATE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);
    assert_int_equal(trp_msg_host.payload.dcp_msg.payload.get_state.state, DCP_CLIENT_STATE_RUNNING);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_reset, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_RESET;

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

trp_msg_t trp_msg = {
    .hdr =
        {

            .src_node =
                {
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

TEST_FUNCTION(test_etr_process_request_unsupported_id, test_setup_die0, test_teardown)
{
    // Reset the message type to an unsupported ID
    trp_msg.hdr.trp_msg_id = 0xFF;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_E_PARAM
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_invalid_core, test_setup_die0, test_teardown)
{
    // Test with AP Core
    trp_msg.hdr.src_node.core_id = CPU_AP;
    fake_core_buffer.CoreId = CPU_AP;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_E_PARAM
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_available, test_setup_die0, test_teardown)
{
    // Test with SDM Core
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    etr_ddr_buffer_state_t pre_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t pre_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC timestamp conversion mocks (called twice: once for Start, once for End)
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    etr_ddr_buffer_state_t post_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t post_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    assert_int_equal(pre_request_state, post_request_state);
    assert_true(pre_request_size < post_request_size);
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    assert_memory_equal(&fake_core_buffer, asic_buffer_addr, sizeof(fake_core_buffer));

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_maxed_die0, test_setup_die0, test_teardown)
{
    // Run test case with CDED Core.
    trp_msg.hdr.src_node.core_id = CPU_CDED_SDM;
    fake_core_buffer.CoreId = CPU_CDED_SDM;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Fake out the active buffer to be full
    ddr_buffer_info_t* p_old_asic_buffer = s_test_context.p_active_asic_buffer;
    p_old_asic_buffer->buffer.asic.asic_header.UsedBytes = p_old_asic_buffer->buffer.asic.asic_header.BufferSize;

    fake_core_buffer.UsedBytes = sizeof(fake_core_buffer);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC timestamp conversion mocks (called twice: once for Start, once for End)
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    etr_worker_thread_func((ULONG)&s_test_context);

    ddr_buffer_info_t* p_new_asic_buffer = s_test_context.p_active_asic_buffer;

    assert_ptr_not_equal(p_old_asic_buffer, p_new_asic_buffer);

    // validate that the old buffer is now pending and that it's header was updated in memory
    assert_int_equal(p_old_asic_buffer->state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(p_new_asic_buffer->state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_memory_equal(&p_old_asic_buffer->buffer.asic, p_old_asic_buffer->payload_management.base_addr, sizeof(asic_buffer_info_t));

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_maxed_die1, test_setup_die1, test_teardown)
{
    // Run test case with CDED Core.
    trp_msg.hdr.src_node.core_id = CPU_CDED_SDM;
    fake_core_buffer.CoreId = CPU_CDED_SDM;

    // Reset the message type to intercore block notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Fake out the active buffer to be full
    ddr_buffer_info_t* p_old_asic_buffer = s_test_context.p_active_asic_buffer;
    p_old_asic_buffer->buffer.asic.asic_header.UsedBytes = p_old_asic_buffer->buffer.asic.asic_header.BufferSize;

    fake_core_buffer.UsedBytes = sizeof(fake_core_buffer);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC timestamp conversion mocks (called twice: once for Start, once for End)
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Exclude HSP handler - this test covers the copy buffer path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    /* Expect mts_client_send_new_trp_msg from notify_ddr_buffer_available() via etr_complete_asic_buffer (die1) */
    expect_function_call(__wrap_mts_client_send_new_trp_msg);

    etr_worker_thread_func((ULONG)&s_test_context);

    ddr_buffer_info_t* p_new_asic_buffer = s_test_context.p_active_asic_buffer;

    assert_ptr_not_equal(p_old_asic_buffer, p_new_asic_buffer);

    // validate that the old buffer is now pending and that it's header was updated in memory
    assert_int_equal(p_old_asic_buffer->state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(p_new_asic_buffer->state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_memory_equal(&p_old_asic_buffer->buffer.asic, p_old_asic_buffer->payload_management.base_addr, sizeof(asic_buffer_info_t));

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_no_free_asics, test_setup_die0, test_teardown)
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
            s_test_context.ddr_buffers[i].buffer.asic.asic_header.BufferSize;
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
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC timestamp conversion mocks (called twice: once for Start, once for End)
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)200);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)150);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    // Since SCP, expect a Cache Invalidate call
    expect_function_call(__wrap_SCB_InvalidateDCache_by_Addr);
    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    etr_worker_thread_func((ULONG)&s_test_context);

    // validate that the old buffer is now pending and that first pending buffer is now active
    assert_int_equal(s_test_context.ddr_buffers[1].state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(s_test_context.ddr_buffers[0].state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_int_equal(s_test_context.health_stats.asic_buffers_reused, 1);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_complete_die0, test_setup_die0, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_1;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package complete
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of DCP Message status is set to DATA_COLLECTION_RD_DATA_VALID_MORE
    assert_int_equal(trp_msg.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_complete_buffer_pending_die1, test_setup_die1, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package complete
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;

    // Fake out the active buffer to be pending
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    // Set a good address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset =
        s_test_context.p_active_asic_buffer->payload_management.base_addr;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Exclude HSP handler - this test covers the read package complete path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_RD_DATA_NONE
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_RD_DATA_NONE);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_intercore_block_complete, test_setup_die0, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    trp_msg.payload.read_intercore_block_complete.block_id = 0;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read intercore block complete
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_any(__wrap_FPFwETControllerRecycleBuffer, pTraceController);
    expect_any(__wrap_FPFwETControllerRecycleBuffer, bufIndex);
    will_return(__wrap_FPFwETControllerRecycleBuffer, FPFW_STATUS_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_RD_DATA_NONE
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_RD_DATA_NONE);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_intercore_block_complete_invalid_block, test_setup_die1, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    trp_msg.payload.read_intercore_block_complete.block_id = 3;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read intercore block complete
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Exclude HSP handler - this test covers the intercore block complete path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_RD_DATA_NONE
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_RD_DATA_NONE);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_complete_invalid_address_die1, test_setup_die1, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_1;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package complete
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_COMPLETE;

    // Fake out the active buffer to be pending
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    // Set a good address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset = 0xDEADBEEF; // Invalid address

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Exclude HSP handler - this test covers the read package complete path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_RD_DATA_NONE
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_RD_DATA_NONE);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_notification, test_setup_die0, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_1;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to package notification
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_PACKAGE_NOTIFICATION;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    // Set both accelerators to be isolated for branch coverage
    accel_isolation = true;

    /* Exclude HSP handler - this test covers the package notification path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_no_pending_buffer, test_setup_die1, test_teardown)
{
    // Test with MCP Core
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;

    // Fake out the active buffer to be free
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_FREE;

    // Set an invalid address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset = 0xDEADBEEF; // Invalid address

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Override number of pending buffers to 0 */
    set_num_buffers_pending(0);

    /* Exclude HSP handler - this test covers the read package path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_RD_DATA_NONE
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_RD_DATA_NONE);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_buffer_pending, test_setup_die1, test_teardown)
{
    // Test with MCP Core
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE;

    // Fake out the active buffer to be pending
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    // Set a good address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset =
        s_test_context.p_active_asic_buffer->payload_management.base_addr;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Override number of pending buffers to 1 */
    set_num_buffers_pending(1);

    /* Exclude HSP handler - this test covers the read package path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_SUCCESS
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_read_package_response, test_setup_die0, test_teardown)
{
    trp_msg.hdr.src_node.core_id = CPU_MCP;
    trp_msg.hdr.src_node.die_id = DIE_0;
    fake_core_buffer.CoreId = CPU_MCP;

    // Reset the message type to read package response
    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_READ_PACKAGE_RESPONSE;

    // Fake out the active buffer to be pending
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    // Set a good address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset = 0xDEADBEEF; // Invalid address

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_request_invalid, test_setup_die0, test_teardown)
{
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_UTC_SYNC;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_E_DCP_ERROR
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_E_DCP_ERROR);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_read_data_buffer_pending, test_setup_die0, test_teardown)
{
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA;

    // Set a good address for the host read request
    trp_msg_host.payload.intercore_block_notification.addr_offset =
        s_test_context.p_active_asic_buffer->payload_management.base_addr;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of the dcp read data request is set to success, and TRP Message status is set to success
    assert_int_equal(trp_msg_host.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_MORE);
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_read_data_no_buffer_pending_primary_etr, test_setup_die0, test_teardown)
{
    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Fake out the active buffer to be free
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_FREE;

    // Set an invalid address for the host read request
    trp_msg.payload.read_package_complete.phy_addr_offset = 0xDEADBEEF; // Invalid address

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_read_data_complete_valid_address, test_setup_die0, test_teardown)
{
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
    trp_msg_host.hdr.trp_msg_id = TRP_MSG_ID_DCP_FORWARD;
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;

    // Set a good address for the host read request
    trp_msg_host.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset =
        s_test_context.p_active_asic_buffer->payload_management.base_addr - MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    /* Override number of pending buffers to 2. After completing this buffer, there will be only one pending buffer */
    set_num_buffers_pending(2);

    /* Exclude HSP handler - this test covers the read data complete path, not HSP */
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the active buffer is set to free
    assert_int_equal(s_test_context.p_active_asic_buffer->state, ETR_DDR_BUFFER_STATE_FREE);

    // Expect that the status of the dcp read data request is set to DATA_COLLECTION_RD_DATA_VALID_LAST
    // since we only have 1 buffer pending after processing DCP_MSG_ID_READ_DATA_COMPLETE
    assert_int_equal(trp_msg_host.payload.dcp_msg.hdr.msg_status, DATA_COLLECTION_RD_DATA_VALID_LAST);

    // Expect that the status of TRP Message status is set to success
    assert_int_equal(trp_msg_host.hdr.trp_msg_status, TRP_STATUS_SUCCESS);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_process_request_host_read_data_complete_invalid_address, test_setup_die0, test_teardown)
{
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;
    trp_msg_host.payload.dcp_msg.hdr.msg_id = DCP_MSG_ID_READ_DATA_COMPLETE;

    // Set a bad address for the host read request
    trp_msg_host.payload.dcp_msg.payload.read_data_complete.rd_data_addr_offset = 0xDEADBEEF;

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg_host);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the active buffer is still pending since the address was invalid
    assert_int_equal(s_test_context.p_active_asic_buffer->state, ETR_DDR_BUFFER_STATE_PENDING);

    // Run the function again to cover the path where txe_flags_get returns an invalid flag.
    // This functionality is built into the mock here:
    // https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.CortexM7?path=/tests/utilities/mocks/threadx/thread_x_mocks.c&version=GBmain&line=154&lineEnd=154&lineStartColumn=6&lineEndColumn=33&lineStyle=plain&_a=contents
    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_icc_handle_hsp_primary_die, test_setup_die0, test_teardown)
{
    // The HSP ICC interface simply handles a request, sends a response, and sets up another receive
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    static hsp_log_payload_header_t test_payload = {
        .output_header =
            {
                .manifest_size = 10,
                .log_size = 10,
            },
    };
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    etr_set_override_atu_test_address(&test_payload);

    /* Override number of pending buffers to 0 */
    set_num_buffers_pending(0);

    expect_function_call(__wrap_SCB_InvalidateDCache_by_Addr);
    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    /* Expect event flag set from set_etr_thread_event_flags() to process HSP buffer */
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    etr_icc_handle_hsp((void*)&s_test_context, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
}

TEST_FUNCTION(test_etr_icc_handle_hsp_primary_die_buffers_pending, test_setup_die0, test_teardown)
{
    // The HSP ICC interface simply handles a request, sends a response, and sets up another receive
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    static hsp_log_payload_header_t test_payload_pending = {
        .output_header =
            {
                .manifest_size = 10,
                .log_size = 10,
            },
    };
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    etr_set_override_atu_test_address(&test_payload_pending);

    /* Override number of pending buffers to a non-zero value */
    set_num_buffers_pending(3);

    expect_function_call(__wrap_SCB_InvalidateDCache_by_Addr);
    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    /* Expect event flag set from set_etr_thread_event_flags() to process HSP buffer */
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    etr_icc_handle_hsp((void*)&s_test_context, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
}

TEST_FUNCTION(test_etr_icc_handle_hsp_secondary_die, test_setup_die1, test_teardown)
{
    // The HSP ICC interface simply handles a request, sends a response, and sets up another receive
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    expect_function_call(__wrap_SCB_InvalidateDCache_by_Addr);
    expect_function_call(__wrap_SCB_CleanDCache_by_Addr);

    static hsp_log_payload_header_t test_payload = {
        .output_header =
            {
                .manifest_size = 10,
                .log_size = 10,
            },
    };
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    etr_set_override_atu_test_address(&test_payload);

    /* Expect event flag set from set_etr_thread_event_flags() to process HSP buffer */
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    etr_icc_handle_hsp((void*)&s_test_context, 0, FPFW_ICC_BASE_STATUS_SUCCESS);
}

// TEST BUFFER METADATA TIMESTAMP VALIDATION

TEST_FUNCTION(test_etr_copy_buffer_zero_asic_start_timestamp, test_setup_die0, test_teardown)
{
    // Zero StartAsicTimeStamp should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 0;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag and DCP notification flag - only process MTS messages
    set_event_flags_mask(ETR_EVENT_FLAG_NEW_MTS_MSG);

    // Set Expectations for successful tx_event_flags_get
    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    // Set up expectations for a successful tx_queue_receive
    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // Set up expectations for a successful tx_block_release
    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Expect that the status of TRP Message status is set to TRP_STATUS_E_PARAM
    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_zero_utc_timestamp, test_setup_die0, test_teardown)
{
    // Zero StartUTCTimeStamp should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartUTCTimeStamp = 0;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_end_before_start_asic_timestamp, test_setup_die0, test_teardown)
{
    // EndAsicTimeStamp < StartAsicTimeStamp should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 200;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 200;
    fake_core_buffer.EndUTCTimeStamp = 300;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_end_before_start_utc_timestamp, test_setup_die0, test_teardown)
{
    // EndUTCTimeStamp < StartUTCTimeStamp should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 50;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 200;
    fake_core_buffer.EndUTCTimeStamp = 100;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_zero_used_bytes, test_setup_die0, test_teardown)
{
    // UsedBytes == 0 should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.UsedBytes = 0;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_used_bytes_exceeds_buffer_size, test_setup_die0, test_teardown)
{
    // UsedBytes > BufferSize should fail validation
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.UsedBytes = fake_core_buffer.BufferSize + 1;

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    assert_int_equal(trp_msg.hdr.trp_msg_status, TRP_STATUS_E_PARAM);

    etr_worker_thread_func((ULONG)&s_test_context);
}

// TEST UTC TIMESTAMP CONVERSION

TEST_FUNCTION(test_etr_copy_buffer_utc_conversion_when_utc_equals_asic, test_setup_die0, test_teardown)
{
    // When StartUTCTimeStamp == StartAsicTimeStamp, UTC conversion should be triggered
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 50;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 50; // Same as StartAsicTimeStamp - triggers conversion
    fake_core_buffer.EndUTCTimeStamp = 100;  // Same as EndAsicTimeStamp - triggers conversion

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC conversion mocks - called twice: once for Start, once for End
    // current_utc_time = 1000, current_system_time_ms = 200
    // Start: utc_converted = 1000 - (200 - 50) = 850
    // End:   utc_converted = 1000 - (200 - 100) = 900
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)1000);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)200);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)1000);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)200);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Verify the timestamps were converted - check the buffer in DDR memory
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    PFPFW_ET_CORE_BUFFER_HEADER p_copied_header = (PFPFW_ET_CORE_BUFFER_HEADER)asic_buffer_addr;
    assert_int_equal(p_copied_header->StartUTCTimeStamp, 850);
    assert_int_equal(p_copied_header->EndUTCTimeStamp, 900);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_utc_conversion_skipped_when_utc_zero, test_setup_die0, test_teardown)
{
    // When UTC sync returns 0, conversion should be skipped (timestamp unchanged)
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 50;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 50; // Same as ASIC - triggers conversion attempt
    fake_core_buffer.EndUTCTimeStamp = 100;  // Same as ASIC - triggers conversion attempt

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // UTC sync returns 0 (not available) - conversion should be skipped
    // Note: convert_system_timestamp_to_utc calls BOTH utc_sync and gtimer before checking utc == 0
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)0);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)100);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)0);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)100);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Timestamp should remain unchanged since UTC was not available
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    PFPFW_ET_CORE_BUFFER_HEADER p_copied_header = (PFPFW_ET_CORE_BUFFER_HEADER)asic_buffer_addr;
    assert_int_equal(p_copied_header->StartUTCTimeStamp, 50);
    assert_int_equal(p_copied_header->EndUTCTimeStamp, 100);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_utc_conversion_future_timestamp, test_setup_die0, test_teardown)
{
    // When system timestamp < buffer timestamp (time from the future), conversion should be skipped
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 500;
    fake_core_buffer.EndAsicTimeStamp = 600;
    fake_core_buffer.StartUTCTimeStamp = 500; // Same as ASIC - triggers conversion attempt
    fake_core_buffer.EndUTCTimeStamp = 600;   // Same as ASIC - triggers conversion attempt

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // current_system_time_ms (100) < buffer timestamp (500) - future timestamp
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)1000);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)100);
    will_return(__wrap_utc_sync_client_get_current_time_epoch_ms, (uint64_t)1000);
    will_return(__wrap_gtimer_get_timestamp_ms, (uint64_t)100);

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Timestamp should remain unchanged since conversion was aborted (future timestamp)
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    PFPFW_ET_CORE_BUFFER_HEADER p_copied_header = (PFPFW_ET_CORE_BUFFER_HEADER)asic_buffer_addr;
    assert_int_equal(p_copied_header->StartUTCTimeStamp, 500);
    assert_int_equal(p_copied_header->EndUTCTimeStamp, 600);

    etr_worker_thread_func((ULONG)&s_test_context);
}

TEST_FUNCTION(test_etr_copy_buffer_no_utc_conversion_when_utc_differs_from_asic, test_setup_die0, test_teardown)
{
    // When StartUTCTimeStamp != StartAsicTimeStamp, no conversion should occur
    trp_msg.hdr.src_node.core_id = CPU_SDM;
    fake_core_buffer.CoreId = CPU_SDM;
    fake_core_buffer.StartAsicTimeStamp = 50;
    fake_core_buffer.EndAsicTimeStamp = 100;
    fake_core_buffer.StartUTCTimeStamp = 5000; // Different from ASIC - no conversion
    fake_core_buffer.EndUTCTimeStamp = 10000;  // Different from ASIC - no conversion

    trp_msg.hdr.trp_msg_id = TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION;

    // Mask off HSP buffer flag to prevent notify_ddr_buffer_available from calling tx_event_flags_set
    set_event_flags_mask(~ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);

    expect_any_always(__wrap__txe_event_flags_get, group_ptr);
    expect_any_always(__wrap__txe_event_flags_get, requested_flags);
    expect_any_always(__wrap__txe_event_flags_get, get_option);
    expect_any_always(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_any_always(__wrap__txe_event_flags_get, wait_option);
    will_return_always(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_receive, queue_ptr);
    expect_any_always(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    will_return(set_tx_queue_receive_value, &trp_msg);
    set_txe_queue_receive_callback_func(set_tx_queue_receive_value);

    // No UTC conversion mocks needed - conversion should not be triggered

    expect_any_always(__wrap__txe_block_release, block_ptr);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    expect_function_call(__wrap_mts_client_send_dcp_notification);

    etr_worker_thread_func((ULONG)&s_test_context);

    // Timestamps should remain as set (no conversion occurred)
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    PFPFW_ET_CORE_BUFFER_HEADER p_copied_header = (PFPFW_ET_CORE_BUFFER_HEADER)asic_buffer_addr;
    assert_int_equal(p_copied_header->StartUTCTimeStamp, 5000);
    assert_int_equal(p_copied_header->EndUTCTimeStamp, 10000);

    etr_worker_thread_func((ULONG)&s_test_context);
}

// TEST SET_ETR_THREAD_EVENT_FLAGS

TEST_FUNCTION(test_set_etr_thread_event_flags, test_setup_die0, test_teardown)
{
    // Verify set_etr_thread_event_flags sets the correct flags via tx_event_flags_set
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    set_etr_thread_event_flags(ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION);
}

TEST_FUNCTION(test_set_etr_thread_event_flags_hsp, test_setup_die0, test_teardown)
{
    // Verify set_etr_thread_event_flags works with HSP buffer flag
    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    set_etr_thread_event_flags(ETR_EVENT_FLAG_PROCESS_HSP_BUFFER);
}