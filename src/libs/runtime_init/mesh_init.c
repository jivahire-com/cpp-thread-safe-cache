//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file mesh_init.c
 * Instantiates Mesh
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <boot_status.h>   // for post_led_status
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(mesh_stg_1,
                    FPFW_INIT_DEPENDENCIES("i3c_controller", "icc_hspmbx", "sysinfo", "icc_d2dmbx", "fuse_pre_mesh", "debug_print", "var_serv", "d2d_cntr_sync", "boot_stat"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    FPFW_DBGPRINT_INFO("Mesh init, die_num: [%u]\n", die_num);
    boot_status_req_t boot_status_req = {0};
    post_led_status(&boot_status_req, LED_STATUS_CODE_SCP_MESH_INIT_START);

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    mesh_init(die_num, icc_ctx);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(mesh_stg_2, FPFW_INIT_DEPENDENCIES("tower_cfg", "boot_stat"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    FPFW_DBGPRINT_INFO("D2D init, die_num: [%u]\n", die_num);

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    d2d_init(die_num, icc_ctx);
    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_MESH_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_D2DMBX_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
