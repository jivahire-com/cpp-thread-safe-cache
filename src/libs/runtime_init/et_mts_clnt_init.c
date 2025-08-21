/**
 * @file et_mts_clnt_init.c
 * Instantiates the Event Trace MTS client on SCP core.
 */

/*------------- Includes -----------------*/

#include <et_mts_client.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW...
#include <stddef.h>    // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(et_mts_clnt, FPFW_INIT_DEPENDENCIES("mts_svc", "etc", "etd"))
{
    event_trace_mts_client_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
