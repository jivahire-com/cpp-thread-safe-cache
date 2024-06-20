/**
 * @file avs_scp_cli_init.c
 * Instantiates AVS CLIfor the SCP
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <scp_avs.h>
#include <scp_avs_cli.h>
#include <scp_avs_driver.h>
#include <scp_interrupts.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(avs_cli, FPFW_INIT_DEPENDENCIES("cli", "avs0_int"))
{
    scp_avs_interface_t* avs0_int = (scp_avs_interface_t*)fpfw_init_get_handle("avs0_int");

    // eventually do this for all AVS
    scp_avs_cli_initialize(avs0_int);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
