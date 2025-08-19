//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * PCIe combined RPSS interrupt service routines and associated structure
 * definitions.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_ss_common.h>
#include <pciess_int.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief This function walks rpss intus and logs out CPERs for all
 *         available error data given the interrupt statuses. Notifier
 *         interrupts are enqueued, and the INTU statuses are cleared prior
 *         to exit.
 *
 *  @param[in] ss
 *             Pointer to the rpss entity struct that ties to the interrupt
 *             domain.
 * 
 *  @param[in] info
 *             INTU probe information to process
 *
 *  @return
 *      None. Status will be conveyed through request completions.
 */
void rpss_irq_callback(pcie_ss_entity_t *ss, pciess_int_probe_t *info);

/**
 *  @brief In case intu status reflects a pcie link up event on an rp, this
 *         callback is invoked so that an associated link training request
 *         completion is sent to the service.
 *
 *  @param[in] ss
 *             Pointer to the rpss entity struct associated with that rp.
 *
 *  @param[in] rp_idx
 *             Pointer to the rp_index within the rpss where this event occurred
 *
 *  @return
 *      None. Status will be conveyed through request completions.
 */
void pcie_link_up_interrupt_callback(pcie_ss_entity_t* ss, uint8_t rp_idx);
