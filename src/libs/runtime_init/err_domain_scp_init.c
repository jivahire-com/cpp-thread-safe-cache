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
FPFW_INIT_COMPONENT(scp_ras, FPFW_INIT_DEPENDENCIES("hm_svc"))
{
    register_scp_error_domain();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}