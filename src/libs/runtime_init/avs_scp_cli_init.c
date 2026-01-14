//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file avs_scp_cli_init.c
 * Instantiates AVS CLIfor the SCP
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <scp_avs.h>
#include <scp_avs_cli.h>
#include <scp_avs_driver.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(avs_cli, FPFW_INIT_DEPENDENCIES("cli", "avs0_int", "avs1_int", "avs2_int", "avs3_int", "hw_ver"))
{
    scp_avs_interface_t* cli_array[MAX_AVS_INST] = {0};

    cli_array[AVS_BUS0] = (scp_avs_interface_t*)fpfw_init_get_handle("avs0_int");
    if (idsw_get_die_id() == DIE_0) // DIE_1 only has one AVSBus.
    {
        cli_array[AVS_BUS1] = (scp_avs_interface_t*)fpfw_init_get_handle("avs1_int");
        cli_array[AVS_BUS2] = (scp_avs_interface_t*)fpfw_init_get_handle("avs2_int");
        cli_array[AVS_BUS3] = (scp_avs_interface_t*)fpfw_init_get_handle("avs3_int");
    }

    scp_avs_cli_initialize(cli_array);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
