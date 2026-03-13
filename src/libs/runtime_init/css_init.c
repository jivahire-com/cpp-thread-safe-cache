//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file css_init.c
 * Instantiates CSS
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <boot_status.h>
#include <css.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>
#include <utils.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
PLACED_CODE FPFW_INIT_COMPONENT(css_prme, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "atu_svc", "cfg_mgr", "debug_print", "boot_stat"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    FPFW_DBGPRINT_INFO("CSS Pre Mesh init on die[%d]\n", die_num);

    css_pre_mesh_init(die_num);

    // TODO: System tower should be configured by HSP, until then configure here for SVP
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1725718
    if (IS_PLATFORM_SVP())
    {
        css_configure_system_tower(die_num);
        FPFW_DBGPRINT_INFO("System Control Tower Initialized\n");
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_PRE_CSS_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_PRE_CSS_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(css_pome, FPFW_INIT_DEPENDENCIES("std_io", "accel_iso_cfg", "debug_print", "ift", "boot_stat"))
{
    FPFW_DBGPRINT_INFO("CSS Post Mesh init\n");

    css_post_mesh_init();
    FPFW_DBGPRINT_INFO("CSS Post Mesh init done\n");

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_POST_CSS_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_POST_CSS_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}