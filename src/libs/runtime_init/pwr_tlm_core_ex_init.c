//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_ex_init.c
 * This file contains the initialization of the PWR TLM Core Exchange.
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <pwr_tlm_core_exchange.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
// initialize from SCP only
FPFW_INIT_COMPONENT(pwr_tlm_core_ex, FPFW_INIT_DEPENDENCIES("boot_stat"))
{
    pwr_tlm_core_exch_init();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(&boot_status_req,
                            (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_POWER_TLM_CORE_INIT_END
                                                             : MSCP_BOOT_STATUS_CODE_MCP_POWER_TLM_CORE_INIT_END,
                            GEN_BOOT_STATUS_EX_GENERIC_CODE(
                                (idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                MSCP_GENERIC,
                                (idsw_get_die_id() == DIE_0)
                                    ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                    : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}