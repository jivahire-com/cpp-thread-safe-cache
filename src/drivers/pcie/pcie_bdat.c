//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_bdat.c
 *       Implements support to publish PCIe BDAT information
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_lib.h>
#include <bdat_schema.h>
#include <ddrss_reserved_regions.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <oi_pcie.h>
#include <pcie_ss_common.h>
#include <pciess_common.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define COMBINED_BDAT_ATU_MAPPING \
    ATU_MAPPING(COMBINED_BDAT_RSVD_REGION_BASE, 0, COMBINED_BDAT_RSVD_REGION_SIZE, ATU_PRIV_ROOT)

/*
 * We populate PCIe BDAT data at the end of the BDAT combined BDAT reservation
 * region. This is because we do not want the CRC to mismatch once UEFI
 * calculates it.
 * UEFI will read out PCIe data from the end of the region backwards and
 * create the BDAT table from the starting of this reservation.
 *
 * The final layout is as follows:
 * +----------------------------------------------------------------------+
 * | COMBINED_BDAT_RSVD_REGION_BASE                                       |
 * +----------------------------------------------------------------------+
 * | BDAT Header (BDAT_STRUCTURE_MSFT_5)                                  |
 * +----------------------------------------------------------------------+
 * | Final DDRSS BDAT (BDAT_MEMORY_DATA_STRUCTURE_MSFT_5)                 |
 * +----------------------------------------------------------------------+
 * | Final PCIe BDAT (BDAT_PCIE_DATA_STRUCTURE_MSFT_5)                    |
 * +----------------------------------------------------------------------+
 * | Free space                                                           |
 * +----------------------------------------------------------------------+
 * | Temp BDAT_PCIE_PER_RP_DATA_MSFT_1 rpData[PCIE_BDAT_MAX_RP_COUNT]     |
 * | for UEFI usage (zeroed out once UEFI is done)                        |
 * +----------------------------------------------------------------------+
 * | COMBINED_BDAT_RSVD_REGION_END                                        |
 * +----------------------------------------------------------------------+
 */
#define GET_BDAT_ADDRESS_FOR_RP(rp_index_offset)                                                       \
    ((void*)((uintptr_t)pcie_bdat_atu_map_struct.mscp_start_address + COMBINED_BDAT_RSVD_REGION_SIZE - \
             (sizeof(BDAT_PCIE_PER_RP_DATA_MSFT_1) * (PCIE_BDAT_MAX_RP_COUNT - (rp_index_offset)))))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool bdat_published[NUM_RPSS * PCIESS_NUM_PORTS] = {false};
static BDAT_PCIE_PER_RP_DATA_MSFT_1 temp_bdat_entry = {0};

/*------------- Functions ----------------*/
silibs_status_t publish_pcie_bdat_info_for_this_rp(pcie_ss_entity_t* rpss, uint8_t rp_index)
{
    atu_map_entry_t pcie_bdat_atu_map_struct = COMBINED_BDAT_ATU_MAPPING;
    uint8_t rp_index_offset = (rpss->id * PCIESS_NUM_PORTS) + rp_index;

    /* Only publish BDAT information once per boot */
    if (bdat_published[rp_index_offset])
    {
        return SILIBS_SUCCESS;
    }

    /* Mark as published to prevent retries on re-train/SBR events */
    bdat_published[rp_index_offset] = true;

    /* Zero out the temp BDAT buffer */
    memset(&temp_bdat_entry, 0, sizeof(BDAT_PCIE_PER_RP_DATA_MSFT_1));

    silibs_status_t sts = atu_map(ATU_ID_MSCP, &pcie_bdat_atu_map_struct);
    if (sts != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: BDAT ATU map failed with status: %d\n", rpss->id, rp_index, (int8_t)sts);
        return sts;
    }

    /*
     * oi_pcie_ss_populate_rp_bdat can return an error status even if the lane count
     * mismatches. But even in such cases, we want to publish BDAT information
     * which can help diagnose link quality issues. Hence, we are ignoring the return status here
     */
    sts = oi_pcie_ss_populate_rp_bdat(rpss, rp_index, &temp_bdat_entry, sizeof(BDAT_PCIE_PER_RP_DATA_MSFT_1));
    memcpy(GET_BDAT_ADDRESS_FOR_RP(rp_index_offset), &temp_bdat_entry, sizeof(BDAT_PCIE_PER_RP_DATA_MSFT_1));
    atu_unmap(ATU_ID_MSCP, &pcie_bdat_atu_map_struct);

    return sts;
}
