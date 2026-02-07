//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_events.h
 * This file defines the event trace events for the power management
 * service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <assert.h>                 // for static_assert
#include <corebits.h>               // for COREBITS_FMT_DATA
#include <event_trace.h>            // for _MH_IS_PARENTHESIZED_imp1, _MH_SP...
#include <event_trace_providers.h>  // for EVENT_TRACE_PROVIDER_ID_SCP_POWER
#include <stdint.h>                 // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/* Helper macros to log power events */
#define POWER_ET_FATAL(type, status, param) EventWritePowerFatal((status), (param), (type))
#define POWER_ET_ERROR(type, param)         EventWritePowerErrorParam((param), (type))
#define POWER_ET_WARN(type, param)          EventWritePowerWarnParam((param), (type))
#define POWER_ET_STATUS(type)               EventWritePowerStatus((type))
#define POWER_ET_STATUS_PARAM(type, param)  EventWritePowerStatusParam((param), (type))
#define POWER_ET_LOG_TRACE_STATUS(type)    \
    EventWritePowerLogTraceStatus((type))
#define POWER_ET_LOG_TRACE_STR(message) \
    EventWritePowerLogTraceStr((message))
#define POWER_ET_LOG_TRACE_PARAM(type, param) \
    EventWritePowerLogTraceParam((param), (type))
#define POWER_ET_LOG_TRACE_CHANGE_STATE(loop_id, state) \
    EventWritePowerLogChangeState((loop_id), (state))
#define POWER_ET_LOG_TRACE_SIGNAL_EVT(event) \
    EventWritePowerLogSignalEvt((event))
#define POWER_ET_LOG_TRACE_PLL_ERROR_STATUS(core_pll_base_addr, pll_error_sr, core_idx, wait_pll_lock_timer_exp, wait_fll_lock_timer_exp, wait_fll_cal_timer_exp, wait_freq_change_timer_exp, pllrawlockerror, fllrawlockerror, pll_error_mask_cr) \
    EventWritePowerLogPllErrorStatus((core_pll_base_addr), \
                                    (pll_error_sr), (core_idx),(wait_pll_lock_timer_exp), \
                                    (wait_fll_lock_timer_exp), (wait_fll_cal_timer_exp), \
                                    (wait_freq_change_timer_exp), (pllrawlockerror), \
                                    (fllrawlockerror), (pll_error_mask_cr))
#ifdef COREBITS_FMT_DATA
    #define POWER_ET_AFFECTED_CORES(type, cores) EventWritePowerWarnParamAffectedCores(COREBITS_FMT_DATA(cores), (type))
#endif

/* Helpers to encode values in 32b parameter */
#define POWER_ET_ENCODE_DETAIL_CORE(detail, core)     ((detail << 8) | core)
#define POWER_ET_ENCODE_FREQ_CURVE(freq, curve)       ((freq << 8) | curve)
#define POWER_ET_ENCODE_RETRIES_STATE(retries, state) ((retries << 16) | state)
#define POWER_ET_ENCODE_MODULE_EVENT(module, event)   ((module << 16) | event)

#define ET_NOPARAM 0
#define PWR_TRACE_STR_SIZE      192
/*-------------- Typedefs ----------------*/

