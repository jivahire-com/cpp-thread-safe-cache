//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file vab_atu_mappings.h
 * Set and get persistent ATU mapped addresses for VABs and RPSS.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <kng_soc_constants.h>
#include <silibs_status.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Maps a VAB instance using the ATU
 *
 * @param vab_id The VAB instance identifier to be mapped
 *
 * @return  SILIBS_SUCCESS: Mapping completed successfully
 * @return  SILIBS_ERROR: ATU mapping error
 */
silibs_status_t map_vab_instance(uint16_t vab_id);

/**
 * @brief Gets the resolved base address for a VAB instance
 *
 * @param vab_id The VAB instance identifier
 *
 * @return  The resolved base address for the VAB instance
 */
uintptr_t get_vab_resolved_base(uint16_t vab_id);

/**
 * @brief Gets the resolved base address for a RPSS instance
 *
 * @param rpss_id The RPSS instance identifier
 *
 * @return  The resolved base address for the RPSS instance
 */
uintptr_t get_rpss_resolved_base(RPSS_INSTANCE rpss_id);
