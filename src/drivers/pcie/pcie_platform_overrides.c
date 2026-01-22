//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implement platform specific overrides to account for differences
 * when running on different pre-silicon SDVs like big fpga, svp,
 * emulation etc.
 */

/*------------- Includes -----------------*/
#include <idsw.h>            // for idsw_get_platform_sdv
#include <idsw_kng.h>        // for KNG_PLAT_ID, PLATFORM_SVP_SIM
#include <pcie_common.h>     // for PCIESS_NUM_PORTS
#include <pcie_rp_common.h>  // for pcie_rp_entity_t, rp_offsets_t
#include <pcie_ss_common.h>  // for pcie_ss_entity_t, ss_bases_t
#include <rpss_p1_regs.h>    // for rpss_p1_reg, rpss_p1_rp_fuse
#include <silibs_platform.h> // for MMIO_WRITE32
#include <stdint.h>          // for uint8_t
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void program_all_rp_fuses_good(pcie_ss_entity_t* rpss);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 *  @brief Program all RPSS fuses to read good till we have a fuse
 *         distribution module that tells us which rpss are actually
 *         good.
 *
 *  @param[in] rpss
 *             Pointer to the pcie_ss_entity_t type that is being
 *             initialized.
 *
 *  @return
 *      None
 */
static void program_all_rp_fuses_good(pcie_ss_entity_t* rpss)
{
    rpss_p1_reg* regs = (rpss_p1_reg*)rpss->bases.p1_base_addr;
    rpss_p1_rp_fuse reg;
    reg.as_uint32 = 0;
    MMIO_WRITE32(&regs->rp_fuse, reg.as_uint32);
}

bool plat_get_phy_programming_support()
{
    /*
     * Only actual silicon has PCIe PHYs on each RPSS.
     *
     * Emulation and big FPGA do not support PHY programming.
     *
     * SVP stubs out PHY register space but due to this being done
     * partially, it is disabled here.
     * This will be fixed once the bug below is fixed and an SVP
     * release is integrated:
     * https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2292635
     */
    return ((idsw_get_platform_sdv() >= PLATFORM_RVP_EVT_SILICON) ? true : false);
}

PLACED_CODE void plat_overrides_pre_pciess_config_ss_for_bifur(pcie_ss_entity_t* rpss)
{
    KNG_PLAT_ID plat = (KNG_PLAT_ID)idsw_get_platform_sdv();

    /* Only override PCIe root port fuses and set them to good on development platforms */
    if (plat < PLATFORM_RVP_EVT_SILICON)
    {
        program_all_rp_fuses_good(rpss);
    }
}
