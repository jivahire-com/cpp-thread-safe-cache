//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file i3c_target_mcp_init.c
 * Initialize i3c_2 mcp target controller 
 */
/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <fpfw_dw_i3c.h>    // for fpfw_dw_i3c_config_t, fpfw_dw_i3c_device_t
#include <fpfw_i3c_cli.h>   // for fpfw_dw_i3c_interface_initialize
#include <fpfw_init.h>      // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
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

    fpfw_dw_i3c_device_t* i3cTargetDevice = (fpfw_dw_i3c_device_t*)fpfw_init_get_handle("i3c_target");

    static fpfw_dw_i3c_interface_t i3cTargetInterface;
    fpfw_dw_i3c_interface_initialize(i3cTargetDevice, &i3cTargetInterface);

    static fpfw_i3c_cli_config_entry_t i3cCliConfigTable[] = {{.i3c_interface = &i3cTargetInterface.i3c_interface}};
    fpfw_i3c_cli_initialize(i3cCliConfigTable, sizeof(i3cCliConfigTable) / sizeof(fpfw_i3c_cli_config_entry_t));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}