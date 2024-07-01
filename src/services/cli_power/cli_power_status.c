//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_status.c
 * Source file to implement power status CLI commands.
 */

/*------------- Includes -----------------*/
#include "DfwkCommon.h" // for _DFWK_ASYNC_REQUEST_HEADER
#include "power_dfwk.h" // for (anonymous), ppower_service_cli_req...

#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <cli_power_common.h>    // for ST_COUNT
#include <cli_power_interface.h> // for power_cli_sub_command_dictionary_el...
#include <cli_power_status.h>
#include <stdint.h> // for uint32_t
#include <stdio.h>  // for printf, NULL
#include <string.h> // for strcmp

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
