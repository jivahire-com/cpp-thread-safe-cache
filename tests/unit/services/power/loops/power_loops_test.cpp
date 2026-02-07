//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops_test.cpp
 * Power service loops tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include "DfwkPtrTypes.h"      // for PDFWK_ASYNC_REQUEST_HEADER
#include "power_runconfig_i.h" // for power_runconfig_t

#include <CMockaWrapper.h> // for expect_value, assert_int_equal, expec...
#include <power_loops_i.h> // for power_loops_queue_entry_t, power_loop...
#include <setjmp.h>        // for longjmp, jmp_buf, setjmp
#include <stdint.h>        // for int32_t, uint64_t, uint32_t
#include <string.h>        // for memcpy
#include <tx_api.h>        // for ULONG, TX_SUCCESS, UINT, VOID, TX_NO_...

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_STATES     2
#define EXIT_TEST_VALUE 0x1234

/*------------- Typedefs -----------------*/
typedef VOID (*entry_function_t)(ULONG entry_input);

/*-------- Function Prototypes -----------*/
static void mock_handler0(int event, const void* event_data);
static void mock_handler1(int event, const void* event_data);
static void mock_idle_callback(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context);

void __real_power_loops_init_loop(power_loop_context_t* p_context);
void __real_power_loops_handle_event(power_loop_context_t* context, int event, const void* event_data);
void __real_power_loops_change_state(power_loop_context_t* context, int state);
bool __real_power_loops_retry_fail(power_loop_context_t* context, power_loop_retries_t type);

/*-- Declarations (Statics and globals) --*/
static entry_function_t s_entry_function = NULL;
static power_loops_queue_entry_t s_queue_entry = {};
// Table of state handler functions for mock loop
static const power_state_handler_t mock_handler_table[TEST_STATES] = {
    [0] = mock_handler0,
    [1] = mock_handler1,
};
static power_loop_residency_t mock_handler_residency[TEST_STATES] = {{0}};

static power_loop_context_t s_mock_loop_context = {.state_count = TEST_STATES,
                                                   .handlers = mock_handler_table,
                                                   .residency = mock_handler_residency,
                                                   .id = LOOP_ID_CONTROL,
                                                   .error_state = -1};

static jmp_buf mock_jump_buf;
/*------------- Functions ----------------*/
//
// Mocks
//
// mocks for ThreadX (tx) APIs
int32_t __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                                  CHAR* name_ptr,
                                  VOID (*entry_function)(ULONG entry_input),
                                  ULONG entry_input,
                                  VOID* stack_start,
                                  ULONG stack_size,
                                  UINT priority,
                                  UINT preempt_threshold,
                                  ULONG time_slice,
                                  UINT auto_start)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(entry_function);
    check_expected(entry_input);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    check_expected(preempt_threshold);
    check_expected(time_slice);
    check_expected(auto_start);
    // store the entry function for later use
    s_entry_function = entry_function;
    return mock_type(int32_t);
}

int32_t __wrap__txe_queue_create(TX_QUEUE* queue_ptr, const char* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name_ptr);
    check_expected(message_size);
    check_expected_ptr(queue_start);
    check_expected(queue_size);
    return mock_type(int32_t);
}

int32_t __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);

    memcpy(&s_queue_entry, source_ptr, sizeof(power_loops_queue_entry_t));
    return mock_type(int32_t);
}

int32_t __wrap__txe_queue_front_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);

    memcpy(&s_queue_entry, source_ptr, sizeof(power_loops_queue_entry_t));
    return mock_type(int32_t);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(destination_ptr);
    check_expected(wait_option);

    power_loops_queue_entry_t* provided_entry = mock_type(power_loops_queue_entry_t*);
    if ((unsigned)provided_entry == (unsigned)EXIT_TEST_VALUE)
    {
        longjmp(mock_jump_buf, 1);
    }
    memcpy(destination_ptr, provided_entry, sizeof(power_loops_queue_entry_t));

    return mock_type(UINT);
}

UINT __wrap__txe_queue_info_get(TX_QUEUE* queue_ptr,
                                CHAR** name,
                                ULONG* enqueued,
                                ULONG* available_storage,
                                TX_THREAD** first_suspended,
                                ULONG* suspended_count,
                                TX_QUEUE** next_queue)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name);
    check_expected_ptr(enqueued);
    check_expected_ptr(available_storage);
    check_expected_ptr(first_suspended);
    check_expected_ptr(suspended_count);
    check_expected_ptr(next_queue);
    if (enqueued != NULL)
    {
        ULONG count = mock_type(ULONG);
        printf("enqueued: %lu\n", count);
        *enqueued = count;
    }
    return mock_type(UINT);
}

