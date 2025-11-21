//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_relay_events.c
 * Instantiates the actual event trace functions for the service.
 */

/*-------------------------------- Includes ---------------------------------*/

// Instantiates the actual event trace functions
#define FPFW_ET_IMPLEMENTATION
#define FPFW_ET_METADATA

#include <event_trace_relay_events.h>
#include <stdarg.h>
#include <stdio.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

void etr_log_ascii_info(const char* fmt, ...)
{
    static char etr_ascii_log[ETR_LOG_ASCII_STRING_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(etr_ascii_log, sizeof(etr_ascii_log), fmt, args);
    va_end(args);
    FPFW_ET_LOG(EtrInfo, etr_ascii_log);
}

void etr_log_ascii_verbose(const char* fmt, ...)
{
    static char etr_ascii_log[ETR_LOG_ASCII_STRING_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(etr_ascii_log, sizeof(etr_ascii_log), fmt, args);
    va_end(args);
    FPFW_ET_LOG(EtrVerbose, etr_ascii_log);
}

void etr_log_ascii_warn(const char* fmt, ...)
{
    static char etr_ascii_log[ETR_LOG_ASCII_STRING_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(etr_ascii_log, sizeof(etr_ascii_log), fmt, args);
    va_end(args);
    FPFW_ET_LOG(EtrWarn, etr_ascii_log);
}

void etr_log_ascii_error(const char* fmt, ...)
{
    static char etr_ascii_log[ETR_LOG_ASCII_STRING_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(etr_ascii_log, sizeof(etr_ascii_log), fmt, args);
    va_end(args);
    FPFW_ET_LOG(EtrErr, etr_ascii_log);
}