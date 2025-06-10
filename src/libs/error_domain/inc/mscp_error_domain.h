//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mscp_error_domain.h
 * Header file of error domain
 */
#pragma once
#include <cper.h>
#include <fpfw_icc_base.h>

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

#if defined (SCP_RUNTIME_INIT)
/*--------- Function Prototypes ----------*/
/**
 * @brief Register the SCP error domain.
 */
void register_scp_error_domain();

#elif defined (MCP_RUNTIME_INIT)
/**
 * @brief Register the MCP error domain.
 */
void register_mcp_error_domain(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief start ICC listener for MCP error injection request
 */
void start_mcp_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief Get MHU handle
 *  
 * @return
 *      pointer to handle.
 */
fpfw_icc_base_ctx_t* get_mhu_handle(void);
#endif