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
FPFW_INIT_COMPONENT(pcie, FPFW_INIT_DEPENDENCIES("mesh", "dfwk", "tower_cfg", "vab"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    PDFWK_THREADX_HOST host = fpfw_init_get_handle(dfwk_id);
    uint16_t rpss_to_init = 0;
    KNG_PLAT_ID plat = idsw_get_platform_sdv();

    switch (plat)
    {
    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rpss_to_init = ((1 << RPSS1) | (1 << RPSS2));
        break;

    case PLATFORM_SVP_SIM:
    case PLATFORM_RVP_EVT_SILICON:
        rpss_to_init = ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3));
        break;

    default:
        printf("Skip RPSS init on unknown platform id: %d\n", plat);
        break;
    }

    void* pcie_dev_handles = scp_pcie_initialize(&(host->Schedule), rpss_to_init);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, pcie_dev_handles};
}

FPFW_INIT_COMPONENT(pcie_cli, FPFW_INIT_DEPENDENCIES("pcie", "cli"))
{
    fpfw_init_component_id_t pcie_id = "pcie";
    void* pcie_dev_handles = fpfw_init_get_handle(pcie_id);
    pcie_cli_init((pciess_device_t*)pcie_dev_handles);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
