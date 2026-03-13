//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file hw_semaphore_init.c
 * Instantiates HW semaphores.
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <bug_check.h>    // for BUG_CHECK
#include <fpfw_init.h>    // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <hw_semaphore.h> // for init_hw_semaphore
#include <idsw.h>         // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <silibs_platform.h> // for DEBUG_PRINT
#include <utils.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
#if defined(SCP_RUNTIME_INIT)
PLACED_CODE FPFW_INIT_COMPONENT(hw_sem, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "mesh_stg_2", "boot_stat"))
#elif defined(MCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(hw_sem, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "boot_stat"))
#endif
{
    KNG_STATUS status = init_hw_semaphore();

    if (status != KNG_SUCCESS)
    {
        DEBUG_PRINT("HW Semaphore init Error status: [0x%x]\n", status);
        BUG_CHECK(status, 0, 0);
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_HW_SEM_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_HW_SEM_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
