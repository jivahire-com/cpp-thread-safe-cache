//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scmi_init.c
 * This file contains the config/initialization for the SCMI driver
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <boot_status.h>
#include <fpfw_init.h> // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw.h>      // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <scmi_init.h>
#include <stdio.h> // for printf
#include <utils.h>

// pass ap_core svc interface to SCMI driver
PLACED_CODE FPFW_INIT_COMPONENT(scmi_drv, FPFW_INIT_DEPENDENCIES("icc_mscp2tfa_if", "ap_core_int", "std_io", "boot_stat"))
{
    scmi_drv_init(fpfw_init_get_handle("icc_mscp2tfa_if"));
    scmi_set_apcore_interface(fpfw_init_get_handle("ap_core_int"));
    FPFW_DBGPRINT_INFO("SCMI Init Complete\n");

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(&boot_status_req,
                            (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_POWER_SCMI_INIT_END
                                                             : MSCP_BOOT_STATUS_CODE_MCP_POWER_SCMI_INIT_END,
                            GEN_BOOT_STATUS_EX_GENERIC_CODE(
                                (idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                MSCP_GENERIC,
                                (idsw_get_die_id() == DIE_0)
                                    ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                    : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
