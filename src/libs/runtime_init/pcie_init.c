//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Initializes and starts up PCIe subsystem management drivers and service.
 */

/*------------- Includes -----------------*/
#include <DfwkPtrTypes.h>
#include <DfwkThreadXHost.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <pcie_cli.h>
#include <scp_pcie_manager.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(pcie, FPFW_INIT_DEPENDENCIES("mesh", "dfwk", "tower_cfg", "vab", "cfg_mgr"))
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
        printf("WARNING: rpss_mask == %d\n", rpss_mask);
    }

    switch (plat)
    {
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rpss_to_init = ((1 << RPSS1) | (1 << RPSS2) | (1 << RPSS5));
        break;

    case PLATFORM_SVP_SIM:
    case PLATFORM_RVP_EVT_SILICON:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
        // Note: Emu only supports bifur 1x16 for rpss2&rpss3 on each die
        rpss_to_init = ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3) | (1 << RPSS4) |
                        (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7));
        break;

    default:
        printf("Skip RPSS init on unknown platform id: %d\n", plat);
        break;
    }

    rpss_to_init &= rpss_mask;

    if ((idsw_get_platform_sdv() == PLATFORM_EMU_1D) || (idsw_get_platform_sdv() == PLATFORM_EMU_1D_8C) ||
        (idsw_get_platform_sdv() == PLATFORM_EMU_2D) || (idsw_get_platform_sdv() == PLATFORM_EMU_2D_8C))
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }
    void* pcie_dev_handles = scp_pcie_initialize(&(host->Schedule), rpss_to_init, die_id);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, pcie_dev_handles};
}

FPFW_INIT_COMPONENT(pcie_cli, FPFW_INIT_DEPENDENCIES("pcie", "cli"))
{
    fpfw_init_component_id_t pcie_id = "pcie";
    void* pcie_dev_handles = fpfw_init_get_handle(pcie_id);
    if ((idsw_get_platform_sdv() == PLATFORM_EMU_1D) || (idsw_get_platform_sdv() == PLATFORM_EMU_1D_8C) ||
        (idsw_get_platform_sdv() == PLATFORM_EMU_2D) || (idsw_get_platform_sdv() == PLATFORM_EMU_2D_8C))
    {
        printf("Skip PCIe CLI init on emulation");
    }
    else
    {
        pcie_cli_init((pciess_device_t*)pcie_dev_handles);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
