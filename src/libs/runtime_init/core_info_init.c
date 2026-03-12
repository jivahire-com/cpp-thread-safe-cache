//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_init.c
 * This file contains the config/initialization for the APcore service
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <core_info.h>
#include <fpfw_init.h>
#if defined(SCP_RUNTIME_INIT)
    #include <fuse_client.h>
    #include <fuse_init.h>
    #include <utils.h>
#endif
#include <fuse_utils.h>
#include <idhw.h>
#include <idsw.h>
#include <stdio.h> // for NULL, printf
/*-------------- Functions ---------------*/
#if defined(SCP_RUNTIME_INIT)
PLACED_CODE FPFW_INIT_COMPONENT(core_info, FPFW_INIT_DEPENDENCIES("cfg_mgr", "fuse_post_mesh", "boot_stat"))
#elif defined(MCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(core_info, FPFW_INIT_DEPENDENCIES("cfg_mgr", "fuse_en", "boot_stat"))
#endif
{
    core_info_init();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_COREINFO_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_COREINFO_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}