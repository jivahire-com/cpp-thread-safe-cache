//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ap_advlog_pldm_mcp_cli_init.c
 * Instantiates AP Adv Logger PLDM CLI for MCP core
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <ap_advlog_pldm.h> // for ap_advlog_pldm_init
#include <cli_ap_advlog.h> // for cli_ap_advlog_pldm_inits
#include <fpfw_init.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ap_advlog_cli, FPFW_INIT_DEPENDENCIES("pldm", "debug_print"))
{
    cli_ap_advlog_pldm_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