void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

uint64_t __wrap_power_timer_get_counter()
{
    return mock_type(uint64_t);
}

uint64_t __wrap_power_timer_get_counter_ticks_us(uint32_t time_in_us)
{
    check_expected(time_in_us);
    return mock_type(uint64_t);
}

static void mock_handler0(int event, const void* event_data)
{
    check_expected(event);
    check_expected(event_data);
}

static void mock_handler1(int event, const void* event_data)
{
    check_expected(event);
    check_expected(event_data);
}

static void mock_idle_callback(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    check_expected(p_request);
    check_expected(p_context);
}
// End mocks

// wrap for power_runconfig_get
power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}

} // extern "C"
/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

POWER_TEST(power_loops_init, NULL, NULL)
{
#define THREAD_PRIORITY          (5)
#define THREAD_PREEMPT_THRESHOLD (5)

    // expectations for queue create API
    // work queue
    expect_not_value(__wrap__txe_queue_create, queue_ptr, NULL);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_value(__wrap__txe_queue_create, message_size, sizeof(power_loops_queue_entry_t) / sizeof(uint32_t));
    expect_not_value(__wrap__txe_queue_create, queue_start, NULL);
    expect_not_value(__wrap__txe_queue_create, queue_size, 0);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    // idle queue
    expect_not_value(__wrap__txe_queue_create, queue_ptr, NULL);
    expect_any(__wrap__txe_queue_create, name_ptr);
    expect_value(__wrap__txe_queue_create, message_size, sizeof(power_loops_queue_entry_t) / sizeof(uint32_t));
    expect_not_value(__wrap__txe_queue_create, queue_start, NULL);
    expect_not_value(__wrap__txe_queue_create, queue_size, 0);
    will_return(__wrap__txe_queue_create, TX_SUCCESS);

    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_thread_create, thread_ptr, NULL);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_not_value(__wrap__txe_thread_create, entry_function, NULL);
    expect_value(__wrap__txe_thread_create, entry_input, NULL);
    expect_not_value(__wrap__txe_thread_create, stack_start, NULL);
    expect_not_value(__wrap__txe_thread_create, stack_size, 0);
    expect_value(__wrap__txe_thread_create, priority, THREAD_PRIORITY);
    expect_value(__wrap__txe_thread_create, preempt_threshold, THREAD_PREEMPT_THRESHOLD);
    expect_value(__wrap__txe_thread_create, time_slice, TX_NO_TIME_SLICE);
    expect_value(__wrap__txe_thread_create, auto_start, TX_AUTO_START);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);

    expect_value_count(__wrap_FpFwAssert, expression, true, 3);

    // call the function
    power_loops_init();
}

static void setup_expectations_for_exec_in_idle()
{
    // expectations for queue send API
    power_loops_queue_entry_type_t type = LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE;
    // work queue
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_memory(__wrap__txe_queue_send, source_ptr, &type, sizeof(type)); // type is first field in entry
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // idle queue
    type = LOOP_QUEUE_ENTRY_TYPE_NOP;
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_memory(__wrap__txe_queue_send, source_ptr, &type, sizeof(type)); // type is first field in entry
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);
}

static void check_exec_in_idle(int handler, int request, int context)
{
    // check the queue entry
    assert_int_equal(s_queue_entry.data.exec_in_idle.p_handler, handler);
    assert_int_equal(s_queue_entry.data.exec_in_idle.p_request, request);
    assert_int_equal(s_queue_entry.data.exec_in_idle.p_context, context);
}

// test for  power_loops_exec_in_idle
POWER_TEST(power_loops_exec_in_idle, NULL, NULL)
{
#define HANDLER_PTR (0x1234)
#define REQUEST_PTR (0x5678)
#define CONTEXT_PTR (0x9abc)

    setup_expectations_for_exec_in_idle();

    // call the function
    power_loops_exec_in_idle((power_exec_in_idle_handler_t)HANDLER_PTR,
                             (PDFWK_ASYNC_REQUEST_HEADER)REQUEST_PTR,
                             (void*)CONTEXT_PTR);

    check_exec_in_idle(HANDLER_PTR, REQUEST_PTR, CONTEXT_PTR);
}

