//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_log.c
 * Source file with implementations of power log CLI commands.
 */

/*------------- Includes -----------------*/
#include "DfwkCommon.h" // for PDFWK_ASYNC_REQUEST_HEADER, _DFWK_ASYNC_REQU...

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <cli_power_log.h>
#include <stdio.h> // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
void cli_power_log_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_context);

    printf("Power Logging yet to be implemented\n");
}
