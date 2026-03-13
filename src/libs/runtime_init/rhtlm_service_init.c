//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rhtlm_service_init.c
 * This file contains the config/initialization for the RH Telemetry service
 */

/*------------- Includes -----------------*/
#include <cli_rhtlm.h>
#include <ddr_rhtlm_service.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <mu_public.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(rhtlm_svc, FPFW_INIT_DEPENDENCIES("std_io"))
{
    cli_rhtlm_initialize();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
