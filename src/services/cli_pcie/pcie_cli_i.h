//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * PCIe CLI helper functions
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <rc4sx16_pf0_type1_hdr_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Helper function to print out a passed in rpss entity structure
 *         in a human readable format - typically used for debugging.
 *
 *  @param[in] ss
 *      Pointer to a pcie_ss_entity_t type structure retrieved from the
 *      pcie driver.
 *
 *  @return
 *      None
 */
void print_rpss_entity(pcie_ss_entity_t* ss);

/**
 *  @brief Helper function to print out a passed in root port entity
 *         structure in a human readable format - typically used
 *         for debugging.
 *
 *  @param[in] ss
 *      Pointer to a pcie_rp_entity_t type structure retrieved from the
 *      pcie driver.
 *
 *  @return
 *      None
 */
void print_rp_entity(pcie_rp_entity_t* rp);

/**
 *  @brief Helper function to print out ltssm and link state information
 *         for a given rpss + root port combination.
 *
 *  @param[in] rpss_idx
 *      Root port subsystem index passed in by the user
 *
 *  @param[in] rp_idx
 *      Root port index (within the rpss) passed in by the user
 *
 *  @param[in] ltssm_state
 *      LTSSM information for this root port retrieved from the driver
 *
 *  @param[in] link_state
 *      Link state (speed and width) information for this root port
 *      retrieved from the driver
 *
 *  @return
 *      None
 */
void print_link_state(RPSS_INSTANCE rpss_idx, uint8_t rp_idx, PCIE_LTSSM_STATE ltssm_state, pcie_link_state_t* link_state);

/**
 *  @brief Helper function to print out ltssm and link state information
 *         for a given rpss + root port combination.
 *
 *  @param[in] rpss_idx
 *      Root port subsystem index passed in by the user
 *
 *  @param[in] rp_idx
 *      Root port index (within the rpss) passed in by the user
 *
 *  @param[in] rp_t1_hdr
 *      Pointer to type 1 configuration header associated with this root
 *      port retrieved from the pcie driver
 *
 *  @return
 *      None
 */
void print_type1_rp_header(RPSS_INSTANCE rpss_idx, uint8_t rp_idx, rc4sx16_pf0_type1_hdr_reg* rp_t1_hdr);
