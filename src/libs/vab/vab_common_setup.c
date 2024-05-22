//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This file carries releases VABs from reset, initializes VAB tower
 * interconnects and any other top-level VAB initialization that may be
 * needed. Some routines may be blocking by deign to ensure VABs are
 * correctly brought up.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <atu_lib.h>
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_common.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <tower_vab.h>
#include <vab_cded_ioss_top_regs.h>
#include <vab_pcr_init.h>
#include <vab_regs.h>
#include <vab_rpss_top_regs.h>
#include <vab_sdm_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t atu_vab_map[MAX_VAB_INSTANCES] = {
    {
        /* D0-RPSS0 */
        .ap_base_address = AP_TOP_D0_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D0-RPSS1 */
        .ap_base_address = AP_TOP_D0_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D0-RPSS2 */
        .ap_base_address = AP_TOP_D0_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D0-RPSS3 */
        .ap_base_address = AP_TOP_D0_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-RPSS0 */
        .ap_base_address = AP_TOP_D1_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-RPSS1 */
        .ap_base_address = AP_TOP_D1_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-RPSS2 */
        .ap_base_address = AP_TOP_D1_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-RPSS3 */
        .ap_base_address = AP_TOP_D1_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_RPSS0_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D0-SDMSS */
        .ap_base_address = AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_SDM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-SDMSS */
        .ap_base_address = AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_SDM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D0-CDED/IOSS */
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_CDED_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
    {
        /* D1-CDED/IOSS */
        .ap_base_address = AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_CDED_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
    },
};

/*------------- Functions ----------------*/
static int init_one_vab(SUBSYSTEM_WITH_VAB_ID vab_id)
{
    int status = SILIBS_SUCCESS;
    int unmap_status = SILIBS_SUCCESS;

    status = atu_map(ATU_ID_MSCP, &atu_vab_map[vab_id]);
    if (status != SILIBS_SUCCESS)
    {
        printf("%s: atu_map failed! VAB id: %d | Status: %d\n", __func__, vab_id, status);
        goto exit;
    }

    uint64_t mscp_start_address = atu_vab_map[vab_id].mscp_start_address;
    uint64_t vab_base_addr = atu_vab_map[vab_id].ap_base_address;

    /* HSP will configure VAB SAMs and APUs so this will be removed later */
    tower_configure_vab_apu(vab_base_addr, mscp_start_address + VAB_VAB_TOWER_ADDRESS);
    configure_vab_system_addr_map(vab_base_addr, mscp_start_address + VAB_VAB_TOWER_ADDRESS);

    deassert_pcr_reset(mscp_start_address + VAB_VAB_PCR_TOP_ADDRESS);
    status = vab_pcr_init(mscp_start_address + VAB_VAB_PCR_TOP_ADDRESS);
    if (status != SILIBS_SUCCESS)
    {
        printf("%s: vab_pcr_init failed. VAB id: %d | Status: %d\n", __func__, vab_id, status);
        goto atu_unmap_and_exit;
    }

atu_unmap_and_exit:
    unmap_status = atu_unmap(ATU_ID_MSCP, &atu_vab_map[vab_id]);
    if (unmap_status != SILIBS_SUCCESS)
    {
        printf("%s: atu_unmap failed. VAB id: %d | Status: %d\n", __func__, vab_id, unmap_status);
        status = unmap_status;
    }

exit:
    return status;
}

int vab_init(uint16_t vab_instances_to_init)
{
    int status = SILIBS_SUCCESS;

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init >> vab_id) & 0x1)
        {
            status |= init_one_vab((SUBSYSTEM_WITH_VAB_ID)vab_id);
            FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);
        }
    }
    return status;
}
