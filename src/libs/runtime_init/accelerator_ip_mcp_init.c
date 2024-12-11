/**
 * @file accelerator_ip_mcp_init.c
 * Initialize the Accelerators on MCP core
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h> // for accelerators_init
#include <fpfw_init.h>      // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <idsw.h>
#include <idsw_kng.h>
#include <stdio.h> // for printf, NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "accel_intr_clnt", "nvic"))
{
    // Initialize the Accelerators
    printf("Setup MCP to interact with accel devices.\n");
    mcp_accelerators_init();
    printf("Setup complete, MCP can interact with accel devices.\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}