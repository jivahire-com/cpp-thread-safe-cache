/**
 * @file cli_power_init.c
 * Instantiates power cli for SCP
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <cli_power.h>
#include <fpfw_init.h> 
#include <stddef.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli_pwr, FPFW_INIT_DEPENDENCIES("cli", "pwr_svc"))
{
    FPFW_CLI_STATUS status = cli_power_init();
    return (fpfw_init_result_t){status, NULL};
}