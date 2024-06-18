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
#include "pcie_ss_common.h" // for pcie_ss_entity_t, ss_bases_t

#include <idsw.h>            // for PLAT_ID, idsw_get_platform_sdv, SILIBS_...
#include <rpss_p1_regs.h>    // for rpss_p1_reg, rpss_p1_rp_fuse
#include <silibs_platform.h> // for MMIO_WRITE32

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

void plat_overrides_pre_pciess_config_ss_for_bifur(pcie_ss_entity_t* rpss)
{
    program_all_rp_fuses_good(rpss);
}
