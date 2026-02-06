//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file smc_tlm_mcp.h
 * SMC telemetry request processing for MCP core
 */

/*------------- Includes -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Initialize the SMC telemetry service on MCP, no effect on SCP.
 *
 */
void smc_tlm_mcp_init(void);