// test for  power_loops_handle_event
POWER_TEST(power_loops_handle_interval_event, NULL, NULL)
{
#define EVENT_DATA_PTR (0x5678)

    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;

    // expectations for queue send API
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_send, source_ptr, NULL);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap_FpFwAssert, expression, true);

    // call the function
    __real_power_loops_handle_event(&context, POWER_LOOP_STATE_SIGNAL_INTERVAL, (const void*)EVENT_DATA_PTR);

    // check the queue entry
    assert_int_equal(s_queue_entry.type, LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL);
    assert_int_equal(s_queue_entry.data.state_signal.p_context, &context);
    assert_int_equal(s_queue_entry.data.state_signal.event, POWER_LOOP_STATE_SIGNAL_INTERVAL);
    assert_int_equal(s_queue_entry.data.state_signal.event_data, (const void*)EVENT_DATA_PTR);

    assert_true(context.status.interval_in_flight);
}

// test for  power_loops_handle_event
POWER_TEST(power_loops_handle_interval_event__already_in_flight, NULL, NULL)
{
#define EVENT_DATA_PTR (0x5678)

    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;
    context.status.interval_in_flight = true;

    expect_value(__wrap_FpFwAssert, expression, true);

    // call the function
    __real_power_loops_handle_event(&context, POWER_LOOP_STATE_SIGNAL_INTERVAL, (const void*)EVENT_DATA_PTR);

    assert_true(context.status.interval_in_flight);
}

POWER_TEST(power_loops_handle_other_event__interval_already_in_flight, NULL, NULL)
{
#define EVENT_DATA_PTR (0x5678)

    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;
    context.status.interval_in_flight = true;

    // expectations for queue send API
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_send, source_ptr, NULL);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap_FpFwAssert, expression, true);

    // call the function
    __real_power_loops_handle_event(&context, POWER_LOOP_STATE_SIGNAL_ENTRY, (const void*)EVENT_DATA_PTR);

    // check the queue entry
    assert_int_equal(s_queue_entry.type, LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL);
    assert_int_equal(s_queue_entry.data.state_signal.p_context, &context);
    assert_int_equal(s_queue_entry.data.state_signal.event, POWER_LOOP_STATE_SIGNAL_ENTRY);
    assert_int_equal(s_queue_entry.data.state_signal.event_data, (const void*)EVENT_DATA_PTR);

    assert_true(context.status.interval_in_flight);
}

// test for power_loops_change_state
POWER_TEST(power_loops_change_state, NULL, NULL)
{
#define TEST_STATE 5
    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;

    // expectations for queue send API
    expect_not_value(__wrap__txe_queue_send, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_send, source_ptr, NULL);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    expect_value(__wrap_FpFwAssert, expression, true);

    // call the function
    __real_power_loops_change_state(&context, TEST_STATE);

    // check the queue entry
    assert_int_equal(s_queue_entry.type, LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE);
    assert_int_equal(s_queue_entry.data.state_change.p_context, &context);
    assert_int_equal(s_queue_entry.data.state_change.state, TEST_STATE);
}

// test for power_loops_retry_fail
POWER_TEST(power_loops_retry_fail, NULL, NULL)
{
    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;

    expect_value_count(__wrap_FpFwAssert, expression, true, 3);

    // call the function
    assert_false(__real_power_loops_retry_fail(&context, POWER_LOOP_RETRY_TYPE_INTERVAL));

    // check the retry count, increase by 1
    assert_int_equal(context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL], 1);
}

// test for power_loops_retry_fail
POWER_TEST(power_loops_retry_fail__retries_exhausted, NULL, NULL)
{
    power_loop_context_t context = {0};
    context.id = LOOP_ID_CONTROL;
    context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL] = POWER_LOOP_RETRY_COUNT;

    expect_value_count(__wrap_FpFwAssert, expression, true, 3);

    // call the function
    assert_true(__real_power_loops_retry_fail(&context, POWER_LOOP_RETRY_TYPE_INTERVAL));

    // check the retry count, no change
    assert_int_equal(context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL], POWER_LOOP_RETRY_COUNT);
}

