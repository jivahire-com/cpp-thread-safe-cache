/**
 * @file mesh_scp_cli_init.c
 * Instantiates Mesh CLI for the SCP
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <mesh_cli.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(mesh_cli, FPFW_INIT_DEPENDENCIES("hw_ver", "cli", "mesh"))
{

    mesh_cli_initialize();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
