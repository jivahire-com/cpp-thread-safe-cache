//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_accel_intr_init.c
 * Instantiates accelerator interrupt service cli for SCP
 */

/*------------- Includes -----------------*/
#include <cli_accel_int.h>      // for cli_accel_int_init
#include <fpfw_init.h>          // for fpfw_init_get_handle, FPFW_INIT_COMPONENT
#include <stddef.h>             // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli_accel_intr, FPFW_INIT_DEPENDENCIES("cli", "accel"))
{
    return (fpfw_init_result_t){cli_accel_int_init(), NULL};
}