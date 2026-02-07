//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops.c
 * Implements the power service thread and queue for handling power state transitions
 */

/*------------- Includes -----------------*/
#include "power_events.h"
#include "power_i.h"
#include "power_loops_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PWR_THREAD_PRIORITY          (5)
#define PWR_THREAD_PREEMPT_THRESHOLD (PWR_THREAD_PRIORITY)

#define PWR_STACK_SIZE         ((TX_MINIMUM_STACK) + ((3) * (FPFW_KB)))
#define PWR_QUEUE_ENTRIES      (15)
#define PWR_IDLE_QUEUE_ENTRIES (5)

#define PWR_QUEUE_NAME      "Pwr Work Queue"
#define PWR_IDLE_QUEUE_NAME "Pwr Idle Queue"
#define PWR_THREAD_NAME     "Pwr Worker"

/*------------- Typedefs -----------------*/
typedef struct
{
    uint8_t power_loops_stack[PWR_STACK_SIZE];
    power_loops_queue_entry_t power_loops_queue[PWR_QUEUE_ENTRIES];
    power_loops_queue_entry_t power_idle_queue[PWR_IDLE_QUEUE_ENTRIES];
    TX_THREAD work_thread;
    TX_QUEUE work_queue;
    TX_QUEUE idle_queue;
    uint32_t idle_tracker;
} power_loops_thread_context_t;

/*-------- Function Prototypes -----------*/
static void power_loops_internal_change_state(power_loop_context_t* p_context, int state);
static void power_loops_internal_handle_event(power_loop_context_t* p_context, int event, const void* event_data);

/*-- Declarations (Statics and globals) --*/
static power_loops_thread_context_t s_power_loops_thread_ctx = {0};

// Global timing stats for all loops (emitted every 10 seconds)
static power_all_loops_timing_t s_all_loops_timing = {0};
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

static void process_idle_queue()
{
    power_loops_queue_entry_t message;

    UINT status = tx_queue_receive(&s_power_loops_thread_ctx.idle_queue, &message, TX_NO_WAIT);
    if (status == TX_SUCCESS)
    {
        // if we pulled item from idle queue, send it to front of work queue
        int status = tx_queue_front_send(&s_power_loops_thread_ctx.work_queue, &message, TX_NO_WAIT);
        // only expect this call when queue is empty, so should always succeed
        BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
    }
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
        case LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE:
            FPFW_RUNTIME_ASSERT(message.data.exec_in_idle.p_handler != NULL);
            message.data.exec_in_idle.p_handler(message.data.exec_in_idle.p_request, message.data.exec_in_idle.p_context);
            break;
        default:
            break;
        }

        ULONG enqueued;

        // check if queue is empty, and then process idle queue if all loops are idle
        status = tx_queue_info_get(&s_power_loops_thread_ctx.work_queue, TX_NULL, &enqueued, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
        FPFW_RUNTIME_ASSERT(status == TX_SUCCESS);

        if (enqueued == 0)
        {
            // The work queue is empty; if we're also idle, process the idle queue (one entry at a time)
            if (all_loops_idle())
            {
                process_idle_queue();
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

    status = tx_queue_create(&s_power_loops_thread_ctx.idle_queue,                 /* TX_QUEUE *queue_ptr */
                             PWR_IDLE_QUEUE_NAME,                                  /* CHAR *name_ptr */
                             sizeof(power_loops_queue_entry_t) / sizeof(uint32_t), /* UINT message_size in 32b words */
                             (void*)s_power_loops_thread_ctx.power_idle_queue,   /* VOID *queue_start */
                             sizeof(s_power_loops_thread_ctx.power_idle_queue)); /* ULONG queue_size */
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
    const power_loop_id_t loop_id = p_context->id;
    FPFW_RUNTIME_ASSERT(loop_id < LOOP_ID_COUNT);
    power_loop_timing_stats_t* timing = &s_all_loops_timing.loops[loop_id];
    const int current_state = loop_state_detail->current_state;

    // Initialize window start on first use
    if (s_all_loops_timing.window_start == 0)
    {
        s_all_loops_timing.window_start = counter;
    }

    // START: Leaving IDLE (state 0) to begin new iteration
    if (current_state == POWER_LOOP_IDLE_STATE_ID && state != POWER_LOOP_IDLE_STATE_ID)
    {
        timing->counter_start = counter;
    }

    // STOP: Returning to IDLE from successful completion (not from error state)
    if (state == POWER_LOOP_IDLE_STATE_ID && current_state != POWER_LOOP_IDLE_STATE_ID &&
        current_state != p_context->error_state)
    {
        // Skip sample if counter_start is 0 (can happen if window was reset mid-iteration)
        if (timing->counter_start != 0)
        {
            const uint32_t current_ticks = (uint32_t)(counter - timing->counter_start);

            // First sample for this loop in this window: initialize min and max
            if (timing->sample_count == 0)
            {
                timing->min_ticks = current_ticks;
                timing->max_ticks = current_ticks;
            }
            else
            {
                if (current_ticks > timing->max_ticks)
                {
                    timing->max_ticks = current_ticks;
                }
                if (current_ticks < timing->min_ticks)
                {
                    timing->min_ticks = current_ticks;
                }
            }
            timing->sample_count++;
        }

        // Control loop checks if 10-second window has elapsed and triggers the combined trace
        if (loop_id == LOOP_ID_CONTROL)
        {
            const uint64_t window_ticks = power_timer_get_counter_ticks_us(POWER_LOOP_TIMING_EMIT_INTERVAL_US);

            if ((counter - s_all_loops_timing.window_start) >= window_ticks &&
                s_all_loops_timing.loops[LOOP_ID_CONTROL].sample_count > 0 &&
                s_all_loops_timing.loops[LOOP_ID_VR_TELEM].sample_count > 0 &&
                s_all_loops_timing.loops[LOOP_ID_PVT_TELEM].sample_count > 0)
            {
                // Emit combined trace for all loops (convert ticks to microseconds)
                POWER_ET_ALL_LOOP_METRICS(
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_CONTROL].min_ticks),
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_CONTROL].max_ticks),
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_VR_TELEM].min_ticks),
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_VR_TELEM].max_ticks),
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_PVT_TELEM].min_ticks),
                    (uint32_t)power_timer_get_us_from_counter(s_all_loops_timing.loops[LOOP_ID_PVT_TELEM].max_ticks));

                // Reset all loop stats and start new window
                memset(&s_all_loops_timing, 0, sizeof(s_all_loops_timing));
                s_all_loops_timing.window_start = counter;
            }
        }
    }

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

    POWER_ET_LOG_TRACE_SIGNAL_EVT(event);
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

    POWER_ET_LOG_TRACE_CHANGE_STATE(p_context->id, state);
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

    // idle requests are sent to the idle queue
    int status = tx_queue_send(&s_power_loops_thread_ctx.idle_queue, &message, TX_NO_WAIT);
    BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);

    // ignore failure here; NOP is just to ensure something is in the work queue
    message.type = LOOP_QUEUE_ENTRY_TYPE_NOP;
    tx_queue_send(&s_power_loops_thread_ctx.work_queue, &message, TX_NO_WAIT);
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
