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
#include <event_trace_relay.h>
#include <event_trace_relay_i.h>
#include <in_band_telemetry_ddr.h>
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

uint8_t s_asic_ddr_memory[ASIC_BUFFER_PAYLOAD_SIZE * TEST_ASIC_COUNT] = {0};
uint8_t s_hsp_ddr_memory[HSP_BUFFER_PAYLOAD_SIZE * TEST_HSP_COUNT] = {0};
uint8_t s_test_stack[1024] = {0};

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
};

/*------------- Functions ----------------*/

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

UINT __wrap__txe_queue_front_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

} // extern "C"

int test_setup(void** ppContext)
{
    FPFW_UNUSED(ppContext);

    memset(&s_test_context, 0, sizeof(s_test_context));
    memset(&s_asic_ddr_memory, 0, sizeof(s_asic_ddr_memory));
    memset(&s_hsp_ddr_memory, 0, sizeof(s_hsp_ddr_memory));

    // Setup the threadx expectations
    expect_value(__wrap__txe_block_pool_create, pool_ptr, &s_test_context.request_queue.pool);
    expect_value(__wrap__txe_block_pool_create, name_ptr, ETR_WORK_POOL_NAME);
    expect_value(__wrap__txe_block_pool_create, block_size, sizeof(etr_service_request_t));
    expect_value(__wrap__txe_block_pool_create, pool_start, &s_test_context.request_queue.pool_memory[0]);
    expect_value(__wrap__txe_block_pool_create, pool_size, sizeof(s_test_context.request_queue.pool_memory));
    expect_any(__wrap__txe_block_pool_create, pool_control_block_size);
    will_return(__wrap__txe_block_pool_create, TX_SUCCESS);

    expect_value(__wrap__txe_queue_create, queue_ptr, &s_test_context.request_queue.queue);
    expect_value(__wrap__txe_queue_create, name_ptr, ETR_WORK_QUEUE_NAME);
    expect_any(__wrap__txe_queue_create, message_size);
    expect_value(__wrap__txe_queue_create, queue_start, &s_test_context.request_queue.queue_memory[0]);
    expect_value(__wrap__txe_queue_create, queue_size, sizeof(s_test_context.request_queue.queue_memory));
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

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_available, test_setup, test_teardown)
{

    etr_ddr_buffer_state_t pre_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t pre_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    uint8_t fake_core_buffer[1024];
    memset(&fake_core_buffer[0], 0xAB, sizeof(fake_core_buffer));
    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER,
        .request = {.copy_buffer =
                        {
                            .core_id = 0,
                            .buffer_addr = (uintptr_t)&fake_core_buffer[0],
                            .buffer_header =
                                {
                                    .UsedBytes = sizeof(fake_core_buffer),
                                },
                        }},
    };

    // Setup the expectations for the block pool
    expect_value(__wrap__txe_block_release, block_ptr, &request);
    will_return(__wrap__txe_block_release, TX_SUCCESS);

    etr_process_request(&s_test_context, &request);

    etr_ddr_buffer_state_t post_request_state = s_test_context.p_active_asic_buffer->state;
    uint64_t post_request_size = s_test_context.p_active_asic_buffer->payload_management.size_bytes;

    assert_int_equal(pre_request_state, post_request_state);
    assert_true(pre_request_size < post_request_size);
    void* asic_buffer_addr =
        (void*)(s_test_context.p_active_asic_buffer->payload_management.base_addr + sizeof(asic_buffer_info_t));
    assert_memory_equal(&fake_core_buffer[0], asic_buffer_addr, sizeof(fake_core_buffer));
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_space_maxed, test_setup, test_teardown)
{

    // Fake out the active buffer to be full
    ddr_buffer_info_t* p_old_asic_buffer = s_test_context.p_active_asic_buffer;
    p_old_asic_buffer->buffer.asic.asic_header.UsedBytes = ASIC_BUFFER_PAYLOAD_SIZE - sizeof(asic_buffer_info_t);

    uint8_t fake_core_buffer[1024];
    memset(&fake_core_buffer[0], 0xAB, sizeof(fake_core_buffer));

    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER,
        .request = {.copy_buffer =
                        {
                            .core_id = 0,
                            .buffer_addr = (uintptr_t)&fake_core_buffer[0],
                            .buffer_header =
                                {
                                    .UsedBytes = sizeof(fake_core_buffer),
                                },
                        }},
    };

    // Setup the expectations for the queue
    expect_value(__wrap__txe_queue_front_send, queue_ptr, &s_test_context.request_queue.queue);
    expect_value(__wrap__txe_queue_front_send, source_ptr, &request);
    expect_value(__wrap__txe_queue_front_send, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_front_send, TX_SUCCESS);

    etr_process_request(&s_test_context, &request);

    ddr_buffer_info_t* p_new_asic_buffer = s_test_context.p_active_asic_buffer;

    assert_ptr_not_equal(p_old_asic_buffer, p_new_asic_buffer);

    // validate that the old buffer is now pending and that it's header was updated in memory
    assert_int_equal(p_old_asic_buffer->state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(p_new_asic_buffer->state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_memory_equal(&p_old_asic_buffer->buffer.asic, p_old_asic_buffer->payload_management.base_addr, sizeof(asic_buffer_info_t));
}

TEST_FUNCTION(test_etr_process_request_copy_buffer_no_free_asics, test_setup, test_teardown)
{
    assert_int_equal(s_test_context.health_stats.asic_buffers_reused, 0);

    // Fake out the active buffer to be full
    for (uint32_t i = 0; i < ASIC_BUFFER_DDR_CAPACITY_MAX; i++)
    {
        s_test_context.ddr_buffers[i].state = ETR_DDR_BUFFER_STATE_PENDING;
        s_test_context.ddr_buffers[i].buffer.asic.asic_header.UsedBytes =
            ASIC_BUFFER_PAYLOAD_SIZE - sizeof(asic_buffer_info_t);
    }

    // Setup the active buffer to be active
    s_test_context.p_active_asic_buffer = &s_test_context.ddr_buffers[1];
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;

    // Send a request to copy a buffer, which won't fit, causing a new asic buffer to be used
    // all of which are pending. Forcing a pending buffer to be re-used
    uint8_t fake_core_buffer[1024];
    memset(&fake_core_buffer[0], 0xAB, sizeof(fake_core_buffer));

    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_COPY_BUFFER,
        .request = {.copy_buffer =
                        {
                            .core_id = 0,
                            .buffer_addr = (uintptr_t)&fake_core_buffer[0],
                            .buffer_header =
                                {
                                    .UsedBytes = sizeof(fake_core_buffer),
                                },
                        }},
    };

    // Setup the expectations for the queue
    expect_value(__wrap__txe_queue_front_send, queue_ptr, &s_test_context.request_queue.queue);
    expect_value(__wrap__txe_queue_front_send, source_ptr, &request);
    expect_value(__wrap__txe_queue_front_send, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_front_send, TX_SUCCESS);

    etr_process_request(&s_test_context, &request);

    // validate that the old buffer is now pending and that first pending buffer is now active
    assert_int_equal(s_test_context.ddr_buffers[1].state, ETR_DDR_BUFFER_STATE_PENDING);
    assert_int_equal(s_test_context.ddr_buffers[0].state, ETR_DDR_BUFFER_STATE_ACTIVE);
    assert_int_equal(s_test_context.ddr_buffers[0].payload_management.size_bytes, sizeof(asic_buffer_info_t));
    assert_int_equal(s_test_context.ddr_buffers[0].buffer.asic.asic_header.UsedBytes, sizeof(FPFW_ET_ASIC_BUFFER_HEADER));

    assert_int_equal(s_test_context.health_stats.asic_buffers_reused, 1);
}

