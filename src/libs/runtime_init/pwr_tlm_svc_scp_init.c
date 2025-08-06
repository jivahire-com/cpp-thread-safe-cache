//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_svc_scp_init.c
 * Initializes the SCP telemetry service.
 */

/*------------- Includes -----------------*/
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <pwr_tlm_service_scp.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(pwr_tlm_svc_scp,
                    FPFW_INIT_DEPENDENCIES("mts_svc", "pwr_tlm_core_ex","hw_sem"))
{
    pwr_tlm_svc_scp_init();
    
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}