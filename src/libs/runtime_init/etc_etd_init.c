//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file etc_etd_init.c
 * Instantiates Event Tracing Collector for the MCP and SCP cores
 */

/*------------- Includes -----------------*/

#include <boot_status.h>
#include <etc_etd_svc.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW...
#include <idsw.h>      // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <stddef.h> // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(etd, FPFW_INIT_DEPENDENCIES("std_io", "debug_print"))
{
    etd_service_context_t* context;
    etd_svc_init();
    context = get_etd_service_context();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_ETD_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_ETD_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, context};
}

FPFW_INIT_COMPONENT(etc, FPFW_INIT_DEPENDENCIES("etd"))
{
    etc_service_context_t* context;
    etc_svc_init();
    context = get_etc_service_context();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_ETC_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_ETC_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, context};
}
