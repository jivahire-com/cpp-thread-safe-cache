//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_events.h
 * This file defines the event trace events for the ddr management
 * service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <assert.h>                 // for static_assert
#include <event_trace.h>            // for _MH_IS_PARENTHESIZED_imp1, _MH_SP...
#include <event_trace_providers.h>  // for EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER
#include <stdint.h>                 // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/* Helper macros to log ddr manager events */
#define DDR_MANAGER_ET_FATAL(type, status, param) EventWriteDDRFatal((status), (param), (type))
#define DDR_MANAGER_ET_ERROR(type, param)         EventWriteDDRErrorParam((param), (type))
#define DDR_MANAGER_ET_WARN(type, param)          EventWriteDDRWarnParam((param), (type))
#define DDR_MANAGER_ET_STATUS(type)               EventWriteDDRStatus((type))
#define DDR_MANAGER_ET_STATUS_PARAM(type, param)  EventWriteDDRStatusParam((param), (type))
#define DDR_MANAGER_ET_VERBOSE(type)              EventWriteDDRVerbose((type))
#define DDR_MANAGER_ET_VERBOSE_PARAM(type, param) EventWriteDDRVerboseParam((param), (type))

// DEBUG events are not included in a production build
#define DDR_MANAGER_ET_DEBUG(type)                EventWriteDDRDebug((type))
#define DDR_MANAGER_ET_DEBUG_PARAM(type, param)   EventWriteDDRDebugParam((param), (type))

#define ET_NOPARAM 0

/*-------------- Typedefs ----------------*/

