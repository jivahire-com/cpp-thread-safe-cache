//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implement platform specific overrides to account for differences
 * when running on different pre-silicon SDVs like big fpga, svp,
 * emulation etc.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <idsw.h>
#include <pciess.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Carry out rpss sdv/platform-specific overrides before the subsystem
 *         is configured for bifurcation.
 *
 *  @param[in] rpss
 *             Instance of the pcie rpss being configured.
 *
 * @return None - majority of this routine will just overwrite registers within
 *          the pcie rpss.
 */
void plat_overrides_pre_pciess_config_ss_for_bifur(pcie_ss_entity_t* rpss);

/**
 *  @brief Overrides :
 *          1. Program the GIC Comparator in the RPSS to enable MSI
 *
 *  @param[in] rpss
 *             Instance of the pcie rpss being configured.
 *
 * @return None - majority of this routine will just overwrite registers within
 *          the pcie rpss.
 */
void plat_overrides_post_rp_ready(pcie_ss_entity_t* rpss);

/**
 *  @brief Choose phy programming settings based on platform type.
 *
 *  @param[in] None
 *
 * @return Boolean value to enable/disable phy programming using silibs.
 */
bool plat_get_phy_programming_support();
