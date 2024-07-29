//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_cli.h
 * AVS CLI APIs
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkClient.h>
#include <avs_lib.h>
#include <scp_avs_driver.h>
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_AVS_INST    4 // Die0 (AVS0 - AVS3), Die1 (AVS4 packaging pin, but is AVS0 on Die1)

/*-------------- Typedefs ----------------*/

typedef void (*avs_client_init_completion_routine)(void* CompletionContext);
typedef struct _avs_client_context_t
{
    PDFWK_INTERFACE_HEADER avs_interface;
    scp_avs_request_t Request;
    avs_client_init_completion_routine avs_init_completion;
    void* InitCompletionContext;
} avs_client_context_t, *pavs_client_context;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *
 *    Initializes the AVS CLI interface  
 *
 *    @param[in]  Interface
 * 
 *    @brief Called for each AVS.
 *
 */
void scp_avs_cli_initialize(pscp_avs_interface_t avs_array[]);

