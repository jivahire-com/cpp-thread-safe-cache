/**
 * @file i3c_controller_scp_cli_init.c
 * Instantiates i3c_controller CLI for the SCP
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <i3c_controller_cli.h>
#include <idhw.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(i3c_cli, FPFW_INIT_DEPENDENCIES("hw_ver", "cli", "i3c_controller"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    FPFW_DBGPRINT_INFO("i3c_controller init, die_num: [%u]\n", die_num);

    i3c_controller_cli_initialize();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
