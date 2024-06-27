//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_status.c
 * Source file to implement power status CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_power_common.h>
#include <cli_power_status.h>
#include <power_dfwk.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
void cli_power_status_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_context);

    printf("Commands to be implemented\n");
}
