//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops.c
 * Implements the power service thread and queue for handling power state transitions
 */

/*------------- Includes -----------------*/
#include "power_i.h"
#include "power_loops_i.h"
#include "power_stub_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PWR_THREAD_PRIORITY          (10)
#define PWR_THREAD_PREEMPT_THRESHOLD (10)

#define PWR_STACK_SIZE    ((TX_MINIMUM_STACK) + ((1) * (FPFW_KB)))
#define PWR_QUEUE_ENTRIES (15)

#define PWR_QUEUE_NAME  "Pwr Work Queue"
#define PWR_THREAD_NAME "Pwr Worker"

/*------------- Typedefs -----------------*/
typedef struct
{
    uint8_t power_loops_stack[PWR_STACK_SIZE];
    power_loops_queue_entry_t power_loops_queue[PWR_QUEUE_ENTRIES];
    TX_THREAD work_thread;
    TX_QUEUE work_queue;
    uint32_t idle_tracker;
} power_loops_thread_context_t;

/*-------- Function Prototypes -----------*/
static void power_loops_internal_change_state(power_loop_context_t* p_context, int state);
static void power_loops_internal_handle_event(power_loop_context_t* p_context, int event, const void* event_data);

/*-- Declarations (Statics and globals) --*/
static power_loops_thread_context_t s_power_loops_thread_ctx = {0};
/*------------- Functions ----------------*/

// function called on state changes to help keep track of whether all loops have gone idle
static void update_idle(int id, int state)
{
    if (state == POWER_LOOP_IDLE_STATE_ID)
    {
        s_power_loops_thread_ctx.idle_tracker &= ~(1 << id);
    }
    else
    {
        s_power_loops_thread_ctx.idle_tracker |= (1 << id);
    }
}

static bool all_loops_idle()
{
    return (s_power_loops_thread_ctx.idle_tracker == 0);
}

static void power_loops_worker_thread_function(ULONG service_ctx)
{
    FPFW_UNUSED(service_ctx);
    power_loops_queue_entry_t message;

    POWER_LOG_INFO("Worker thread begin");

    while (1)
    {
        UINT status = tx_queue_receive(&s_power_loops_thread_ctx.work_queue, &message, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

        switch (message.type)
        {
        case LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE: {
            // track overall idle state of all loops
            update_idle(message.data.state_change.id, message.data.state_change.state);
            // adjust state of this loop
            power_loops_internal_change_state(message.data.state_change.p_context, message.data.state_change.state);
        }
        break;
        case LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL: {
            power_loops_internal_handle_event(message.data.state_signal.p_context,
                                              message.data.state_signal.event,
                                              message.data.state_signal.event_data);
        }
        break;
        case LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE: {
            if (all_loops_idle())
            {
                FPFW_RUNTIME_ASSERT(message.data.exec_in_idle.p_handler != NULL);
                message.data.exec_in_idle.p_handler(message.data.exec_in_idle.p_request,
                                                    message.data.exec_in_idle.p_context);
            }
            else
            {
                // requeue the request if loops aren't idle
                power_loops_exec_in_idle(message.data.exec_in_idle.p_handler,
                                         message.data.exec_in_idle.p_request,
                                         message.data.exec_in_idle.p_context);
            }
        }
        }
    }
}

void power_loops_init()
{
    POWER_LOG_INFO("Worker thread init");

    // create thread, queue, and event flags for handling requests
    int status = tx_queue_create(&s_power_loops_thread_ctx.work_queue, /* TX_QUEUE *queue_ptr */
                                 PWR_QUEUE_NAME,                       /* CHAR *name_ptr */
                                 sizeof(power_loops_queue_entry_t) / sizeof(uint32_t), /* UINT message_size in 32b words */
                                 (void*)s_power_loops_thread_ctx.power_loops_queue,   /* VOID *queue_start */
                                 sizeof(s_power_loops_thread_ctx.power_loops_queue)); /* ULONG queue_size */
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

    status = tx_thread_create(&s_power_loops_thread_ctx.work_thread, /* TX_THREAD *thread_ptr */
                              PWR_THREAD_NAME,                       /* CHAR *name_ptr */
                              power_loops_worker_thread_function,    /* VOID (*entry_function)(ULONG) */
                              (ULONG)NULL,                           /* ULONG entry_input */
                              (void*)s_power_loops_thread_ctx.power_loops_stack,  /* VOID *stack_start */
                              sizeof(s_power_loops_thread_ctx.power_loops_stack), /* ULONG stack_size */
                              PWR_THREAD_PRIORITY,                                /* UINT priority */
                              PWR_THREAD_PREEMPT_THRESHOLD,                       /* UINT preempt_threshold */
                              TX_NO_TIME_SLICE,                                   /* ULONG time_slice */
                              TX_AUTO_START);                                     /* UINT auto_start */
    FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);
}

static void power_loops_internal_handle_event(power_loop_context_t* p_context, int event, const void* event_data)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    power_loop_state_t* loop_state_detail = &p_context->status;
    const power_state_handler_t* handler_table = p_context->handlers;

    // Ensure no coding errors introduced
    FPFW_RUNTIME_ASSERT(handler_table != NULL);
    FPFW_RUNTIME_ASSERT(loop_state_detail != NULL);
    FPFW_RUNTIME_ASSERT(handler_table[loop_state_detail->current_state] != NULL);

    // clear interval in flight flag
    if (event == POWER_LOOP_STATE_SIGNAL_INTERVAL)
    {
        p_context->status.interval_in_flight = false;
    }

    loop_state_detail->last_event = event;
    handler_table[loop_state_detail->current_state](event, event_data);
}

