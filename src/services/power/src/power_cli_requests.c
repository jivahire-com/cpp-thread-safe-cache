//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_cli_requests.c
 * Implements the handlers for CLI requests
 */

/*------------- Includes -----------------*/
#include "power_i.h" // for POWER_LOG_INFO, power_ap_soc_init
#include "power_loops_i.h"
#include "power_runconfig_i.h" // for power_runconfig_get_element, power...

#include <DfwkDriver.h> // for DfwkAsyncRequestComplete, DfwkInte...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <power_dfwk.h> // for (anonymous), ppower_service_cli_re...
#include <stdbool.h>    // for false
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void power_cli_requests_callback(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{

    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    case CLI_COMMANDS_POWER_CONFIG: {
        ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
        p_cli_request->p_requested_data = power_runconfig_get_element(p_cli_request->power_ext_if_cmd_id);
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    case CLI_COMMANDS_POWER_SET: {
        ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
        power_runconfig_set_element(p_cli_request->power_ext_if_cmd_id, p_cli_request->p_set_data);
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    case CLI_COMMANDS_POWER_STATUS:
    case CLI_COMMANDS_POWER_LOG: {
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void power_cli_requests_async_handler(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    // queue the request to the power loop to be done in idle
    power_loops_exec_in_idle(power_cli_requests_callback, p_request, p_context);
}
