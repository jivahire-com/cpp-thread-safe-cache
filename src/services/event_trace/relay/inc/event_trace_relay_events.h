//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_relay_events.h
 * Private event trace functions for the Event Trace Relay service.
 */

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace.h>
#include <event_trace_providers.h>
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define ETR_LOG_ASCII_STRING_SIZE (32)

#define FPFW_ET_LOG_ETR_ASCII_INFO    etr_log_ascii_info
#define FPFW_ET_LOG_ETR_ASCII_WARN    etr_log_ascii_warn
#define FPFW_ET_LOG_ETR_ASCII_ERROR   etr_log_ascii_error
#define FPFW_ET_LOG_ETR_ASCII_VERBOSE etr_log_ascii_verbose

/*-------------------------------- Typedefs ---------------------------------*/
typedef enum
{
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_INFO,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_WARNING,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_ERROR,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_VERBOSE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_LOG_HSP_BUFFER_PROC,
    ETR_EVENT_ID_MCP_ETR_SERVICE_LOG_HOST_CMD,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ERROR,
    ETR_EVENT_ID_MCP_ETR_SERVICE_INFO,
    ETR_EVENT_ID_MCP_ETR_SERVICE_DELAYED_HOST_READS,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASIC_BUFFERS_REUSED,
    ETR_EVENT_ID_MCP_ETR_SERVICE_HSP_BUFFERS_DROPPED,
    ETR_EVENT_ID_MCP_ETR_SERVICE_MAX
} ETR_EVENT_ID_MCP;

/*------------------- Declarations (Statics and globals) --------------------*/

FPFW_ET_DEFINE_PROVIDER_EX(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR,
    FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
    FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
                     ETR_EVENT_ID_MCP_ETR_SERVICE_LOG_HSP_BUFFER_PROC,
                     HspBufferProc,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, HdrVer),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, MsgVer),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, PlatId),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, ManSize),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, LogSize))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
                     ETR_EVENT_ID_MCP_ETR_SERVICE_LOG_HOST_CMD,
                     HostCmd,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, AddrOffset),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, Size),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, Crc),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(8), BufferStatus))

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_VERBOSE,
    EtrVerbose,
    FPFW_ET_LEVEL_VERBOSE,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(ETR_LOG_ASCII_STRING_SIZE), Log)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_INFO,
    EtrInfo,
    FPFW_ET_LEVEL_INFO,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(ETR_LOG_ASCII_STRING_SIZE), Log)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_WARNING,
    EtrWarn,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(ETR_LOG_ASCII_STRING_SIZE), Log)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASCII_LOG_ERROR,
    EtrErr,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_ASCII_STRING(ETR_LOG_ASCII_STRING_SIZE), Log)
)

typedef enum
{
    ETR_ERR_INVALID_TRP_MSG,
    ETR_ERR_INVALID_DCP_MSG,
    ETR_ERR_INVALID_ETC_BUFFER_CORE,
    ETR_ERR_INVALID_ETC_BUFFER_SIZE,
    ETR_ERROR_MAX
} E_ETR_ERROR_TYPE;

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ERROR,
    Error,
    FPFW_ET_LEVEL_ERROR,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, Die),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, Core),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT8, Error),
    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT8, ErrorParam)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_DELAYED_HOST_READS,
    DelHostReads,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, DelayedHostReads)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_ASIC_BUFFERS_REUSED,
    AsicBufReused,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, AsicBuffersReused)
)

FPFW_ET_DEFINE_EVENT(
    EVENT_TRACE_PROVIDER_ID_MCP_ETR_SERVICE,
    ETR_EVENT_ID_MCP_ETR_SERVICE_HSP_BUFFERS_DROPPED,
    HSPBufDropped,
    FPFW_ET_LEVEL_WARNING,
    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT64, HSPBuffersDropped)
)

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Logs an ASCII info message.
 * 
 * @param fmt 
 * @param ... 
 * @return None
 */
void etr_log_ascii_info(const char* fmt, ...) ;

/**
 * @brief Logs an ASCII verbose message.
 * 
 * @param fmt 
 * @param ...
 * @return None
 */
void etr_log_ascii_verbose(const char* fmt, ...) ;

/**
 * @brief Logs an ASCII warning message.
 * 
 * @param fmt 
 * @param ...
 * @return None
 */
void etr_log_ascii_warn(const char* fmt, ...) ;

/**
 * @brief Logs an ASCII error message.
 * 
 * @param fmt 
 * @param ...
 * @return None
 */
void etr_log_ascii_error(const char* fmt, ...) ;
