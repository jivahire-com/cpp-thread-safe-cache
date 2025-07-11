//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file err_domain_scp_init.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <mscp_error_domain.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
FPFW_INIT_COMPONENT(mcp_ras, FPFW_INIT_DEPENDENCIES("icc_mscp2mscp", "hm_svc"))
{
    register_mcp_error_domain(fpfw_init_get_handle("icc_mscp2mscp"));
    start_mcp_error_injection_listener(fpfw_init_get_handle("icc_mscp2mscp"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}