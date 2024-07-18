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
 *  @brief Configures all the VAB instances for the given die
 *         VAB instances programmed depends on the platform type
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
int vab_common_init(uint16_t vab_instances_to_init);

/**
 *  @brief Install interrupt handlers for VAB interrupts that are routed to
 *         SCP.
 *
 *  @param[in] vab_instances_to_init
 *             Bitmap of all VAB instances to be initialized. VAB bit positions
 *             correspond to SUBSYSTEM_WITH_VAB_ID as described in
 *             kng_soc_constants.h.
 *
 *  @return    None
 */
void enable_vab_isrs(uint16_t vab_instances_to_init);
