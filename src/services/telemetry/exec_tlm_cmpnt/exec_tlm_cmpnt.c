//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_executive.c
 * Rtos thread for telemetry services
 */

/*------------- Includes -----------------*/

#include "exec_tlm_cmpnt.h"

#include "data_proc_tlm_cmpnt.h"
#include "exec_tlm_cmpnt_i.h"
#include "in_band_tlm_cmpnt.h"
#include "out_of_band_tlm_cmpnt.h"
#include "telemetry_events_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>
#include <gtimer_prodfw.h> //for timestamp
#include <stdbool.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MS_TO_TX_TICKS(ms) (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)
#define TICKS_PER_MINUTE   (TX_TIMER_TICKS_PER_SECOND * 60)
#define TICKS_PER_HOUR     (TICKS_PER_MINUTE * 60)
#define TICKS_PER_24HR     (TICKS_PER_HOUR * 24)

#define TLM_SVC_STACK_SIZE (TX_MINIMUM_STACK + 2048)

#define PWR_AGGR_START_TICK    (TX_TIMER_TICKS_PER_SECOND / 2)
#define PWR_AGGR_PKG_PERIOD_MS (10)

#define EVERY_24_HR_PKG_PERIODIC_TICK (TICKS_PER_24HR)

#define ALL_EVENT_GROUP_BITS (0xFFFFFFFF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static TX_THREAD s_tlm_svc_thread;
static uint8_t s_tlm_svc_thread_stack[TLM_SVC_STACK_SIZE];

TX_TIMER pwr_aggr_tmr;
TX_TIMER inst_sample_tmr;
TX_TIMER power_pkg_tmr;
TX_TIMER _24hr_pkg_tmr;

TX_EVENT_FLAGS_GROUP s_thread_control;

telemetry_executive_status_t tlm_executive_status = {
    .op_mode = TLM_OP_MODE_NOMINAL,
    .pwr_pkg_period_ms = 0,
    .inst_pkg_sample_period_ms = 0,
    .pwr_aggr_period_ms = PWR_AGGR_PKG_PERIOD_MS,
    .pwr_pkg_timer_active = false,
    .inst_sample_timer_active = false,
    .pwr_aggr_timer_active = false,
    .twenty_four_hr_pkg_timer_active = false,
};

/*------------- Functions ----------------*/
void exec_tlm_cmpnt_init(uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms)
{
    tlm_executive_status.pwr_pkg_period_ms = pwr_pkg_period_ms;
    tlm_executive_status.inst_pkg_sample_period_ms = inst_pkg_sample_period_ms;

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
    // auto activate to pull data from sensor fifos, even if tlm package reporting is not enabled
    txStatus = tx_timer_create(&pwr_aggr_tmr,       /* TX_TIMER *timer_ptr */
                               "tlm_svc_pwr_aggr",  /* CHAR *name_ptr */
                               pwr_aggr_timer_cb,   /* VOID (*expiration_function)(ULONG input) */
                               0,                   /* ULONG expiration_input */
                               PWR_AGGR_START_TICK, /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(PWR_AGGR_PKG_PERIOD_MS), /* ULONG reschedule_ticks */
                               TX_AUTO_ACTIVATE);                      /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&inst_sample_tmr,      /* TX_TIMER *timer_ptr */
                               "tlm_svc_inst_sample", /* CHAR *name_ptr */
                               inst_sample_timer_cb,  /* VOID (*expiration_function)(ULONG input) */
                               0,                     /* ULONG expiration_input */
                               MS_TO_TX_TICKS(inst_pkg_sample_period_ms), /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(inst_pkg_sample_period_ms), /* ULONG reschedule_ticks */
                               TX_NO_ACTIVATE);                           /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&power_pkg_tmr,    /* TX_TIMER *timer_ptr */
                               "tlm_svc_pwr_pkg", /* CHAR *name_ptr */
                               pwr_pkg_timer_cb,  /* VOID (*expiration_function)(ULONG input) */
                               0,                 /* ULONG expiration_input */
                               MS_TO_TX_TICKS(pwr_pkg_period_ms), /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(pwr_pkg_period_ms), /* ULONG reschedule_ticks */
                               TX_NO_ACTIVATE);                   /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_timer_create(&_24hr_pkg_tmr,                /* TX_TIMER *timer_ptr */
                               "tlm_svc_24hr_pkg",            /* CHAR *name_ptr */
                               every_24hr_pkg_timer_cb,       /* VOID (*expiration_function)(ULONG input)*/
                               0,                             /* ULONG expiration_input */
                               EVERY_24_HR_PKG_PERIODIC_TICK, /* ULONG initial_ticks >= 1 */
                               EVERY_24_HR_PKG_PERIODIC_TICK, /* ULONG  reschedule_ticks */
                               TX_NO_ACTIVATE);               /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_event_flags_create(&s_thread_control, "Telemetry Service Event");
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void developer_test()
{
#define PWR_DEV_SAMPLING_PERIOD_MS (20)
    tx_timer_change(&power_pkg_tmr, MS_TO_TX_TICKS(PWR_DEV_SAMPLING_PERIOD_MS), MS_TO_TX_TICKS(PWR_DEV_SAMPLING_PERIOD_MS));

#define INST_DEV_SAMPLING_PERIOD_MS (10)
    tx_timer_change(&inst_sample_tmr, MS_TO_TX_TICKS(INST_DEV_SAMPLING_PERIOD_MS), MS_TO_TX_TICKS(INST_DEV_SAMPLING_PERIOD_MS));
    tlm_executive_status.inst_pkg_sample_period_ms = PWR_DEV_SAMPLING_PERIOD_MS;

    exec_tlm_cmpnt_enable_disable_telemetry(true);
}

