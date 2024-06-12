//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fuse_events.h
 * This file defines the event trace events for the fuse management
 * service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <assert.h>                 // for static_assert
#include <event_trace.h>            // for _MH_IS_PARENTHESIZED_imp1, _MH_SP...
#include <event_trace_providers.h>  // for EVENT_TRACE_PROVIDER_ID_SCP_FUSE
#include <stdint.h>                 // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/* Helper macros to log fuse events */
#define FUSE_ET_FATAL(type, status, param) EventWriteFuseFatalParam((status), (param), (type))
#define FUSE_ET_ERROR(type, param)         EventWriteFuseErrorParam((param), (type))
#define FUSE_ET_WARN(type, param)          EventWriteFuseWarnParam((param), (type))
#define FUSE_ET_STATUS(type)               EventWriteFuseStatus((type))
#define FUSE_ET_STATUS_PARAM(type, param)  EventWriteFuseStatusParam((param), (type))

/*-------------- Typedefs ----------------*/

typedef enum
{
    FUSE_ET_TYPE_UNKNOWN,
    FUSE_ET_TYPE_SVC_START,
    FUSE_ET_TYPE_MAILBOX_REQUEST_OVERRIDES,
    FUSE_ET_TYPE_RAM_DMA_COPY,
    FUSE_ET_TYPE_UNFUSED_WITH_OVERRIDES,
    FUSE_ET_TYPE_FUSED_NO_OVERRIDES,
    FUSE_ET_TYPE_FUSED_WITH_OVERRIDES,
    FUSE_ET_TYPE_FUSED_IGNORE_VALIDS,
    FUSE_ET_TYPE_OVERRIDE_COMPLETE,
    FUSE_ET_TYPE_DISTRIBUTION_START,
    FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR0,
    FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR3_MINOR1,
    FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR0,
    FUSE_ET_TYPE_DISTRIBUTION_PHASE_MAJOR4_MINOR1,
    FUSE_ET_TYPE_DISTRIBUTION_END,
    FUSE_ET_TYPE_READ,
    FUSE_ET_TYPE_COUNT
}   FUSE_ET_TYPE_T;

typedef enum {
    FUSE_ET_ID_TYPE_UNKNOWN,
    FUSE_ET_ID_TYPE_FATAL,
    FUSE_ET_ID_TYPE_ERROR,
    FUSE_ET_ID_TYPE_WARNING,
    FUSE_ET_ID_TYPE_STATUS_NOPARAMS,
    FUSE_ET_ID_TYPE_STATUS_PARAMS,
    FUSE_ET_ID_TYPE_LOG_TRACE,

    FUSE_ET_ID_TYPE_COUNT
} FUSE_ET_ID_TYPES_T;

/*-- Declarations (Statics and globals) --*/
static_assert(FUSE_ET_TYPE_COUNT <= UINT8_MAX, "Fuse ET type count exceeds space allocated in trace.");

static_assert(FUSE_ET_ID_TYPE_COUNT <= UINT8_MAX, "Fuse ET ID count exceeds space allocated in trace.");

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_FUSE,
    SocFuseModule,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR 
                    | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_FUSE,         // Provider ID for this event
                     FUSE_ET_ID_TYPE_FATAL,                    // Event ID, for this provider
                     FuseFatalParam,                           // Event Name
                     FPFW_ET_LEVEL_FATAL,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_FUSE,         // Provider ID for this event
                     FUSE_ET_ID_TYPE_ERROR,                    // Event ID, for this provider
                     FuseErrorParam,                           // Event Name
                     FPFW_ET_LEVEL_ERROR,                       // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_FUSE,         // Provider ID for this event
                     FUSE_ET_ID_TYPE_WARNING,                  // Event ID, for this provider
                     FuseWarnParam,                            // Event Name
                     FPFW_ET_LEVEL_WARNING,                     // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_FUSE,         // Provider ID for this event
                     FUSE_ET_ID_TYPE_STATUS_NOPARAMS,          // Event ID, for this provider
                     FuseStatus,                               // Event Name
                     FPFW_ET_LEVEL_INFO,                        // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_FUSE,         // Provider ID for this event
                     FUSE_ET_ID_TYPE_STATUS_PARAMS,            // Event ID, for this provider
                     FuseStatusParam,                          // Event Name
                     FPFW_ET_LEVEL_INFO,                        // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*--------- Function Prototypes ----------*/
