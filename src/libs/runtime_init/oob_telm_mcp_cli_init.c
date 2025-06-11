//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file oob_telm_mcp_cli_init.c
 * Initialize the OOB telemetry MCP CLI components.
 */
/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <fpfw_dw_i3c.h>    // for fpfw_dw_i3c_config_t, fpfw_dw_i3c_device_t
#include <fpfw_i3c_cli.h>   // for fpfw_dw_i3c_interface_initialize
#include <fpfw_init.h>      // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_mctp_cli.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <mcp_exp_top_regs.h>
#include <silibs_mcp_top_regs.h>

/*------------- Typedefs -----------------*/
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Initialize I3C target CLI
 *  
 */
FPFW_INIT_COMPONENT(i3c_target_cli, FPFW_INIT_DEPENDENCIES("cli", "i3c_target"))
{
    if (idsw_get_die_id() == DIE_1)
    { // DIE_1 I3C_2 not in use.
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    fpfw_dw_i3c_device_t* i3c_target_device = (fpfw_dw_i3c_device_t*)fpfw_init_get_handle("i3c_target");

    static fpfw_dw_i3c_interface_t i3c_target_interface;
    fpfw_dw_i3c_interface_initialize(i3c_target_device, &i3c_target_interface);

    static fpfw_i3c_cli_config_entry_t i3c_cli_config_table[] = {{.i3c_interface = &i3c_target_interface.i3c_interface}};
    fpfw_i3c_cli_initialize(i3c_cli_config_table, sizeof(i3c_cli_config_table) / sizeof(fpfw_i3c_cli_config_entry_t));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

#define NUM_MCTP_INSTANCES 1
FPFW_INIT_COMPONENT(mctp_cli, FPFW_INIT_DEPENDENCIES("mctp"))
{
    // Get the MCTP context from the mctp init component.
    fpfw_mctp* mctp_context = (fpfw_mctp*)fpfw_init_get_handle("mctp");

    if (mctp_context == NULL)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_POINTER, NULL};
    }

    static fpfw_mctp_cli_config_t mctp_cli_config[NUM_MCTP_INSTANCES] = {0};
    mctp_cli_config[0].mctp_interface = mctp_context;

    // Initialize MCTP CLI context
    fpfw_mctp_cli_initialize(mctp_cli_config, NUM_MCTP_INSTANCES);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
