/**
 * @file scmi_cli_init.c
 * Initialize the scmi cli library.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <scmi_cli.h>   // for scmi_cli_init
#include <stddef.h>    // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(scmi_cli, FPFW_INIT_DEPENDENCIES("cli", "std_io"))
{
    scmi_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
