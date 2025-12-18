//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_bdat_i.h
 *
 *       Provides functions to publish PCIe BDAT data
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pciess_common.h>
#include <silibs_status.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/*
 * Publish BDAT information for a given root port. This function is expected to
 * be called only once per RP in a boot cycle and will be called after link up.
 *
 * It will call silibs APIs to get information such as figure of merit (FoM) for
 * all the lanes owned by this RP and populate the BDAT structure for this RP in
 * DDR.
 *
 * @param[in] rpss      Pointer to the RPSS entity
 * @param[in] rp_index  Index of the RP in the RPSS for which BDAT info is to be
 *                      published.
 *
 * @return SILIBS_SUCCESS on success, error code otherwise.
 */
silibs_status_t publish_pcie_bdat_info_for_this_rp(pcie_ss_entity_t* rpss, uint8_t rp_index);

/*
 * Clear out the combined BDAT reserved region in DDR. This function is expected
 * to be called only once per boot cycle.
 *
 * @return SILIBS_SUCCESS on success, error code otherwise.
 */
silibs_status_t clear_out_combined_bdat_rsvd_region(void);