typedef enum
{
    POWER_ET_TYPE_INIT_MAX_RESOURCES,
    POWER_ET_TYPE_DVFS_INIT,
    POWER_ET_TYPE_DVFS_REINIT,
    POWER_ET_TYPE_PLATFORM_NOT_SUPPORTED_SKIPPING_DVFS_INIT,
    POWER_ET_TYPE_PLATFORM_NOT_SUPPORTED_SKIPPING_SOC_HW_INIT,
    POWER_ET_TYPE_FUSE_USING_DEFAULT_CURVE,
    POWER_ET_TYPE_FUSE_LDO_OFFSET,
    POWER_ET_TYPE_FUSE_LDO_CAPPED,
    POWER_ET_TYPE_KNOB_MPMM_DISABLED,
    POWER_ET_TYPE_CTRLLOOP_INVALID_MIN_PLIMIT,
    POWER_ET_TYPE_CTRLLOOP_PLIMITS_PENDING,
    POWER_ET_TYPE_CTRLLOOP_PLIMITS_SUCCESSFUL,
    POWER_ET_TYPE_CTRLLOOP_ERROR_ENTRY,
    POWER_ET_TYPE_VRTEL_ERROR_ENTRY,
    POWER_ET_TYPE_FORCE_PSTATE_CLIPPED,
    POWER_ET_TYPE_VCPU_UNEXPECED_LL_LOSS_CALCULATION,
    POWER_ET_TYPE_VCPU_CALCULATED_OVER_MAX,
    POWER_ET_TYPE_VCPU_CALCULATED_UNDER_MIN,
    POWER_ET_TYPE_CURVE_HAS_NO_VALID_PSTATES,
    POWER_ET_TYPE_REMOTE_SEND_CAP_UNEXPECTED,
    POWER_ET_TYPE_REMOTE_UNEXPECTED_MCP_ICC,
    POWER_ET_TYPE_REMOTE_UNEXPECTED_SCP_ICC,
    POWER_ET_TYPE_REMOTE_UNEXPECTED_SCP_ICC_ACK,
    POWER_ET_TYPE_AVS_READ_ALL,
    POWER_ET_TYPE_AVS_READ_WRITE,
    POWER_ET_TYPE_UNEXPECTED_EVENT,
    POWER_ET_TYPE_UNEXPECTED_DELAYED_EVENT,
    POWER_ET_TYPE_UNEXPECTED_NOTIFICATION,
    POWER_ET_TYPE_UNEXPECTED_DVFS_MESSAGE_TYPE,
    POWER_ET_TYPE_PLDM_POWER_SENSOR,
    POWER_ET_TYPE_PLDM_THROTTLING_SENSOR,
    POWER_ET_TYPE_PLDM_POWER_CAP_EFFECTER,
    POWER_ET_TYPE_PLL_ERROR_SR,
    POWER_ET_D2D_SEND_FAIL,
    POWER_ET_D2D_RECV_FAIL,
    POWER_ET_D2D_SEND_FAIL_CB,
    POWER_ET_D2D_RECV_FAIL_CB,
    POWER_ET_D2D_UNEXPECTED_STATE,
    POWER_ET_TYPE_REMOTE_LOOP_FAILURE,
    POWER_ET_D2D_PREV_DATA_NOT_CONSUMED,
    POWER_ET_D2D_REMOTE_DATA_CORRUPTED,
    POWER_ET_D2D_REMOTE_INVALID_PWRCAP,
    POWER_ET_D2D_PID_MISMATCH,
    POWER_ET_TYPE_SOC_PWR_CALCULATION,
    POWER_ET_D2D_SEND_TRANSACTION_COUNT,
    POWER_ET_D2D_RECV_TRANSACTION_COUNT,
    POWER_ET_TYPE_LOOP_STATE_CHANGE,
    POWER_ET_TYPE_LOOP_SIGNAL_EVENT,
    POWER_ET_TYPE_CTRL_LOOP_TIMER_CB,
    POWER_ET_TYPE_PVT_TELEM_TIMER_CB,
    POWER_ET_D2D_SYNC_WAITING,
    POWER_ET_TYPE_DTS_TEMP_OUT_OF_RANGE,
    POWER_ET_TYPE_ALL_LOOPS_ITERATION_METRICS,

    POWER_ET_TYPE_COUNT
}   E_POWER_ET_TYPE_T;

typedef enum {
    POWER_ET_ID_TYPE_FATAL,
    POWER_ET_ID_TYPE_ERROR,
    POWER_ET_ID_TYPE_WARNING,
    POWER_ET_ID_TYPE_WARNING_AFFECTED_CORES,
    POWER_ET_ID_TYPE_STATUS_NOPARAMS,
    POWER_ET_ID_TYPE_STATUS_PARAMS,
    POWER_ET_ID_TYPE_LOG_TRACE,
    POWER_ET_ID_TYPE_LOG_TRACE_NO_PARAMS,
    POWER_ET_ID_TYPE_LOG_TRACE_STR,
    POWER_ET_ID_TYPE_LOG_TRACE_PARAM,
    POWER_ET_ID_TYPE_CHANGE_STATE,
    POWER_ET_ID_TYPE_SIGNAL_EVT,
    POWER_ET_ID_TYPE_PLL_ERROR,
    POWER_ET_ID_TYPE_ALL_LOOP_METRICS,

    POWER_ET_ID_TYPE_COUNT
} E_POWER_ET_ID_TYPES_T;

