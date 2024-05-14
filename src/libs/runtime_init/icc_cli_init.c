/**
 * @file icc_cli_init.c
 * Initialize the icc cli library.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <icc_cli.h>   // for icc_cli_init
#include <stddef.h>    // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(icc_cli, FPFW_INIT_DEPENDENCIES("cli", "std_io"))
{
    icc_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
