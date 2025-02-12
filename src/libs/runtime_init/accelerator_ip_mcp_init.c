/**
 * @file accelerator_ip_mcp_init.c
 * Initialize the Accelerators on MCP core
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h> // for accelerators_init
#include <atu_init.h>
#include <fpfw_init.h>      // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <idsw.h>
#include <idsw_kng.h>
#include <stdio.h> // for printf, NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("hw_ver", "accel_intr_clnt", "nvic", "accel_atu"))
{
    // Initialize the Accelerators
    mcp_accelerators_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(accel_atu, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "nvic"))
{
    // Initialize the Accelerators
    accel_atu_config();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
