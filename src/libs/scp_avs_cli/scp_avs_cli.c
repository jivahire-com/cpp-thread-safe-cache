//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_cli.c
 * This file contains the implementation of the AVS CLI (client) module
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>     // for DfwkAsyncRequestInititalize, PDFWK_INTER...
#include <scp_avs_cli.h>    // for avs_client_init_completion_routine, pavs...
#include <scp_avs_driver.h> // for scp_avs_request_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void scp_avs_cli_initialize(pavs_client_context Context,
                            PDFWK_INTERFACE_HEADER Interface,
                            avs_client_init_completion_routine InitCompletion,
                            void* InitCompletionContext)
{
    Context->avs_interface = Interface;
    Context->avs_init_completion = InitCompletion;
    Context->InitCompletionContext = InitCompletionContext;

    //
    // Only need to initialize the async request once here.
    //
    DfwkAsyncRequestInititalize(&Context->Request.Header, sizeof(scp_avs_request_t));
}