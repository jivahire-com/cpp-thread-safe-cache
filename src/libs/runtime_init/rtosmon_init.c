//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rtosmon_init.c
 * @brief FPFW init component for RTOSMon on SCP/MCP Cortex-M7
 * This component initializes RTOSMon via rtosmon_plugin_init(), after its
 * declared dependencies are ready.
 *
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <bug_check.h>          // for BUG_ASSERT
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_result_t
#include <fpfw_status.h>        // for fpfw_status_t
#include <idsw.h>
#include <idsw_kng.h>
#include <rtosmon_service.h>    // rtosmon_service_init

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(rtosmon, FPFW_INIT_DEPENDENCIES("etc", "systick_upd","boot_stat"))
{
    BUG_ASSERT(rtosmon_service_init() == FPFW_STATUS_SUCCESS);
    
    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_RTOSMON_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_RTOSMON_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
