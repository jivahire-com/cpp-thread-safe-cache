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
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief This function walks rpss intus and enqueues completions for
 *         outstanding PCIe requests based on intu status. Once done, it
 *         clears stale intu status bits.
 *
 *  @param[in] irq_num
 *             IRQ number in response to which rpss handling was requested.
 *             This can be any one of the four VAB IRQs which map to SCP.
 *
 *  @return
 *      None. Status will be conveyed through request completions.
 */
void rpss_irq_callback(uint32_t irq_num);

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
