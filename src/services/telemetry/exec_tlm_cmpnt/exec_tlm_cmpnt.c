//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_executive.c
 * Rtos thread for telemetry services
 */

/*------------- Includes -----------------*/

#include "data_proc_tlm_cmpnt.h"
#include "in_band_tlm_cmpnt.h"
#include "out_of_band_tlm_cmpnt.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>
#include <stdbool.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MS_TO_TX_TICKS(ms) (((ms)*TX_TIMER_TICKS_PER_SECOND) / 1000)
#define TICKS_PER_MINUTE   (TX_TIMER_TICKS_PER_SECOND * 60)
#define TICKS_PER_HOUR     (TICKS_PER_MINUTE * 60)
#define TICKS_PER_24HR     (TICKS_PER_HOUR * 24)

#define TLM_SVC_STACK_SIZE (TX_MINIMUM_STACK + 2048)

#define PWR_AGGR_START_TICK    (TX_TIMER_TICKS_PER_SECOND / 2)
#define PWR_AGGR_PERIODIC_TICK (1)

#define PERF_RPT_START_TICK           (TX_TIMER_TICKS_PER_SECOND * 2)
#define EVERY_24_HR_RPT_PERIODIC_TICK (TICKS_PER_24HR)

#define ALL_EVENT_GROUP_BITS (0xFFFFFFFF)

// event flags
#define PWR_AGGR_TMR_EXPIRED       (1 << 0)
#define PERF_RPT_TMR_EXPIRED       (1 << 1)
#define PWR_RPT_TMR_EXPIRED        (1 << 2)
#define EVERY_24HR_RPT_TMR_EXPIRED (1 << 3)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void tlm_svc_thread(ULONG thread_input);
static void pwr_aggr_timer_cb(ULONG context);
static void perf_rpt_timer_cb(ULONG context);
static void pwr_rpt_timer_cb(ULONG context);
static void every_24hr_rpt_timer_cb(ULONG context);

/*-- Declarations (Statics and globals) --*/
static TX_THREAD s_tlm_svc_thread;
static uint8_t s_tlm_svc_thread_stack[TLM_SVC_STACK_SIZE];

static TX_TIMER s_pwr_aggr_tmr;
static TX_TIMER s_perf_rpt_tmr;
static TX_TIMER s_power_rpt_tmr;
static TX_TIMER s_24hr_rpt_tmr;

static TX_EVENT_FLAGS_GROUP s_thread_control;

/*------------- Functions ----------------*/
void exec_tlm_cmpnt_init(uint32_t pwr_rpt_period_ms, uint32_t perf_rpt_period_ms)
{
    UINT txStatus = tx_thread_create(&s_tlm_svc_thread,
                                     "tlm_service",          // Thread name
                                     tlm_svc_thread,         // Thread entry
                                     0,                      // Entry input
                                     s_tlm_svc_thread_stack, // Thread stack
                                     TLM_SVC_STACK_SIZE,     // Thread stack size
                                     3,                      // Priority
                                     3,                      // Preempt Threshold
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    // note, telemetry sampling needs a faster timer than the minimum rtos tick rate.
    // this timer will be updated at a later time
    txStatus = tx_timer_create(&s_pwr_aggr_tmr,        /* TX_TIMER *timer_ptr */
                               "tlm_svc_pwr_aggr",     /* CHAR *name_ptr */
                               pwr_aggr_timer_cb,      /* VOID (*expiration_function)(ULONG input) */
                               0,                      /* ULONG expiration_input */
                               PWR_AGGR_START_TICK,    /* ULONG initial_ticks >= 1 */
                               PWR_AGGR_PERIODIC_TICK, /* ULONG reschedule_ticks */
                               TX_AUTO_ACTIVATE);      /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&s_perf_rpt_tmr,    /* TX_TIMER *timer_ptr */
                               "tlm_svc_perf_rpt", /* CHAR *name_ptr */
                               perf_rpt_timer_cb,  /* VOID (*expiration_function)(ULONG input) */
                               0,                  /* ULONG expiration_input */
                               MS_TO_TX_TICKS(perf_rpt_period_ms), /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(perf_rpt_period_ms), /* ULONG reschedule_ticks */
                               TX_NO_ACTIVATE);                    /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&s_power_rpt_tmr,  /* TX_TIMER *timer_ptr */
                               "tlm_svc_pwr_rpt", /* CHAR *name_ptr */
                               pwr_rpt_timer_cb,  /* VOID (*expiration_function)(ULONG input) */
                               0,                 /* ULONG expiration_input */
                               MS_TO_TX_TICKS(pwr_rpt_period_ms), /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(pwr_rpt_period_ms), /* ULONG reschedule_ticks */
                               TX_AUTO_ACTIVATE);                 /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&s_24hr_rpt_tmr,               /* TX_TIMER *timer_ptr */
                               "tlm_svc_24hr_rpt",            /* CHAR *name_ptr */
                               every_24hr_rpt_timer_cb,       /* VOID (*expiration_function)(ULONG input) */
                               0,                             /* ULONG expiration_input */
                               EVERY_24_HR_RPT_PERIODIC_TICK, /* ULONG initial_ticks >= 1 */
                               EVERY_24_HR_RPT_PERIODIC_TICK, /* ULONG reschedule_ticks */
                               TX_NO_ACTIVATE);               /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_event_flags_create(&s_thread_control, "Telemetry Service Event");
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

static void tlm_svc_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);
    static ULONG current_bits;

    while (true)
    {
        UINT txStatus = tx_event_flags_get(&s_thread_control, ALL_EVENT_GROUP_BITS, TX_OR_CLEAR, &current_bits, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

        if (current_bits & PWR_AGGR_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_pwr_tlm_data();
        }

        if (current_bits & PERF_RPT_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_perf_tlm_data();
            in_band_tlm_cmpnt_generate_perf_report();
        }

        if (current_bits & PWR_RPT_TMR_EXPIRED)
        {
            in_band_tlm_cmpnt_generate_pwr_report();
        }

        if (current_bits & EVERY_24HR_RPT_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_24hr_tlm_data();
        }
    }
}

static void pwr_aggr_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, PWR_AGGR_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

static void perf_rpt_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, PERF_RPT_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

static void pwr_rpt_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, PWR_RPT_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

static void every_24hr_rpt_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, EVERY_24HR_RPT_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}
