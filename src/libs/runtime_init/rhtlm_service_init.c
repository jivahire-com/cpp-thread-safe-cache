//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rhtlm_service_init.c
 * This file contains the config/initialization for the RH Telemetry service
 */

/*------------- Includes -----------------*/
#include <cli_rhtlm.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <mu_public.h>
#include <rhtlm_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------------- Declarations (Statics and globals) --------------------*/
static rhtlm_service_ctx_t ctx = {0};
/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(rhtlm_svc, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver"))
{
    ctx.die_id = idsw_get_die_id();
    rhtlm_thread_init(&ctx);
    cli_rhtlm_initialize();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
