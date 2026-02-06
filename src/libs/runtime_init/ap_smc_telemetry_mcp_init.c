//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ap_smc_telemetry_mcp_init.c
 * Instantiates AP SMC Telemetry for MCP core
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <fpfw_init.h>
#include <smc_tlm_mcp.h> // for smc_tlm_mcp_init
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(smc_tlm_mcp, FPFW_INIT_DEPENDENCIES("debug_print"))
{
    smc_tlm_mcp_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
