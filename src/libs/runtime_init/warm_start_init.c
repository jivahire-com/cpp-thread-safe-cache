//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_init.c
 * Initialization of Warm Start.
 */

/*------------- Includes -----------------*/
#include "warm_start.h"

#include <boot_status.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idhw.h>
#include <stddef.h> // for NULL
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
PLACED_CODE FPFW_INIT_COMPONENT(ws_init, FPFW_INIT_DEPENDENCIES("mpu", "boot_stat"))
{
    warm_start_init();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_WS_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_WS_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
