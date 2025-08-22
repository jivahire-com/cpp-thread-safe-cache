//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file oob_uart_mcp_cli_init.c
 * Initialize the OOB telemetry MCP CLI components.
 */
/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <fpfw_init.h>      // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_mctp_cli.h>
#include <fpfw_pldm_cli.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <mcp_exp_top_regs.h>
#include <silibs_mcp_top_regs.h>

/*------------- Typedefs -----------------*/
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

#define NUM_MCTP_INSTANCES 1
FPFW_INIT_COMPONENT(mctp_cli, FPFW_INIT_DEPENDENCIES("cli", "mctp"))
{
    // Get the MCTP context from the mctp init component.
    fpfw_mctp *mctp_ctx = (fpfw_mctp*)fpfw_init_get_handle("mctp");
    if (mctp_ctx == NULL)
    {
        // Skip initialization on Die 1
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    if (mctp_ctx == NULL)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_POINTER, NULL};
    }

    static fpfw_mctp_cli_config_t mctp_cli_config[NUM_MCTP_INSTANCES] = {0};
    mctp_cli_config[0].mctp_interface = mctp_ctx;

    // Initialize MCTP CLI context
    fpfw_mctp_cli_initialize(mctp_cli_config, NUM_MCTP_INSTANCES);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(pldm_cli, FPFW_INIT_DEPENDENCIES("cli", "pldm"))
{
    if (idsw_get_die_id() == DIE_1)
    {
        // Skip initialization on Die 1
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    fpfw_pldm_cli_initialize();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
