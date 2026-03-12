//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file err_domain_scp_init.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <error_domain_ap_wdt.h>
#include <error_domain_gic.h>
#include <error_domain_smmu.h>
#include <fpfw_init.h>
#include <mscp_error_domain.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
PLACED_CODE FPFW_INIT_COMPONENT(scp_ras, FPFW_INIT_DEPENDENCIES("hm_svc", "nvic", "icc_mscp2mscp", "shared_mem", "boot_stat"))
{
    mcp_error_injection_setup_listener(fpfw_init_get_handle("icc_mscp2mscp"));
    register_scp_error_domain(fpfw_init_get_handle("icc_mscp2mscp"));
    register_smmu_error_domain();
    register_gic_error_domain();
    register_ap_wdt_error_domain();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_RAS_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_RAS_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}