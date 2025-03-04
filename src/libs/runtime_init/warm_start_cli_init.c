/**
 * @file warn_start_cli_init.c
 * Instantiates Warm Start CLIs for the SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <startup_shutdown_init.h>
#include <stdint.h>
#include <stdio.h>
#include <warm_start_cli.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(ws_cli_init, FPFW_INIT_DEPENDENCIES("cli", "ws_init", "sos_int", "sos_svc"))
{
    warm_start_cli_init(fpfw_init_get_handle("sos_svc"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}