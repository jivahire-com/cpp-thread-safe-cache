//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_cores_init.c
 * This file initializes the AP Cores and brings them out of RESET.
 */

/*------------- Includes -----------------*/
#include "ap_cores_init.h"

#define __NO_CSR_TYPEDEFS__

#include <FpFwAssert.h>                 // for FPFW_RUNTIME_ASSERT
#include <ap_top_regs.h>                // for AP_TOP_D0_CORE_CLUSTER_ADDRESS, ...
#include <atu_lib.h>                    // for atu_map, atu_unmap, ...
#include <cluster_ppu_regs.h>           // for CLUSTER_PPU_PPU_PWPR_ADDRESS, CLUSTER_PPU_PPU_PWSR_ADDRESS, ...
#include <core0_ppu_regs.h>             // for CORE0_PPU_PPU_PWPR_2_ADDRESS, CORE0_PPU_PPU_PWSR_2_ADDRESS, ...
#include <core_cluster_with_pvt_regs.h> // for CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS
#include <core_manager_and_clock_control_registers_regs.h> // for CORE_MANAGER_AND_CLOCK_CONTROL_REGISTERS_PE_RVBARADDR_LW_ADDRESS, ...
#include <stdio.h>                                         // for printf
#include <voyager_dsu_cluster_regs.h> // for VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS, ...

/*-- Symbolic Constant Macros (defines) --*/
#define GET_REG(register)       (*((volatile uint32_t*)(register)))
#define SET_REG(register, data) (*((volatile uint32_t*)(register)) = data)

#define ATU_AP_CORE_CLUSTER_ADDRESS (0xb0000000U)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static int32_t ap_cores_init_primary_core_power_on(void)
{
    int32_t ret = 0;

    atu_map_entry_t atu_utility_bus_die0 = {.ap_base_address = AP_TOP_D0_CORE_CLUSTER_ADDRESS,
                                            .mscp_start_address = ATU_AP_CORE_CLUSTER_ADDRESS,
                                            .mscp_end_address = ATU_AP_CORE_CLUSTER_ADDRESS + 0x1ffffff,
                                            .attribute = {.as_uint32 = 0}};

    ret = atu_map(ATU_ID_MSCP, &atu_utility_bus_die0);
    if (ret != 0)
    {
        printf("ATU MAP failed \n");
        return E_AP_CORES_INIT_STATUS_ATU_MAP_FAIL;
    }

    uint32_t i = 0;

    printf("Powering on cluster 0\n");
    uint32_t cluster_ppu_base = ATU_AP_CORE_CLUSTER_ADDRESS +
                                CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS * (2 * i + 1) +
                                VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS;
    SET_REG(cluster_ppu_base + CLUSTER_PPU_PPU_PWPR_ADDRESS, 0x70008);
    while (GET_REG(cluster_ppu_base + CLUSTER_PPU_PPU_PWSR_ADDRESS) != GET_REG(cluster_ppu_base + CLUSTER_PPU_PPU_PWPR_ADDRESS))
    {
        ;
    }
    printf("Cluster 0 powered on.\n");

#if 0
    /* Program RVBAR */
    uint32_t core_manager_ppu_base = ATU_AP_CORE_CLUSTER_ADDRESS +
                                     CORE_CLUSTER_WITH_PVT_CORE_MANAGER_ADDRESS;
    SET_REG(core_manager_ppu_base + CORE_MANAGER_AND_CLOCK_CONTROL_REGISTERS_PE_RVBARADDR_LW_ADDRESS, 0x83000);
    SET_REG(core_manager_ppu_base + CORE_MANAGER_AND_CLOCK_CONTROL_REGISTERS_PE_RVBARADDR_UP_ADDRESS, 0);
#endif

    printf("Powering on core 0\n");
    uint32_t core_ppu_base = ATU_AP_CORE_CLUSTER_ADDRESS +
                             CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS * (2 * i + 1) +
                             VOYAGER_DSU_CLUSTER_CORE0_PPU_ADDRESS;
    SET_REG(core_ppu_base + CORE0_PPU_PPU_PWPR_2_ADDRESS, 0x8);
    while (GET_REG(core_ppu_base + CORE0_PPU_PPU_PWSR_2_ADDRESS) != GET_REG(core_ppu_base + CORE0_PPU_PPU_PWPR_2_ADDRESS))
    {
        ;
    }
    printf("Core 0 powered on.\n");

    ret = atu_unmap(ATU_ID_MSCP, &atu_utility_bus_die0);
    if (ret != 0)
    {
        printf("ATU UNMAP FAILED\n");
        return E_AP_CORES_INIT_STATUS_ATU_UNMAP_FAIL;
    }

    FPFW_RUNTIME_ASSERT(ret == E_AP_CORES_INIT_STATUS_SUCCESS);

    return ret;
}

int32_t scp_ap_cores_init(void)
{
    int32_t ret = E_AP_CORES_INIT_STATUS_SUCCESS;

    ret = ap_cores_init_primary_core_power_on();
    FPFW_RUNTIME_ASSERT(ret == E_AP_CORES_INIT_STATUS_SUCCESS);

    return ret;
}
