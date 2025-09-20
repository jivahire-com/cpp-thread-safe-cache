//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file err_domain_scp_init.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <error_domain_gic.h>
#include <error_domain_smmu.h>
#include <fpfw_init.h>
#include <mscp_error_domain.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
FPFW_INIT_COMPONENT(scp_ras, FPFW_INIT_DEPENDENCIES("hm_svc", "nvic", "icc_mscp2mscp"))
{
    mcp_error_injection_setup_listener(fpfw_init_get_handle("icc_mscp2mscp"));
    register_scp_error_domain(fpfw_init_get_handle("icc_mscp2mscp"));
    register_smmu_error_domain();
    register_gic_error_domain();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}