typedef enum
{
    DDR_MANAGER_ET_TYPE_QUEUE_CREATE_ERROR,                                // 0
    DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_CREATE_BDAT,                      // 1
    DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_CREATE_SMBIOS_TABLES,             // 2
    DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_COPY_PRM_CONFIG,                  // 3
    DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_START_POLLING_TIMER,              // 4
    DDR_MANAGER_ET_TYPE_THREAD_CREATE_ERROR,                               // 5
    DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR,                                // 6
    DDR_MANAGER_ET_TYPE_NO_HSP_DETECTED,                                   // 7
    DDR_MANAGER_ET_TYPE_TX_QUEUE_RECEIVE,                                  // 8
    DDR_MANAGER_ET_TYPE_UNKNOWN_MESSAGE_TYPE,                              // 9
    DDR_MANAGER_ET_TYPE_Q_MESSAGE_RECEIVED,                                // 10
    DDR_MANAGER_ET_TYPE_DDR_MESSAGE_TO_HSP_SENT,                           // 11
    DDR_MANAGER_ET_TYPE_SENDING_DDR_MESSAGE_TO_HSP,                        // 12
    DDR_MANAGER_ET_TYPE_PLATFORM_NOT_SUPPORT_I3C_POLLING,                  // 13
    DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_0,                         // 14
    DDR_MANAGER_ET_TYPE_READ_TEMPERATURE_SENSOR_1,                         // 15
    DDR_MANAGER_ET_TYPE_DIMM_EXCEEDED_CRITICAL_TEMPERATURE_THRESHOLD,      // 16
    DDR_MANAGER_ET_TYPE_DIMM_TEMPERATURES_EXCEED_HIGH_THRESHOLD_BWL_DISABLE, // 17
    DDR_MANAGER_ET_TYPE_CREATE_MMAP,                                       // 18
    DDR_MANAGER_ET_TYPE_ADD_RESERVED_RANGE_TO_MEMORY_MAP,                  // 19
    DDR_MANAGER_ET_TYPE_USING_SDS_STRUCTURE,                               // 20
    DDR_MANAGER_ET_TYPE_READ_MEMORY_MAP_SIZE,                              // 21
    DDR_MANAGER_ET_TYPE_NUMBER_OF_MEMORY_REGION,                           // 22
    DDR_MANAGER_ET_TYPE_SORTING_RESERVED_MEMORY_MAP,                       // 23
    DDR_MANAGER_ET_TYPE_UNEXPECTED_RESERVATION_OVERLAP,                    // 24
    DDR_MANAGER_ET_TYPE_INSERTING_RESERVED_MEMORY_MAP,                     // 25
    DDR_MANAGER_ET_TYPE_RESERVED_ADDRESS_RANGE_NOT_TERMINATED,             // 26
    DDR_MANAGER_ET_TYPE_MEMORY_REGIONS_EXCEEDS_EXPECTED_SIZE,              // 27
    DDR_MANAGER_ET_TYPE_ICC_NULL_POINTER,                                  // 28
    DDR_MANAGER_ET_TYPE_SEND_REQUEST_PHY_BIN,                              // 29
    DDR_MANAGER_ET_TYPE_PHY_BIN_LOAD_COMPLETE,                             // 30
    DDR_MANAGER_ET_TYPE_TEMPERATURE_SENSOR_MR49_READ,                      // 31
    DDR_MANAGER_ET_TYPE_TEMPERATURE_SENSOR_MR50_READ,                      // 32
    DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_I3C,                                // 33
    DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_I3C,                               // 34
    DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_MR4,                                // 35
    DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_MR4,                               // 36
    DDR_MANAGER_ET_TYPE_BWL_FORCED_ENABLE,                                 // 37
    DDR_MANAGER_ET_TYPE_BWL_FORCED_DISABLE,                                // 38
    DDR_MANAGER_ET_TYPE_PMIC_POWER_READ,                                   // 39
    DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_START_RH_TLM_SERVICE,             // 40
    DDR_MANAGER_ET_TYPE_SDL_NOT_FOUND,                                     // 41
    DDR_MANAGER_ET_TYPE_SDL_CREATED_EMPTY_SDL_HEADER,                      // 42
    DDR_MANAGER_ET_TYPE_SDL_FOUND_AND_LOADED_TO_DDR,                       // 43
    DDR_MANAGER_ET_TYPE_SDL_SKIPPED_CFG_KNOB_DISABLED,                     // 44
    DDR_MANAGER_ET_TYPE_SDL_COPY_TO_DDR_FAILED,                            // 45
    DDR_MANAGER_ET_TYPE_SDL_ADDRESS_PUBLISHING_SKIPPED,                    // 46
    DDR_MANAGER_ET_TYPE_WARM_START_DETECTED,                               // 47
    DDR_MANAGER_ET_TYPE_DDR_INIT_DURATION_MS,                              // 48
    DDR_MANAGER_ET_TYPE_DDR_TRAINING_TIME_MS,                              // 49
    DDR_MANAGER_ET_TYPE_DFWK_DISPATCH_REQUEST,                             // 50
    DDR_MANAGER_ET_TYPE_SDL_STORE_FAILED,                                  // 51
    DDR_MANAGER_ET_TYPE_SDL_STORED_TO_FLASH,                               // 52
    DDR_MANAGER_ET_TYPE_PPR_GET_VARIABLE,                                  // 53
    DDR_MANAGER_ET_TYPE_PPR_GET_VARIABLE_FAIL,                             // 54
    DDR_MANAGER_ET_TYPE_PPR_VARIABLE_FOUND,                                // 55
    DDR_MANAGER_ET_TYPE_PPR_VARIABLE_RESET,                                // 56
    DDR_MANAGER_ET_TYPE_PPR_HPPR,                                          // 57
    DDR_MANAGER_ET_TYPE_PPR_SPPR,                                          // 58
    DDR_MANAGER_ET_TYPE_PPR_MPPR,                                          // 59
    DDR_MANAGER_ET_TYPE_PPR_INVALID_TYPE,                                  // 60
    DDR_MANAGER_ET_TYPE_PPR_SYNC_DO_SEND_D1,                               // 61
    DDR_MANAGER_ET_TYPE_PPR_SYNC_D1_RECV_D0,                               // 62
    DDR_MANAGER_ET_TYPE_PPR_RESULT_DIMM_IDX_0_BASED,                       // 63
    DDR_MANAGER_ET_TYPE_PPR_RESULT_STATUS,                                 // 64
    DDR_MANAGER_ET_TYPE_PPR_SETUP_BEGIN,                                   // 65
    DDR_MANAGER_ET_TYPE_PPR_SETUP_END,                                     // 66
    DDR_MANAGER_ET_TYPE_PPR_SETUP_SKIPPED_SINGLE_DIE,                      // 67
    DDR_MANAGER_ET_TYPE_COUNT                                              // 68
}   DDR_MANAGER_ET_TYPE_T;

