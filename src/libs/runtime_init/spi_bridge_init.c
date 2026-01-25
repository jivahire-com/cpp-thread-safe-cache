//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file spi_bridge_init.c
 * Initialization of the SPI Bridge
 */

/*------------- Includes -----------------*/
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <spi_ctrl_bridge.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define D2D_MBOX_SPI_CTRL_BASE_ADDR   (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS)
#define D2D_MBOX_SPI_BRIDGE_BASE_ADDR (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(spi_bridge, FPFW_INIT_DEPENDENCIES("hw_ver", "boot_stat"))
{
#if defined(SCP_RUNTIME_INIT)
    int32_t status = spi_controller_bridge_init(D2D_MBOX_SPI_CTRL_BASE_ADDR, D2D_MBOX_SPI_BRIDGE_BASE_ADDR, 20);
    if (status != 0)
    {
        return (fpfw_init_result_t){status, NULL};
    }
#endif

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_SPI_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_SPI_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
