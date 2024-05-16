/**
 * @file mcp_mpu_init.c
 * Instantiates MPU for the MCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>  // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>     // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO: Initialize MPU regions.
FPFW_INIT_COMPONENT(mpu)
{
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