typedef enum {
    DDR_MANAGER_ET_ID_TYPE_FATAL,
    DDR_MANAGER_ET_ID_TYPE_ERROR,
    DDR_MANAGER_ET_ID_TYPE_WARNING,
    DDR_MANAGER_ET_ID_TYPE_STATUS_NOPARAMS,
    DDR_MANAGER_ET_ID_TYPE_STATUS_PARAMS,
    DDR_MANAGER_ET_ID_TYPE_VERBOSE_NOPARAMS,
    DDR_MANAGER_ET_ID_TYPE_VERBOSE_PARAMS,
    DDR_MANAGER_ET_ID_TYPE_DEBUG_NOPARAMS,
    DDR_MANAGER_ET_ID_TYPE_DEBUG_PARAMS,

    DDR_MANAGER_ET_ID_TYPE_COUNT
} DDR_MANAGER_ET_ID_TYPES_T;

static_assert(DDR_MANAGER_ET_TYPE_COUNT <= UINT8_MAX, "ddr manager ET type count exceeds space allocated in trace.");
static_assert(DDR_MANAGER_ET_ID_TYPE_COUNT <= UINT8_MAX, "ddr manager ET ID count exceeds space allocated in trace.");

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,
    SocDDRModule,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR
                    | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_FATAL,                      // Event ID, for this provider
                     DDRFatal,                                          // Event Name
                     FPFW_ET_LEVEL_FATAL,                               // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_ERROR,                      // Event ID, for this provider
                     DDRErrorParam,                                     // Event Name
                     FPFW_ET_LEVEL_ERROR,                               // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_WARNING,                    // Event ID, for this provider
                     DDRWarnParam,                                      // Event Name
                     FPFW_ET_LEVEL_WARNING,                             // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_STATUS_NOPARAMS,            // Event ID, for this provider
                     DDRStatus,                                         // Event Name
                     FPFW_ET_LEVEL_INFO,                                // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_STATUS_PARAMS,              // Event ID, for this provider
                     DDRStatusParam,                                    // Event Name
                     FPFW_ET_LEVEL_INFO,                                // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_VERBOSE_NOPARAMS,           // Event ID, for this provider
                     DDRVerbose,                                        // Event Name
                     FPFW_ET_LEVEL_VERBOSE,                             // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_VERBOSE_PARAMS,             // Event ID, for this provider
                     DDRVerboseParam,                                   // Event Name
                     FPFW_ET_LEVEL_VERBOSE,                             // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

// DEBUG events are not included in a production build
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_DEBUG_NOPARAMS,             // Event ID, for this provider
                     DDRDebug,                                          // Event Name
                     FPFW_ET_LEVEL_DEBUG,                               // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

// DEBUG events are not included in a production build
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_DDR_MANAGER,           // Provider ID for this event
                     DDR_MANAGER_ET_ID_TYPE_DEBUG_PARAMS,               // Event ID, for this provider
                     DDRDebugParam,                                     // Event Name
                     FPFW_ET_LEVEL_DEBUG,                               // Event Log Level, filterable by provider mask
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, param),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, type))

/*--------- Function Prototypes ----------*/