/* below is a common function for handling power loop state changes; updates
 * current state, runs state entry for the new state, and tracks state residency
 */
static void power_loops_internal_change_state(power_loop_context_t* p_context, int state)
{

    FPFW_RUNTIME_ASSERT(p_context != NULL);

    power_loop_state_t* loop_state_detail = &p_context->status;
    power_loop_residency_t* residency = p_context->residency;
    const power_state_handler_t* handler_table = p_context->handlers;

    // Ensure no coding errors introduced
    FPFW_RUNTIME_ASSERT(handler_table != NULL);
    FPFW_RUNTIME_ASSERT(loop_state_detail != NULL);
    FPFW_RUNTIME_ASSERT(residency != NULL);
    FPFW_RUNTIME_ASSERT(state >= POWER_LOOP_IDLE_STATE_ID);
    FPFW_RUNTIME_ASSERT((unsigned)state < p_context->state_count);
    FPFW_RUNTIME_ASSERT(handler_table[state] != NULL);

    const uint64_t counter = power_timer_get_counter();

    // track state residency
    residency[loop_state_detail->current_state].count++;
    residency[loop_state_detail->current_state].ticks +=
        (counter - residency[loop_state_detail->current_state].last_entry);
    residency[state].last_entry = counter;

    POWER_LOG_TRACE("State %d exits %" PRIu32 "%08" PRIu32 " ticks %" PRIu32 "%08" PRIu32,
                    loop_state_detail->current_state,
                    (uint32_t)(residency[loop_state_detail->current_state].count >> 32),
                    (uint32_t)residency[loop_state_detail->current_state].count,
                    (uint32_t)(residency[loop_state_detail->current_state].ticks >> 32),
                    (uint32_t)residency[loop_state_detail->current_state].ticks);

    // reset state retries
    residency[loop_state_detail->current_state].max_retries =
        MAX(loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_STATE],
            residency[loop_state_detail->current_state].max_retries);
    residency[loop_state_detail->current_state].retries += loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_STATE];
    loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_STATE] = 0;

    // reset interval retries
    if (state == POWER_LOOP_IDLE_STATE_ID)
    {
        residency[state].max_retries =
            MAX(loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_INTERVAL], residency[state].max_retries);
        residency[state].retries += loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_INTERVAL];
        loop_state_detail->retries[POWER_LOOP_RETRY_TYPE_INTERVAL] = 0;
    }

    // keep track of previous state, update current state to new state
    loop_state_detail->last_state = loop_state_detail->current_state;
    loop_state_detail->current_state = state;

    // execute the state entry for the new state
    handler_table[state](POWER_LOOP_STATE_SIGNAL_ENTRY, NULL);
}

void power_loops_init_loop(power_loop_context_t* p_context)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    FPFW_RUNTIME_ASSERT(p_context->residency != NULL);

    const uint64_t counter_now = power_timer_get_counter();

    // set initial state; state 0 reserved for table of totals
    p_context->status.current_state = POWER_LOOP_IDLE_STATE_ID;
    p_context->residency[POWER_LOOP_IDLE_STATE_ID].last_entry = counter_now;
}

void power_loops_handle_event(power_loop_context_t* p_context, int event, const void* event_data)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    if (event == POWER_LOOP_STATE_SIGNAL_INTERVAL)
    {
        // expect interrupt context, so no need for mutex, etc
        // allow only one interval signal at a time per loop
        if (p_context->status.interval_in_flight)
        {
            return;
        }
        p_context->status.interval_in_flight = true;
    }

    power_loops_queue_entry_t message;
    message.type = LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL;
    message.data.state_signal.p_context = p_context;
    message.data.state_signal.event = event;
    message.data.state_signal.event_data = event_data;

    int status = tx_queue_send(&s_power_loops_thread_ctx.work_queue, &message, TX_NO_WAIT);
    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
}

void power_loops_change_state(power_loop_context_t* p_context, int state)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    // queue state change
    power_loops_queue_entry_t message;
    message.type = LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE;
    message.data.state_change.id = p_context->id;
    message.data.state_change.p_context = p_context;
    message.data.state_change.state = state;

    int status = tx_queue_send(&s_power_loops_thread_ctx.work_queue, &message, TX_NO_WAIT);
    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
}

void power_loops_exec_in_idle(power_exec_in_idle_handler_t p_handler, PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    power_loops_queue_entry_t message;
    message.type = LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE;
    message.data.exec_in_idle.p_handler = p_handler;
    message.data.exec_in_idle.p_request = p_request;
    message.data.exec_in_idle.p_context = p_context;

    int status = tx_queue_send(&s_power_loops_thread_ctx.work_queue, &message, TX_NO_WAIT);
    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
}

bool power_loops_retry_fail(power_loop_context_t* p_context, power_loop_retries_t type)
{
    // Ensure no coding errors introduced
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    FPFW_RUNTIME_ASSERT(type < POWER_LOOP_RETRY_TYPE_COUNT);

    power_loop_state_t* loop_state_detail = &p_context->status;

    // Ensure no coding errors introduced
    FPFW_RUNTIME_ASSERT(loop_state_detail != NULL);

    // basically, we'll return true if the type of retry has exceeded the limit
    // otherwise, we just update the retry count and return false
    if (loop_state_detail->retries[type] < POWER_LOOP_RETRY_COUNT)
    {
        ++loop_state_detail->retries[type];
        return false;
    }
    return true;
}