void tlm_svc_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);
    static ULONG current_bits;

    // uncomment below for developer testing
    // developer_test();

    while (true)
    {
        UINT txStatus = tx_event_flags_get(&s_thread_control, ALL_EVENT_GROUP_BITS, TX_OR_CLEAR, &current_bits, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

        if (current_bits & PWR_AGGR_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_pwr_tlm_data();
        }

        if (current_bits & INST_SAMPLE_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_inst_tlm_data();
            in_band_tlm_cmpnt_add_inst_sample();
        }

        if (current_bits & PWR_PKG_TMR_EXPIRED)
        {
            in_band_tlm_cmpnt_generate_pwr_pkg();
        }

        if (current_bits & EVERY_24HR_PKG_TMR_EXPIRED)
        {
            data_proc_tlm_cmpnt_aggregate_24hr_tlm_data();
        }

        if (current_bits & NEW_INBAND_DCS_MESSAGE)
        {
            in_band_tlm_cmpnt_handle_incoming_dcs_msgs();
        }
    }
}

void pwr_aggr_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, PWR_AGGR_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void inst_sample_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, INST_SAMPLE_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void pwr_pkg_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, PWR_PKG_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void every_24hr_pkg_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, EVERY_24HR_PKG_TMR_EXPIRED, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void exec_tlm_cmpnt_notify_new_in_band_dcs_message(void)
{
    UINT txStatus = tx_event_flags_set(&s_thread_control, NEW_INBAND_DCS_MESSAGE, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

bool exec_tlm_cmpnt_is_telemetry_enabled(void)
{
    return tlm_executive_status.op_mode == TLM_OP_MODE_NOMINAL;
}

void exec_tlm_cmpnt_enable_disable_telemetry(bool enable)
{
    data_proc_tlm_cmpnt_enable_disable_transition(enable);

    if (enable)
    {
        tx_timer_activate(&pwr_aggr_tmr);

        if (tlm_executive_status.inst_pkg_sample_period_ms > 0)
        {
            tx_timer_activate(&inst_sample_tmr);
        }
        tx_timer_activate(&inst_sample_tmr);
        tx_timer_activate(&power_pkg_tmr);
        tx_timer_activate(&_24hr_pkg_tmr);

        tlm_executive_status.op_mode = TLM_OP_MODE_NOMINAL;

        FPFW_ET_LOG(DataCollectionEnabled);
    }
    else
    {
        tlm_executive_status.op_mode = TLM_OP_MODE_DISABLED;

        tx_timer_deactivate(&pwr_aggr_tmr);
        tx_timer_deactivate(&inst_sample_tmr);
        tx_timer_deactivate(&power_pkg_tmr);
        tx_timer_deactivate(&_24hr_pkg_tmr);

        FPFW_ET_LOG(DataCollectionDisabled);
    }
}

void exec_tlm_cmpnt_get_status(telemetry_executive_status_t* status)
{
    UINT active;

    tx_timer_info_get(&pwr_aggr_tmr, TX_NULL, &active, TX_NULL, TX_NULL, TX_NULL);
    tlm_executive_status.pwr_aggr_timer_active = (active == TX_TRUE) ? true : false;

    tx_timer_info_get(&inst_sample_tmr, TX_NULL, &active, TX_NULL, TX_NULL, TX_NULL);
    tlm_executive_status.inst_sample_timer_active = (active == TX_TRUE) ? true : false;

    tx_timer_info_get(&power_pkg_tmr, TX_NULL, &active, TX_NULL, TX_NULL, TX_NULL);
    tlm_executive_status.pwr_pkg_timer_active = (active == TX_TRUE) ? true : false;

    tx_timer_info_get(&_24hr_pkg_tmr, TX_NULL, &active, TX_NULL, TX_NULL, TX_NULL);
    tlm_executive_status.twenty_four_hr_pkg_timer_active = (active == TX_TRUE) ? true : false;

    *status = tlm_executive_status;
}

uint64_t exec_tlm_cmpnt_get_timestamp_microseconds(void)
{
    uint64_t timestamp_us = 0;

    uint64_t tick_count = gtimer_prodfw_get_counter();
    uint32_t frequency = gtimer_prodfw_get_frequency();

    timestamp_us = (tick_count * 1000000) / frequency;

    return timestamp_us;
}