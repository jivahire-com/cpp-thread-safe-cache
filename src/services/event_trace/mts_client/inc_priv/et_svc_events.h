//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file et_svc_events.h
 * Defines Event Defintions for the Event Trace Service.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <event_trace.h>
#include <event_trace_providers.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define LOG_DEFAULT_ASCII_STR_SIZE 128

#define EVENT_TRACE_PROVIDER_ID_EVENT_TRACE_SERVICE EVENT_TRACE_PROVIDER_ID_COMMON_EVENT_TRACE_SERVICE
#define ET_LOG_ET_SVC(event, ...) \
snprintf(s_et_svc_message, LOG_DEFAULT_ASCII_STR_SIZE, __VA_ARGS__);   \
    FPFW_ET_LOG(event, s_et_svc_message);

/*-------------------------------- Typedefs ---------------------------------*/

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_EVENT_TRACE_SERVICE, // ID
    ET_SERVICE,                                   // Name
    ET_LEVEL_MASK_ALL                             // Logging Level Mask
)

/**
 * Define Event Trace events for the Event Trace Service Provider. 
*/
typedef enum {
    E_EVENT_TRACE_SERVICE_EVENT_INIT_INFO = 0, 
    E_EVENT_TRACE_SERVICE_EVENT_INIT_WARNING,
    E_EVENT_TRACE_SERVICE_EVENT_INIT_ERROR,
    E_EVENT_TRACE_SERVICE_EVENT_ID_MAX
} e_event_trace_service_event_id_t;

/*------------------- Declarations (Statics and globals) --------------------*/

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_EVENT_TRACE_SERVICE,
    E_EVENT_TRACE_SERVICE_EVENT_INIT_INFO,
    ETInfo,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(LOG_DEFAULT_ASCII_STR_SIZE), Log)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_EVENT_TRACE_SERVICE,
    E_EVENT_TRACE_SERVICE_EVENT_INIT_WARNING,
    ETWarn,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(LOG_DEFAULT_ASCII_STR_SIZE), Log)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_EVENT_TRACE_SERVICE,
    E_EVENT_TRACE_SERVICE_EVENT_INIT_ERROR,
    ETErr,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(LOG_DEFAULT_ASCII_STR_SIZE), Log)
)

/*--------------------------- Function Prototypes ---------------------------*/
