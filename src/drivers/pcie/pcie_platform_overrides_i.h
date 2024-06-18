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
