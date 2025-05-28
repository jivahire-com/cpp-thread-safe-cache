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
#include <FpFwLock.h>   // for FPFW_LOCK
#include <FpFwUtils.h>
#include <gtimer_prodfw.h> //for timestamp
#include <stdbool.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MS_TO_TX_TICKS(ms) (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)
#define TICKS_PER_MINUTE   (TX_TIMER_TICKS_PER_SECOND * 60)
#define TICKS_PER_HOUR     (TICKS_PER_MINUTE * 60)
#define TICKS_PER_DAY      (TICKS_PER_HOUR * 24)

#define TLM_SVC_STACK_SIZE (TX_MINIMUM_STACK + 4096)

#define DATA_AGGR_START_TICK    (TX_TIMER_TICKS_PER_SECOND / 2)
#define DATA_AGGR_PKG_PERIOD_MS (10)

#define MILLISECONDS_PER_SECOND 1000
#define SECONDS_PER_MINUTE      60
#define MINUTES_PER_HOUR        60
#define HOURS_PER_DAY           24

#define MILLISECONDS_PER_DAY (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY)

#define ALL_EVENT_GROUP_BITS (0xFFFFFFFF)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static TX_THREAD s_tlm_svc_thread;
static uint8_t s_tlm_svc_thread_stack[TLM_SVC_STACK_SIZE];
static FPFW_LOCK s_lock;

TX_TIMER data_aggr_tmr;
TX_TIMER inst_sample_tmr;
TX_TIMER power_pkg_tmr;
TX_TIMER _24hr_pkg_tmr;

TX_EVENT_FLAGS_GROUP s_thread_control;
tlm_operating_mode_t pending_mode_change;

bool _24_pkg_pending = false;

telemetry_executive_status_t tlm_executive_status = {
    .op_mode = TLM_OP_MODE_COLLECTING_DATA,
    .pwr_pkg_period_ms = 0,
    .inst_pkg_sample_period_ms = 0,
    .data_aggr_period_ms = DATA_AGGR_PKG_PERIOD_MS,
    .twenty_four_hr_pkg_period_ms = MILLISECONDS_PER_DAY,
    .pwr_pkg_timer_active = false,
    .inst_sample_timer_active = false,
    .data_aggr_timer_active = false,
    .twenty_four_hr_pkg_timer_active = false,
};

/*------------- Functions ----------------*/
void exec_tlm_cmpnt_init(uint32_t pwr_pkg_period_ms, uint32_t inst_pkg_sample_period_ms)
{
    tlm_executive_status.pwr_pkg_period_ms = pwr_pkg_period_ms;
    tlm_executive_status.inst_pkg_sample_period_ms = inst_pkg_sample_period_ms;

    FpFwLockInitialize(&s_lock);

    UINT txStatus = tx_thread_create(&s_tlm_svc_thread,
                                     "tlm_service",          // Thread name
                                     tlm_svc_thread,         // Thread entry
                                     0,                      // Entry input
                                     s_tlm_svc_thread_stack, // Thread stack
                                     TLM_SVC_STACK_SIZE,     // Thread stack size
                                     5,                      // Priority
                                     5,                      // Preempt Threshold
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    // note, telemetry sampling needs a faster timer than the minimum rtos tick rate.
    // this timer will be updated at a later time
    // auto activate to pull data from sensor fifos, even if tlm package reporting is not enabled
    txStatus = tx_timer_create(&data_aggr_tmr,       /* TX_TIMER *timer_ptr */
                               "tlm_svc_data_aggr",  /* CHAR *name_ptr */
                               data_aggr_timer_cb,   /* VOID (*expiration_function)(ULONG input) */
                               0,                    /* ULONG expiration_input */
                               DATA_AGGR_START_TICK, /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(DATA_AGGR_PKG_PERIOD_MS), /* ULONG reschedule_ticks */
                               TX_AUTO_ACTIVATE);                       /* UINT auto_activate) */
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

    txStatus = tx_timer_create(&_24hr_pkg_tmr,          /* TX_TIMER *timer_ptr */
                               "tlm_svc_24hr_pkg",      /* CHAR *name_ptr */
                               every_24hr_pkg_timer_cb, /* VOID (*expiration_function)(ULONG input)*/
                               0,                       /* ULONG expiration_input */
                               MS_TO_TX_TICKS(MILLISECONDS_PER_DAY), /* ULONG initial_ticks >= 1 */
                               MS_TO_TX_TICKS(MILLISECONDS_PER_DAY), /* ULONG  reschedule_ticks */
                               TX_NO_ACTIVATE);                      /* UINT auto_activate) */
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

    txStatus = tx_event_flags_create(&s_thread_control, "Telemetry Service Event");
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void tlm_svc_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);
    static ULONG current_bits;

    while (true)
    {
        UINT txStatus = tx_event_flags_get(&s_thread_control, ALL_EVENT_GROUP_BITS, TX_OR_CLEAR, &current_bits, TX_WAIT_FOREVER);
        FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);

        if (current_bits & MODE_CHANGE_PENDING)
        {
            exec_tlm_cmpnt_change_telemetry_mode(pending_mode_change);
        }

        if (current_bits & DATA_AGGR_TMR_EXPIRED)
        {
            if (tlm_executive_status.op_mode == TLM_OP_MODE_SENSOR_FIFO_RAW_DATA)
            {
                in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();
            }
            else
            {
                data_proc_tlm_cmpnt_process_input_data();
            }
        }

        if (tlm_executive_status.op_mode == TLM_OP_MODE_PUBLISHING)
        {
            // handles case where timer may have set expiration event while mode change was in progress
            if (current_bits & INST_SAMPLE_TMR_EXPIRED)
            {
                data_proc_tlm_cmpnt_prepare_data_for_inst_sample();
                in_band_tlm_cmpnt_add_inst_sample();
            }

            if (current_bits & PWR_PKG_TMR_EXPIRED)
            {
                in_band_tlm_cmpnt_generate_pwr_pkg();

                if (_24_pkg_pending)
                {
                    in_band_tlm_cmpnt_generate_24hr_pkg();
                    _24_pkg_pending = false;
                }
            }

            if (current_bits & EVERY_24HR_PKG_TMR_EXPIRED)
            {
                // data for the 24hr package requires data collected from other sources
                // the data_proc api will kick off those requests.
                // when the next pwr package expires, then the 24hr package will be generated
                _24_pkg_pending = true;
                data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg();
            }
        }

        if (current_bits & NEW_INBAND_MTS_MESSAGE)
        {
            in_band_tlm_cmpnt_handle_incoming_mts_msgs();
        }
    }
}

