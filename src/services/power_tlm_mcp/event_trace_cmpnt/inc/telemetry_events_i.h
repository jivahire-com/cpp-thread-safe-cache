//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_events_i.h
 * Event traces for the telemetry service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                           TelemetrySvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     10,
                     PwrPkgMemoryAllocationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     11,
                     InstPkgMemoryAllocationFailed,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     14,
                     DeAllocateInvalidLocation,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     15,
                     DeAllocateQueueFreeFailed,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     16,
                     PkgCreatePwrPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, pkg_available_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     17,
                     PkgCreateInstPkgNotEnoughSpace,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, pkg_available_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     20,
                     TelemetryDataCleared,
                     FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     21,
                     MtsMgrClientFreeFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, block_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     22,
                     MtsMgrClientQueueFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, queue),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     23,
                     MtsMgrClientUnexpectedDcpMsg,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     24,
                     MtsMgrInvalidEventEnableDisable,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, provider_id),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, event_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     25,
                     DTSCoefficientReadFailedInit,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     26,
                     DIMMInfoInvalidDimmId,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     27,
                     DataPackagePWRrecordError,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, element_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     28,
                     DataPackageInstRecordError,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, element_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     29,
                     MtsMgrPkgFreeListEmpty,
                     FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    30,
                    TlmSvcDebugIncomingDcpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, seq_num))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    32,
                    TlmSvcDebugIncomingTrpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, source_seq_num),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, mts_client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT16, trp_msg_status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, init_cmd))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    33,
                    TlmSvcDebugOutgoingTrpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, source_seq_num),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, mts_client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT16, trp_msg_status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, init_cmd))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    34,
                    MtsMgrNoMatchPkgComplete,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, ddr_addr_offset))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    35,
                    MtsMgrClientUnexpectedTrpMsg,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    38,
                    DdrMgrDbgAllocatePwrPkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    39,
                    DdrMgrDbgAllocateInstPkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    40,
                    DdrMgrDbgFreePkgMem,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_location))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    41,
                    MtsMgrAddPkgToActiveList,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_core_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    42,
                    MtsMgrRemovePkgFromActive,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, atu_mapped_location),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_core_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    43,
                    MtsMgrPackagesFlushed,
                    FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    44,
                    InvalidRackThrottlePriority,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, priority))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    45,
                    InvalidTerminateThrottleTimeStamp,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, new_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, prev_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_source))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    47,
                    LogCstateValidTimeStamp,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, new_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, prev_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, cstate))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    48,
                    CstateUnexpectedLevelChange,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, new_state),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, last_state))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                     49,
                     ExecInvalidModeChange,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, current_mode),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, new_mode))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    50,
                    ExecTlmSvcModeChange,
                    FPFW_ET_LEVEL_INFO,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, current_mode),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, new_mode))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    51,
                    ExecInvalidTimerPeriod,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, data_aggr_period_ms),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, inst_pkg_sample_period_ms),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, pwr_pkg_period_ms),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, twenty_four_hr_pkg_period_ms))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    52,
                    MtsMgrClientUnexpectedCmd,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, cmd))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    53,
                    MtsMgrInitDDR,
                    FPFW_ET_LEVEL_INFO)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    54,
                    TimerChangeFail,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, tmr_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    55,
                    LogInValidPstateTimeStamp,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, new_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, prev_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, pstate))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    56,
                    ThrottleEndInvalidTimeStamp,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, new_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, prev_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, throttle_source))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    57,
                    RackThrottleEndInvalidTimeStamp,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, new_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, prev_timestamp_uS),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, priority))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    58,
                    LogInvalidTileId,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    59,
                    LogInvalidCoreId,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    60,
                    GtimerIsZero,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    61,
                    CumulativeAverageNullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    62,
                    RunningAverageNullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    63,
                    RunningAvgAddSampleNumSat,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, instance))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    64,
                    MMAU32NullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    65,
                    MeanofMeansHighCount,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, value))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    66,
                    MMAU16NullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    68,
                    InvalidDieId,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, die_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    69,
                    LogInValidCstateId,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, value))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    73,
                    MeanofSummationHighCount,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, value))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    74,
                    MovingAvgParamError,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    75,
                    UnexpectedSensorId,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, expected),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, actual))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    76,
                    OobNoActiveHandler,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    77,
                    OobSensorAlreadyActive,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, active_sensor),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, active_handler))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    78,
                    DataUtilCalcError,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    79,
                    RunningAvgDurResetNullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    80,
                    RunningAvgDurUpdateNullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    81,
                    RunningAvgDurGetNullPointer,
                    FPFW_ET_LEVEL_ERROR)

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    82,
                    AvgAddSampleSumSat,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, instance))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    83,
                    AvgAddSampleNumSat,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, instance))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_TLM_SERVICE,
                    84,
                    AvgGetOutputSat,
                    FPFW_ET_LEVEL_ERROR,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, instance))

/*--------- Function Prototypes ----------*/