static void setup_entry_function_to_queue_entry(power_loops_queue_entry_t* p_entry, bool idle_entry)
{
    expect_not_value(__wrap__txe_queue_receive, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_receive, destination_ptr, NULL);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);

    // provide a message from the queue
    will_return(__wrap__txe_queue_receive, p_entry);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    if (idle_entry)
    {
        // if we want to test idle case, we need to setup an entry for the idle queue
        // reusing the same entry from above
        expect_not_value(__wrap__txe_queue_receive, queue_ptr, NULL);
        expect_not_value(__wrap__txe_queue_receive, destination_ptr, NULL);
        expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

        // provide a message from the queue
        will_return(__wrap__txe_queue_receive, p_entry);
        will_return(__wrap__txe_queue_receive, TX_SUCCESS);

        // now we expect that entry is going to be requeued to work queue
        // idle queue value
        expect_not_value(__wrap__txe_queue_front_send, queue_ptr, NULL);
        expect_memory(__wrap__txe_queue_front_send, source_ptr, &p_entry->type, sizeof(p_entry->type)); // type is first field in entry
        expect_value(__wrap__txe_queue_front_send, wait_option, TX_NO_WAIT);
        will_return(__wrap__txe_queue_front_send, TX_SUCCESS);
    }

    expect_not_value(__wrap__txe_queue_receive, queue_ptr, NULL);
    expect_not_value(__wrap__txe_queue_receive, destination_ptr, NULL);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_WAIT_FOREVER);

    // provide the exit message to the mock function
    will_return(__wrap__txe_queue_receive, EXIT_TEST_VALUE);

    expect_value(__wrap_FpFwAssert, expression, true);
}

static void call_entry_function()
{
    assert_non_null(s_entry_function);
    if (!setjmp(mock_jump_buf))
    {
        s_entry_function(0);
    }
}

static void setup_expectation_for_queue_get_info(ULONG enqueue_count)
{
    // expectations for queue info get API
    expect_not_value(__wrap__txe_queue_info_get, queue_ptr, NULL);
    expect_value(__wrap__txe_queue_info_get, name, NULL);
    expect_not_value(__wrap__txe_queue_info_get, enqueued, NULL);
    expect_value(__wrap__txe_queue_info_get, available_storage, NULL);
    expect_value(__wrap__txe_queue_info_get, first_suspended, NULL);
    expect_value(__wrap__txe_queue_info_get, suspended_count, NULL);
    expect_value(__wrap__txe_queue_info_get, next_queue, NULL);
    will_return(__wrap__txe_queue_info_get, enqueue_count);
    will_return(__wrap__txe_queue_info_get, TX_SUCCESS);

    expect_value(__wrap_FpFwAssert, expression, true);
}

// test for internal change state
POWER_TEST(power_loops_internal_change_state, NULL, NULL)
{
    // expectations for queue handling

    power_loops_queue_entry_t entry = {.type = LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE,
                                       .data.state_change = {.p_context = &s_mock_loop_context, .state = 0}};

    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, false);

    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(1); // still have item enqueued

    // expectations for power_loops_internal_change_state
    expect_value_count(__wrap_FpFwAssert, expression, true, 8);
    will_return(__wrap_power_timer_get_counter, 0);

    // expect handler call
    expect_value(mock_handler0, event, POWER_LOOP_STATE_SIGNAL_ENTRY);
    expect_value(mock_handler0, event_data, NULL);

    call_entry_function();
}

// test for power_loops_internal_handle_event
POWER_TEST(power_loops_internal_handle_event, NULL, NULL)
{
#define TEST_EVENT_DATA (0x5678)
    // expectations for queue handling

    power_loops_queue_entry_t entry = {.type = LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL,
                                       .data.state_signal = {.p_context = &s_mock_loop_context,
                                                             .event = POWER_LOOP_STATE_SIGNAL_INTERVAL,
                                                             .event_data = (void*)TEST_EVENT_DATA}};

    // pick mock_handler1
    s_mock_loop_context.status.current_state = 1;
    s_mock_loop_context.status.interval_in_flight = true;

    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, false);

    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(1); // still have item enqueued

    // expectations for power_loops_internal_handle_event
    expect_value_count(__wrap_FpFwAssert, expression, true, 4);

    // expect handler call
    expect_value(mock_handler1, event, POWER_LOOP_STATE_SIGNAL_INTERVAL);
    expect_value(mock_handler1, event_data, TEST_EVENT_DATA);

    call_entry_function();

    // we send interval signal, so we should clear the interval_in_flight flag
    assert_false(s_mock_loop_context.status.interval_in_flight);
}

