/**
 * @file et_init.c
 * Instantiates Event Tracing
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <scp_event_trace_collector.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(et_init, FPFW_INIT_DEPENDENCIES("uart_bm"))
{
    scp_etc_initialize();
    printf("SCP ET initialized");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}