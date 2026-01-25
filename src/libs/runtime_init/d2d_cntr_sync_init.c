//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file d2d_cntr_sync_init.c
 * Synchronizes counter of d1 with d0 for uniform timestamp across dies
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <boot_status.h>
#include <d2d_cntr_sync.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <stddef.h> // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(d2d_cntr_sync, FPFW_INIT_DEPENDENCIES("hw_ver", "debug_print", "gtimer_stg_2", "cfg_mgr", "boot_stat"))
{
    //! d2d sync counters are not supported on SVP currently
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        KNG_DIE_ID die_num = idsw_get_die_id();
        KNG_CPU_TYPE cpu_type = idsw_get_cpu_type();
        d2d_cntr_sync_init(die_num, cpu_type);
        FPFW_DBGPRINT_INFO("D2D Sync Counter Done, die_num: [%u]\n", die_num);
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_D2DCNTR_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_D2DCNTR_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
