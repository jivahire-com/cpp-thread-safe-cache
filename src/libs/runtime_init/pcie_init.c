//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes and starts up PCIe subsystem management drivers and service.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkPtrTypes.h>
#include <DfwkThreadXHost.h>
#include <atu_api.h>
#include <boot_status.h>
#include <cxl.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <pcie_cli.h>
#include <scp_pcie_manager.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
PLACED_CODE FPFW_INIT_COMPONENT(pcie, FPFW_INIT_DEPENDENCIES("mesh_stg_2", "dfwk", "tower_cfg", "vab", "cfg_mgr", "boot_stat"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    PDFWK_THREADX_HOST host = fpfw_init_get_handle(dfwk_id);
    uint16_t rpss_to_init = 0;
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    KNG_DIE_ID die_id = (KNG_DIE_ID)idsw_get_die_id();

    uint16_t rpss_mask = 0;
    bool die1_rpss_enabled = config_get_die1_rpss_enabled(); /* Will be fetched via config manager */
    if (die_id == DIE_0)
    {
        rpss_mask = (1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3);
    }
    else if (die_id == DIE_1 && die1_rpss_enabled)
    {
        rpss_mask = (1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7);
    }
    else
    {
        FPFW_DBGPRINT_WARNING("WARNING: rpss_mask == %d\n", rpss_mask);
    }

    switch (plat)
    {
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rpss_to_init = ((1 << RPSS1) | (1 << RPSS2) | (1 << RPSS5));
        break;

    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
    case PLATFORM_SVP_SIM:
    case PLATFORM_RVP_EVT_SILICON:
        /* Note: Emulation only supports 1x16 bifurcation on all RPSS on each die! */
        rpss_to_init = ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3) | (1 << RPSS4) |
                        (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7));
        break;

    case PLATFORM_SVP_MIN_CONFIG_SIM:
        FPFW_DBGPRINT_INFO("Skip RPSS init on SVP min config!\n");
        break;

    default:
        FPFW_DBGPRINT_INFO("Skip RPSS init on unknown platform id: %d\n", plat);
        break;
    }

    rpss_to_init &= rpss_mask;

    void* pcie_dev_handles = scp_pcie_initialize(&(host->Schedule), rpss_to_init, die_id);
    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_MSCP_PCIE_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_MSCP_PCIE_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, pcie_dev_handles};
}

PLACED_CODE FPFW_INIT_COMPONENT(pcie_config, FPFW_INIT_DEPENDENCIES("pcie", "atu_svc", "ddr", "var_serv"))
{
    KNG_PLAT_ID plat = idsw_get_platform_sdv();

    if (plat == PLATFORM_SVP_MIN_CONFIG_SIM)
    {
        FPFW_DBGPRINT_INFO("Skip RPSS configuration init on SVP min config!\n");
        goto done;
    }

    scp_pcie_start_config_service_thread();

done:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(pcie_cli, FPFW_INIT_DEPENDENCIES("pcie", "cli"))
{
    fpfw_init_component_id_t pcie_id = "pcie";
    void* pcie_dev_handles = fpfw_init_get_handle(pcie_id);

    pcie_cli_init((pciess_device_t*)pcie_dev_handles);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cxl_chbcr, FPFW_INIT_DEPENDENCIES("mesh_stg_2", "cfg_mgr", "pcie", "ddr", "atu_svc"))
{
    KNG_DIE_ID die_id = (KNG_DIE_ID)idsw_get_die_id();

    // Only one die (die0) should initialize the CHBCR
    if (die_id == DIE_1)
    {
        goto done_cxl_init;
    }

    /*
     * Always ensure the CHBCR region is zeroed out explicitly before init
     * This prevents issues when DDR encryption is enabled and random data
     * is present in the memory region
     */
    memset((void*)MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR, 0, MSCP_ATU_AP_WINDOW_CHBCR_SIZE);

    cxl_chbcr_init((uint8_t*)MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR);

done_cxl_init:
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
