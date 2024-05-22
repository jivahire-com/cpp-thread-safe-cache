//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Declares VAB initialization APIs. Intended to be used by component
 * initialization code to initialize and bring-up all the VABs.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Brings a passed set of VABs out of reset and carries out any
 *         required programming to initialize them.
 *
 *  @note This function will block till all VABs are initialized and out of
 *        reset.
 *
 *  @param[in] vab_instances_to_init
 *             Bitmap of all VAB instances to be initialized. VAB bit positions
 *             correspond to SUBSYSTEM_WITH_VAB_ID as described in
 *             kng_soc_constants.h.
 *
 *  @return
 *      SILIBS_SUCCESS - all VABs successfully initialized
 *      SILIBS_ERROR - VAB initialization failure for one or more VABs.
 */
int vab_init(uint16_t vab_instances_to_init);
