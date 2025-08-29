//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm.h
 * Power pldm request processing for mcp core
 */

/*------------- Includes -----------------*/
#include <fpfw_icc_base.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Initialize the power PLDM service on MCP, no effect on SCP.
 *
 * @param icc_ctx Pointer to the ICC context. Uses ICC mhu mscp2mscp ctx.
 */
void power_pldm_init(fpfw_icc_base_ctx_t* icc_ctx);