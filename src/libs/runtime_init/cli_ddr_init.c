//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file cli_ddr_init.c
 * Instantiates CLI for DDR in the SCP
 */

/*------------- Includes -----------------*/
#include <cli_ddr.h>   // for cli_ddr_init
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>    // for NULL
#include <utils.h>

/*------- Symbolic Constant Macros (defines) ----------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
PLACED_CODE FPFW_INIT_COMPONENT(cli_ddr, FPFW_INIT_DEPENDENCIES("cli", "ddr"))
{
    cli_ddr_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