TEST_FUNCTION(test_etr_process_request_host_read_bad_addr, test_setup, test_teardown)
{
    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_HOST_READ,
        .request = {.host_read =
                        {
                            .die_id = 0,
                            .payload_management =
                                {
                                    .base_addr = 0xDEADBEEF,
                                },
                        }},
    };

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etr_process_request(&s_test_context, &request);
    }
}

TEST_FUNCTION(test_etr_process_request_host_read_good_addr, test_setup, test_teardown)
{
    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_PENDING;

    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_HOST_READ,
        .request = {.host_read =
                        {
                            .die_id = 0,
                            .payload_management =
                                {
                                    .base_addr = s_test_context.p_active_asic_buffer->payload_management.base_addr,
                                },
                        }},
    };

    etr_process_request(&s_test_context, &request);

    assert_int_equal(s_test_context.p_active_asic_buffer->state, ETR_DDR_BUFFER_STATE_FREE);
}

TEST_FUNCTION(test_etr_process_request_host_read_good_addr_bad_state, test_setup, test_teardown)
{
    assert_int_equal(s_test_context.health_stats.delayed_host_reads, 0);

    s_test_context.p_active_asic_buffer->state = ETR_DDR_BUFFER_STATE_ACTIVE;

    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_HOST_READ,
        .request = {.host_read =
                        {
                            .die_id = 0,
                            .payload_management =
                                {
                                    .base_addr = s_test_context.p_active_asic_buffer->payload_management.base_addr,
                                },
                        }},
    };

    etr_process_request(&s_test_context, &request);

    assert_int_equal(s_test_context.health_stats.delayed_host_reads, 1);
}

TEST_FUNCTION(test_etr_process_request_bad_type, test_setup, test_teardown)
{
    etr_service_request_t request = {
        .type = ETR_SERVICE_REQUEST_TYPE_INVALID,
    };

    expect_value(FPFwErrorRaise, error, (uint32_t)FPFW_ET_E_INVALIDARG);
    if (!set_error_handler_return())
    {
        etr_process_request(&s_test_context, &request);
    }
}