static_assert(POWER_ET_TYPE_COUNT <= UINT8_MAX, "Power ET type count exceeds space allocated in trace.");
static_assert(POWER_ET_ID_TYPE_COUNT <= UINT8_MAX, "Power ET ID count exceeds space allocated in trace.");

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_POWER,
    SocPowerModule,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR 
                    | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_FATAL,                    // Event ID, for this provider
                     PowerFatal,                                // Event Name
                     FPFW_ET_LEVEL_FATAL,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_ERROR,                    // Event ID, for this provider
                     PowerErrorParam,                           // Event Name
                     FPFW_ET_LEVEL_ERROR,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_WARNING,                  // Event ID, for this provider
                     PowerWarnParam,                            // Event Name
                     FPFW_ET_LEVEL_WARNING,                     // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

// Generate an Event associated with the provider. This defines the event AND the event write function for it
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_WARNING_AFFECTED_CORES,   // Event ID, for this provider
                     PowerWarnParamAffectedCores,               // Event Name
                     FPFW_ET_LEVEL_WARNING,                     // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cores2),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cores1),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, cores0),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_STATUS_NOPARAMS,          // Event ID, for this provider
                     PowerStatus,                               // Event Name
                     FPFW_ET_LEVEL_INFO,                        // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_STATUS_PARAMS,            // Event ID, for this provider
                     PowerStatusParam,                          // Event Name
                     FPFW_ET_LEVEL_INFO,                        // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

#define POWER_ET_LOG_TRACE_UINT64_COUNT 4   /* count should match UINT64 fields below in PowerLogTrace */

/* This event is for logging power log entries to event trace; should not be enabled by default (LEVEL_DEBUG) */
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_LOG_TRACE,                // Event ID, for this provider
                     PowerLogTrace,                             // Event Name
                     FPFW_ET_LEVEL_DEBUG,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, type_timestamp),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, data0),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, data1),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, data2))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                    POWER_ET_ID_TYPE_LOG_TRACE_NO_PARAMS,                // Event ID, for this provider
                    PowerLogTraceStatus,                             // Event Name
                    FPFW_ET_LEVEL_DEBUG,                       // Event Log Level, filterable by provider mask
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,
                    POWER_ET_ID_TYPE_LOG_TRACE_STR,
                    PowerLogTraceStr,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(PWR_TRACE_STR_SIZE), message))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_LOG_TRACE_PARAM,                // Event ID, for this provider
                     PowerLogTraceParam,                             // Event Name
                     FPFW_ET_LEVEL_DEBUG,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

 FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_CHANGE_STATE,                // Event ID, for this provider
                     PowerLogChangeState,                             // Event Name
                     FPFW_ET_LEVEL_DEBUG,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, loop_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, state))
                     

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_SIGNAL_EVT,                // Event ID, for this provider
                     PowerLogSignalEvt,                             // Event Name
                     FPFW_ET_LEVEL_DEBUG,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, event))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_PLL_ERROR,                // Event ID, for this provider
                     PowerLogPllErrorStatus,                    // Event Name
                     FPFW_ET_LEVEL_ERROR,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, core_pll_base_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pll_error_sr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, core_idx),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, wait_pll_lock_timer_exp),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, wait_fll_lock_timer_exp),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, wait_fll_cal_timer_exp),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, wait_freq_change_timer_exp),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, pllrawlockerror),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, fllrawlockerror),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_BOOL, pll_error_mask_cr))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_POWER,         // Provider ID for this event
                     POWER_ET_ID_TYPE_ALL_LOOP_METRICS,         // Event ID, for this provider
                     PowerAllLoopMetricsInUs,                       // Event Name
                     FPFW_ET_LEVEL_INFO,                        // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, ctrl_loop_min),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, ctrl_loop_max),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, vr_loop_min),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, vr_loop_max),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pvt_loop_min),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, pvt_loop_max))
#define POWER_ET_ALL_LOOP_METRICS(c_min, c_max, vr_min, vr_max, pvt_min, pvt_max) \
    EventWritePowerAllLoopMetricsInUs(c_min, c_max, vr_min, vr_max, pvt_min, pvt_max)

/*--------- Function Prototypes ----------*/
