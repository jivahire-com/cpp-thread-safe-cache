//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_tlm_fuses_init.c
 * Instantiates Telemetry Fuses cli
 */

/*------------- Includes -----------------*/

#include <fpfw_init.h>
#include <stddef.h>
#include <tlm_fuses_cli_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli_tlm_fuses, FPFW_INIT_DEPENDENCIES("cli", "tlm_fuses"))
{

    // Initialize the telemetry fuses CLI service
    tlm_fuses_cli_svc_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}