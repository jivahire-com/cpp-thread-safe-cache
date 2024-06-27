//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_set.c
 * Source file with implementations of power set CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_power_set.h>
#include <power_dfwk.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
void cli_power_set_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_context);

    printf("Commands to be implemented\n");
}