static void setup_state_change(int state, power_loops_queue_entry_t* p_entry)
{
    p_entry->type = LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE;
    p_entry->data.state_change.p_context = &s_mock_loop_context;
    p_entry->data.state_change.state = state;
}

static void setup_exec_in_idle(power_loops_queue_entry_t* p_entry)
{
    p_entry->type = LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE;
    p_entry->data.exec_in_idle.p_handler = (power_exec_in_idle_handler_t)mock_idle_callback;
    p_entry->data.exec_in_idle.p_request = (PDFWK_ASYNC_REQUEST_HEADER)REQUEST_PTR;
    p_entry->data.exec_in_idle.p_context = (void*)CONTEXT_PTR;
}

static void setup_nop(power_loops_queue_entry_t* p_entry)
{
    p_entry->type = LOOP_QUEUE_ENTRY_TYPE_NOP;
}

// test for exec in idle
POWER_TEST(power_loops_internal_exec_in_idle, NULL, NULL)
{
    // -------------------
    // put idle entry into queue / handle
    // -------------------
    power_loops_queue_entry_t entry = {};

    setup_exec_in_idle(&entry);
    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, false);
    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(1); // still have item enqueued
    // checks that the idle callback is valid
    expect_value_count(__wrap_FpFwAssert, expression, true, 1);
    expect_value(mock_idle_callback, p_request, REQUEST_PTR);
    expect_value(mock_idle_callback, p_context, CONTEXT_PTR);

    call_entry_function();
}

// test for exec in idle
POWER_TEST(power_loops_internal_exec_in_idle__setup, NULL, NULL)
{
    // expectations for queue handling

    power_loops_queue_entry_t entry = {};

    // -------------------
    // ensure all are idle
    // -------------------
    setup_state_change(POWER_LOOP_IDLE_STATE_ID, &entry);

    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, true); // true to enable second / idle queue read

    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(0); // no items, enqueued!

    // expectations for power_loops_internal_change_state
    expect_value_count(__wrap_FpFwAssert, expression, true, 8);
    will_return(__wrap_power_timer_get_counter, 0);
    // Control loop returning to IDLE triggers window check
    expect_any(__wrap_power_timer_get_counter_ticks_us, time_in_us);
    will_return(__wrap_power_timer_get_counter_ticks_us, UINT64_MAX); // large value so window doesn't expire

    // expect handler call
    expect_value(mock_handler0, event, POWER_LOOP_STATE_SIGNAL_ENTRY);
    expect_value(mock_handler0, event_data, NULL);

    call_entry_function();
}

POWER_TEST(power_loops_internal_exec_in_idle__not_idle, NULL, NULL)
{
    power_loops_queue_entry_t entry = {};
    // -------------------
    // ensure all not idle
    // -------------------
    setup_state_change(1, &entry);

    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, false);

    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(1); // item enqueued

    // expectations for power_loops_internal_change_state
    expect_value_count(__wrap_FpFwAssert, expression, true, 8);
    will_return(__wrap_power_timer_get_counter, 0);

    // expect handler call
    expect_value(mock_handler1, event, POWER_LOOP_STATE_SIGNAL_ENTRY);
    expect_value(mock_handler1, event_data, NULL);

    call_entry_function();

    // -------------------
    // should not be idle
    // -------------------
    setup_nop(&entry);

    // setup expectations for s_entry_function
    setup_entry_function_to_queue_entry(&entry, false);

    // setup expectation for queue_get_info
    setup_expectation_for_queue_get_info(0); // no items enqueued, but not idle

    call_entry_function();
}

// test for power_loops_init_loop
POWER_TEST(power_loops_init_loop, NULL, NULL)
{
#define TEST_TIMER_VAL 0x1234
    // assert called twice
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);

    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);

    // call the function
    __real_power_loops_init_loop(&s_mock_loop_context);

    // check the queue entry
    assert_int_equal(s_mock_loop_context.status.current_state, POWER_LOOP_IDLE_STATE_ID);
    assert_int_equal(s_mock_loop_context.residency[POWER_LOOP_IDLE_STATE_ID].last_entry, TEST_TIMER_VAL);
}