void data_aggr_timer_cb(ULONG context)
{
    FPFW_UNUSED(context);

    UINT txStatus = tx_event_flags_set(&s_thread_control, DATA_AGGR_TMR_EXPIRED, TX_OR);
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

void exec_tlm_cmpnt_notify_new_in_band_mts_message(void)
{
    UINT txStatus = tx_event_flags_set(&s_thread_control, NEW_INBAND_MTS_MESSAGE, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

void exec_tlm_cmpnt_set_mode_change_pending(tlm_operating_mode_t pending_mode)
{
    pending_mode_change = pending_mode;

    UINT txStatus = tx_event_flags_set(&s_thread_control, MODE_CHANGE_PENDING, TX_OR);
    FPFW_RUNTIME_ASSERT_EXT(txStatus == TX_SUCCESS, txStatus, 0, 0, 0);
}

bool exec_tlm_cmpnt_is_telemetry_publishing_enabled(void)
{
    return tlm_executive_status.op_mode == TLM_OP_MODE_PUBLISHING;
}

void exec_tlm_cmpnt_get_status(telemetry_executive_status_t* status)
{
    UINT active;

    tx_timer_info_get(&data_aggr_tmr, TX_NULL, &active, TX_NULL, TX_NULL, TX_NULL);
    tlm_executive_status.data_aggr_timer_active = (active == TX_TRUE) ? true : false;

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

void exec_tlm_cmpnt_udpdate_timer_periods(uint32_t data_aggr_period_ms,
                                          uint32_t inst_pkg_sample_period_ms,
                                          uint32_t pwr_pkg_period_ms,
                                          uint32_t twenty_four_hr_pkg_period_ms)
{
    uint32_t data_aggr_period_ticks = MS_TO_TX_TICKS(data_aggr_period_ms);
    uint32_t inst_sample_period_ticks = MS_TO_TX_TICKS(inst_pkg_sample_period_ms);
    uint32_t pwr_pkg_period_ticks = MS_TO_TX_TICKS(pwr_pkg_period_ms);
    uint32_t twenty_four_hr_pkg_period_ticks = MS_TO_TX_TICKS(twenty_four_hr_pkg_period_ms);

    if ((data_aggr_period_ticks == 0) || (inst_sample_period_ticks == 0) || (pwr_pkg_period_ticks == 0) ||
        (twenty_four_hr_pkg_period_ticks == 0))
    {
        FPFW_ET_LOG(ExecInvalidTimerPeriod, data_aggr_period_ms, inst_pkg_sample_period_ms, pwr_pkg_period_ms, twenty_four_hr_pkg_period_ms);
        return;
    }

    // lock as may be called from cli thread
    FPFW_LOCK_STATE lock_state;
    lock_state = FpFwLockAcquire(&s_lock);

    tlm_executive_status.data_aggr_period_ms = data_aggr_period_ms;
    tlm_executive_status.inst_pkg_sample_period_ms = inst_pkg_sample_period_ms;
    tlm_executive_status.pwr_pkg_period_ms = pwr_pkg_period_ms;
    tlm_executive_status.twenty_four_hr_pkg_period_ms = twenty_four_hr_pkg_period_ms;

    FpFwLockRelease(&s_lock, lock_state);

    // api's are thread safe
    UINT tx_status = tx_timer_change(&data_aggr_tmr, 1, data_aggr_period_ticks);
    if (tx_status != TX_SUCCESS)
    {
        FPFW_ET_LOG(TimerChangeFail, 1, tx_status);
    }
    tx_status = tx_timer_change(&inst_sample_tmr, 1, inst_sample_period_ticks);
    if (tx_status != TX_SUCCESS)
    {
        FPFW_ET_LOG(TimerChangeFail, 2, tx_status);
    }
    tx_status = tx_timer_change(&power_pkg_tmr, 1, pwr_pkg_period_ticks);
    if (tx_status != TX_SUCCESS)
    {
        FPFW_ET_LOG(TimerChangeFail, 3, tx_status);
    }
    tx_status = tx_timer_change(&_24hr_pkg_tmr, 1, twenty_four_hr_pkg_period_ticks);
    if (tx_status != TX_SUCCESS)
    {
        FPFW_ET_LOG(TimerChangeFail, 4, tx_status);
    }
}

void exec_tlm_cmpnt_change_telemetry_mode(tlm_operating_mode_t new_mode)
{
    tlm_operating_mode_t current_mode = tlm_executive_status.op_mode;
    if (current_mode == new_mode)
    {
        FPFW_ET_LOG(ExecInvalidModeChange, current_mode, new_mode);
        return;
    }
    if ((current_mode == TLM_OP_MODE_SENSOR_FIFO_RAW_DATA) && (new_mode != TLM_OP_MODE_DISABLED))
    {
        FPFW_ET_LOG(ExecInvalidModeChange, current_mode, new_mode);
        return;
    }

    run_timer_exit_actions(current_mode);
    in_band_tlm_cmpnt_tlm_mode_exit_actions(current_mode);

    tlm_executive_status.op_mode = new_mode;

    in_band_tlm_cmpnt_tlm_mode_enter_actions(new_mode);
    run_timer_enter_actions(new_mode);

    FPFW_ET_LOG(ExecTlmSvcModeChange, current_mode, new_mode);
}

void run_timer_exit_actions(tlm_operating_mode_t exiting_mode)
{
    switch (exiting_mode)
    {
    case TLM_OP_MODE_PUBLISHING:
        tx_timer_deactivate(&inst_sample_tmr);
        tx_timer_deactivate(&power_pkg_tmr);
        tx_timer_deactivate(&_24hr_pkg_tmr);
        _24_pkg_pending = false;
        break;

    case TLM_OP_MODE_DISABLED:
        tx_timer_activate(&data_aggr_tmr);
        break;

    case TLM_OP_MODE_COLLECTING_DATA:
        break;

    case TLM_OP_MODE_SENSOR_FIFO_RAW_DATA:
        break;

    default:
        FPFW_RUNTIME_ASSERT_EXT(false, exiting_mode, 0, 0, 0);
        break;
    }
}
void run_timer_enter_actions(tlm_operating_mode_t entering_mode)
{
    switch (entering_mode)
    {
    case TLM_OP_MODE_PUBLISHING:
        if (in_band_tlm_cmpnt_is_any_instantaneous_enabled())
        {
            tx_timer_activate(&inst_sample_tmr);
        }
        tx_timer_activate(&power_pkg_tmr);
        tx_timer_activate(&_24hr_pkg_tmr);
        break;

    case TLM_OP_MODE_DISABLED:
        tx_timer_deactivate(&data_aggr_tmr);
        tx_timer_deactivate(&inst_sample_tmr);
        tx_timer_deactivate(&power_pkg_tmr);
        tx_timer_deactivate(&_24hr_pkg_tmr);
        break;

    case TLM_OP_MODE_COLLECTING_DATA:
        tx_timer_activate(&data_aggr_tmr);
        break;

    case TLM_OP_MODE_SENSOR_FIFO_RAW_DATA:
        break;

    default:
        FPFW_RUNTIME_ASSERT_EXT(false, entering_mode, 0, 0, 0);
        break;
    }
}