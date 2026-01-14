//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file power_pldm_mcp_init.c
 * Instantiates Power PLDM for MCP core
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_init
#include <fpfw_init.h>
#include <power_pldm.h> // for power_pldm_init
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(pwr_pldm, FPFW_INIT_DEPENDENCIES("icc_mscp2mscp", "pldm", "debug_print"))
{
    fpfw_icc_base_ctx_t* local_icc_ctx = fpfw_init_get_handle("icc_mscp2mscp");
    power_pldm_init(local_icc_ctx);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
