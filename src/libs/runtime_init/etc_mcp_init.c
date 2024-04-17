/**
 * @file etc_mcp_init.c
 * Instantiates Event Tracing Collector for the MCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <mcp_event_trace_collector.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(etc_init, FPFW_INIT_DEPENDENCIES("std_io"))
{
    mcp_etc_initialize();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}