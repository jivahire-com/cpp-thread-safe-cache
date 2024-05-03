/**
 * @file etc_scp_init.c
 * Instantiates Event Tracing Collector for the SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <scp_event_trace_collector.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(etc_init, FPFW_INIT_DEPENDENCIES("std_io"))
{
    scp_etc_initialize();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
