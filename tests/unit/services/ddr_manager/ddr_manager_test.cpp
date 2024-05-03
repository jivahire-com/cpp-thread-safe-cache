//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_test.cpp
 * DDR Manager tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <ddr_manager.h>   // for ddr_manager_init, ddr_service_context_t
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <error_handler.h> // for set_error_handler_return
#include <tx_api.h>        // for TX_SUCCESS, ULONG, TX_NOT_DONE, TX_NO_MEMORY
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x) (void)(x)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//

//
// Tests
//
TEST_FUNCTION(ddr_manager_init_fail, NULL, NULL)
{
    ddr_service_context_t ddr_service_context = {};
    ddr_service_config_t ddr_service_config = {};

    // tx queue fails
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_NO_MEMORY);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NO_MEMORY);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config);
    }

    // tx thread create fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any_always(__wrap__txe_thread_create, thread_ptr);
    expect_any_always(__wrap__txe_thread_create, name_ptr);
    expect_any_always(__wrap__txe_thread_create, entry_function);
    expect_any_always(__wrap__txe_thread_create, entry_input);
    expect_any_always(__wrap__txe_thread_create, stack_start);
    expect_any_always(__wrap__txe_thread_create, stack_size);
    expect_any_always(__wrap__txe_thread_create, priority);
    expect_any_always(__wrap__txe_thread_create, preempt_threshold);
    expect_any_always(__wrap__txe_thread_create, time_slice);
    expect_any_always(__wrap__txe_thread_create, auto_start);
    expect_any_always(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_NOT_DONE);
    expect_value(FPFwErrorRaise, error, (uint32_t)TX_NOT_DONE);

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config);
    }

    // tx timer create fails
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    will_return(__wrap__txe_timer_create, TX_TIMER_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)(TX_TIMER_ERROR));

    if (!set_error_handler_return())
    {
        ddr_manager_init(&ddr_service_context, &ddr_service_config);
    }
}

#define DDR_TEST_STACK_SIZE             (1024)
#define DDR_TEST_THREAD_PRIORITY        (10)
#define DDR_TEST_TIMER_INITIAL_TICKS    (10)
#define DDR_TEST_TIMER_RESCHEDULE_TICKS (15)
TEST_FUNCTION(ddr_manager_init_check_params, NULL, NULL)
{
    uint8_t ddr_test_stack[DDR_TEST_STACK_SIZE];
    static uint32_t ddr_queue_pool[10];
    static ddr_service_context_t ddr_service_ctx = {};

    ddr_service_config_t config = {.thread_config =
                                       {
                                           .p_stack = ddr_test_stack,
                                           .stack_size = sizeof(ddr_test_stack),
                                           .priority = DDR_TEST_THREAD_PRIORITY,
                                           .time_slice_option = TX_NO_TIME_SLICE,
                                       },
                                   .timer_config =
                                       {
                                           .initial_ticks = DDR_TEST_TIMER_INITIAL_TICKS,
                                           .reschedule_ticks = DDR_TEST_TIMER_RESCHEDULE_TICKS,
                                       },
                                   .queue_config = {
                                       .p_queue = ddr_queue_pool,
                                       .queue_num_words = sizeof(ddr_queue_pool[0]) / sizeof(uint32_t),
                                   }};

    expect_value(__wrap__txe_queue_create, queue_ptr, &ddr_service_ctx.work_queue);
    expect_value(__wrap__txe_queue_create, name_ptr, DDR_WORK_QUEUE_NAME);
    expect_value(__wrap__txe_queue_create, message_size, config.queue_config.queue_num_words);
    expect_value(__wrap__txe_queue_create, queue_start, config.queue_config.p_queue);
    expect_value(__wrap__txe_queue_create, queue_size, sizeof(config.queue_config.p_queue));
    expect_any(__wrap__txe_queue_create, queue_control_block_size);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    expect_value(__wrap__txe_thread_create, thread_ptr, &ddr_service_ctx.work_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, DDR_WORK_THREAD_NAME);
    expect_value(__wrap__txe_thread_create, entry_function, ddr_worker_thread_func);
    expect_value(__wrap__txe_thread_create, entry_input, (ULONG)&ddr_service_ctx);
    expect_value(__wrap__txe_thread_create, stack_start, config.thread_config.p_stack);
    expect_value(__wrap__txe_thread_create, stack_size, config.thread_config.stack_size);
    expect_value(__wrap__txe_thread_create, priority, config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, preempt_threshold, config.thread_config.priority);
    expect_value(__wrap__txe_thread_create, time_slice, config.thread_config.time_slice_option);
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    expect_any(__wrap__txe_thread_create, thread_control_block_size);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    ddr_manager_init(&ddr_service_ctx, &config);
}

TEST_FUNCTION(ddr_timer_cb_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};

    expect_value(__wrap__txe_queue_send, queue_ptr, &ddr_service_ctx.work_queue);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    ddr_timer_cb((ULONG)&ddr_service_ctx);
}

TEST_FUNCTION(ddr_worker_thread_func_success, NULL, NULL)
{
    ddr_service_context_t ddr_service_ctx = {};
    ddr_service_ctx.work_queue.tx_queue_start = (ULONG*)0x1234;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &(ddr_service_ctx.work_queue));
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, (ULONG)TX_WAIT_FOREVER);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_ERROR);

    expect_value(FPFwErrorRaise, error, (uint32_t)(TX_QUEUE_ERROR));

    if (!set_error_handler_return())
    {
        ddr_worker_thread_func((ULONG)&ddr_service_ctx);
    }
}

// Add coverage to functions that currently do nothing
// TODO: Replace this test as these functions begin to get written
TEST_FUNCTION(ddr_worker_thread_func_test_message_types, NULL, NULL)
{
    ddr_create_memory_map();
    ddr_create_bdat();
    ddr_create_smbios_tables();
    ddr_process_i3c_data();
    ddr_poll_dimms();
    ddr_poll_dimms();
    ddr_poll_dimms();
    ddr_poll_dimms();
}
