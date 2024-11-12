/**
 * @file system_info_init.c
 * Instantiates system info for the MCP & SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <system_info.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(sysinfo, FPFW_INIT_NULL_NODE)
{
    system_info_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
