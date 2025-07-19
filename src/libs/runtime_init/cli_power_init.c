//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_init.c
 * Instantiates power cli for SCP
 */

/*------------- Includes -----------------*/
#include "power_dfwk.h"  // for power_service_interface_t
#include <DbgPrint.h>
#include <cli_power.h>   // for cli_power_init
#include <fpfw_init.h>   // for fpfw_init_get_handle, FPFW_INIT_COMPONENT
#include <stddef.h>      // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli_pwr, FPFW_INIT_DEPENDENCIES("cli", "pwr_int", "debug_print", "icc_d2dmbx"))
{
    static power_service_interface_t* power_interface;
    static fpfw_icc_base_ctx_t* p_icc_base_ctx;

    // FPFW_DBGPRINT_INFO("Initializing Power CLI\n");
    power_interface = (power_service_interface_t*)fpfw_init_get_handle("pwr_int");
    p_icc_base_ctx = (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_d2dmbx");

    return (fpfw_init_result_t){cli_power_init(power_interface, p_icc_base_ctx), NULL};
}