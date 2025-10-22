//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_events.h
 * Defines Crash Dump Trace Provider and events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <event_trace.h>
#include <event_trace_providers.h>

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_ET_INFO(type)                 EventWriteCrashDumpInfoNoParam((type))
#define CRASH_DUMP_ET_INFO_PARAM(type, param)    EventWriteCrashDumpInfoParam((param), (type))
#define CRASH_DUMP_ET_ERROR(type)                EventWriteCrashDumpErrorNoParam((type))
#define CRASH_DUMP_ET_ERROR_PARAM(type, param)   EventWriteCrashDumpErrorParam((param), (type))
#define CRASH_DUMP_ET_WARNING(type)              EventWriteCrashDumpWarningNoParam((type))
#define CRASH_DUMP_ET_WARNING_PARAM(type, param) EventWriteCrashDumpWarningParam((param), (type))

// clang-format off

/**
 * Define Event Trace events for the Crash Dump Provider
*/
typedef enum {
    CRASH_DUMP_ET_ID_TYPE_INFO_NOPARAMS,
    CRASH_DUMP_ET_ID_TYPE_INFO_PARAMS,
    CRASH_DUMP_ET_ID_TYPE_ERROR_NOPARAMS,
    CRASH_DUMP_ET_ID_TYPE_ERROR_PARAMS,
    CRASH_DUMP_ET_ID_TYPE_WARNING_NOPARAMS,
    CRASH_DUMP_ET_ID_TYPE_WARNING_PARAMS,

    CRASH_DUMP_ET_ID_TYPE_COUNT

} CRASH_DUMP_EVENT_ID;

typedef enum
{
    CRASH_DUMP_ET_TYPE_CD_CRASH,
    CRASH_DUMP_ET_TYPE_CD_HSP_RESET,
    CRASH_DUMP_ET_TYPE_ACCEL_DDR_COPY_START,
    CRASH_DUMP_ET_TYPE_ACCEL_INVALID_SIZE,
    CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS,
    CRASH_DUMP_ET_TYPE_ACCEL_TRANSFER_FAILED,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_INCOMPLETE,
    CRASH_DUMP_ET_TYPE_ACCEL_MAGIC_NUMBER_UNMATCHED,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_SKIP,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_START,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_DONE,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_FAILED,
    CRASH_DUMP_ET_TYPE_ACCEL_CD_RETRY,
    CRASH_DUMP_ET_TYPE_ACCEL_DEFAULT_CD_GEN,
    CRASH_DUMP_ET_TYPE_ACCEL_DDR_COPY_DONE,
    CRASH_DUMP_ET_TYPE_CLI_INVALID_PARAMS,
    CRASH_DUMP_ET_TYPE_CLI_EXCEPTION,
    CRASH_DUMP_ET_TYPE_ICC_INVALID_PARAMS,
    CRASH_DUMP_ET_TYPE_ICC_INVALID_ADDRESS,
    CRASH_DUMP_ET_TYPE_ICC_SEND_ERROR,
    CRASH_DUMP_ET_TYPE_INIT_INVALID_ADDRESS,
    CRASH_DUMP_ET_TYPE_STATUS_INVALID_PARAMS,
    CRASH_DUMP_ET_TYPE_STATUS_WARM_START,
    CRASH_DUMP_ET_TYPE_PLDM_READY,
    CRASH_DUMP_ET_TYPE_PLDM_EMPTY_DUMP,
    CRASH_DUMP_ET_TYPE_PLDM_START_TRANSFER,
    CRASH_DUMP_ET_TYPE_PLDM_START_TRANSFER_ERROR,
    CRASH_DUMP_ET_TYPE_PLDM_TRANSFER,
    CRASH_DUMP_ET_TYPE_PLDM_TRANSFER_ERROR,
    CRASH_DUMP_ET_TYPE_PLDM_TRANSFER_COMPLETE,
    CRASH_DUMP_ET_TYPE_STREAM_ATU_ERROR,
    CRASH_DUMP_ET_TYPE_STREAM_OPEN,
    CRASH_DUMP_ET_TYPE_STREAM_OPEN_EMPTY,
    CRASH_DUMP_ET_TYPE_STREAM_OPEN_ERROR,
    CRASH_DUMP_ET_TYPE_STREAM_READ_WARNING,


    CRASH_DUMP_ET_TYPE_COUNT
}   CRASH_DUMP_ET_TYPE_T;

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
    CrashDump_ET,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING |
    FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL |
    FPFW_ET_LEVEL_MASK_ALWAYS
);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_INFO_NOPARAMS,
                     CrashDumpInfoNoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_INFO_PARAMS,
                     CrashDumpInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_ERROR_NOPARAMS,
                     CrashDumpErrorNoParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_ERROR_PARAMS,
                     CrashDumpErrorParam,
                     FPFW_ET_LEVEL_ERROR,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_WARNING_NOPARAMS,
                     CrashDumpWarningNoParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_CRASH_DUMP,
                     CRASH_DUMP_ET_ID_TYPE_WARNING_PARAMS,
                     CrashDumpWarningParam,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

// clang-format on
