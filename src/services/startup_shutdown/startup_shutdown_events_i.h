//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_events_i.h
 * Event traces for the startup and shutdown service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>

/*-- Symbolic Constant Macros (defines) --*/

#define SOS_ET_INFO_PARAM(stage, type, boot_type)    EventWriteSosInfoParam((stage), (type), (boot_type))
#define SSI_ET_INFO_PARAM(type, shutdown_type)   EventWriteSsiInfoParam((type), (shutdown_type))
#define LOCAL_CORE_ET_START_INFO_PARAM(local_stage)   EventWriteLocalCoreSyncStageStart((local_stage))
#define REMOTE_CORE_ET_END_INFO_PARAM(local_stage, iteration)   EventWriteLocalCoreSyncStageRemoteEnd((local_stage), (iteration))
#define REMOTE_CORE_ET_READ_INFO_PARAM(local_stage, remote_stage, iteration)   EventWriteLocalCoreSyncStageRemoteRead((local_stage), (remote_stage), (iteration))
#define SOS_ET_BOOT_PROFILE_INFO(sos_phase, type, operation) EventWriteSoSBootProfileInfo((sos_phase), (type), (operation))

// clang-format off

/**
 * Define Event Trace events for the SOS provider
*/
typedef enum {
    SOS_ET_ID_TYPE_INFO_PARAMS,
    SSI_ET_ID_TYPE_INFO_PARAMS,
    LOCAL_CORE_SYNC_STAGE_PARAM,
    LOCAL_CORE_SYNC_STAGE_REMOTE_PARAM,
    SOS_ET_ID_TYPE_BOOT_PROFILE_INFO,
    LOCAL_CORE_SYNC_STAGE_PARAM_VERBOSE = 0x0A,
    
    SOS_ET_ID_TYPE_COUNT
} SOS_EVENT_ID;
typedef enum
{
    SOS_ET_TYPE_LOCAL_CORE_SYNC_START,
    SOS_ET_TYPE_LOCAL_CORE_SYNC_REMOTE_END,
    SOS_ET_TYPE_LOCAL_CORE_SYNC_REMOTE_READ,
    SOS_ET_TYPE_SSI_STARTUP_STAGE_START_ASYNC,
    SOS_ET_TYPE_SSI_STARTUP_STAGE_COMPLETE_ASYNC,
    SOS_ET_TYPE_SSI_SHUTDOWN_ASYNC,
    SOS_ET_TYPE_SSI_SHUTDOWN_QUIESCE_ASYNC,
    SOS_ET_TYPE_BOOT_OPERATION_START,
    SOS_ET_TYPE_BOOT_OPERATION_COMPLETE,
    
    SOS_ET_TYPE_COUNT
} SOS_ET_TYPE_T;

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                           StartupShutdownSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

//
// Info Events
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     SOS_ET_ID_TYPE_INFO_PARAMS,
                     SosInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, boot_type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     SSI_ET_ID_TYPE_INFO_PARAMS,
                     SsiInfoParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, shutdown_type),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     LOCAL_CORE_SYNC_STAGE_PARAM,
                     LocalCoreSyncStageStart,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     LOCAL_CORE_SYNC_STAGE_REMOTE_PARAM,
                     LocalCoreSyncStageRemoteEnd,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, iterations))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     SOS_ET_ID_TYPE_BOOT_PROFILE_INFO, 
                     SoSBootProfileInfo,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, sos_phase), 
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, type), 
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, operation))

//
// Verbose Events - Offset by 10
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_STARTUP_SHUTDOWN,
                     LOCAL_CORE_SYNC_STAGE_PARAM_VERBOSE,
                     LocalCoreSyncStageRemoteRead,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, local_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, remote_stage),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, iteration))

/*--------- Function Prototypes ----------*/
