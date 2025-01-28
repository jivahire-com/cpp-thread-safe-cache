//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * VAB interrupt structures and routines
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <vab_intu.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct _vab_isr_ctx_t
{
    uint32_t vab_irq_num;
    uintptr_t vab_base;
    vabss_int_probe_t probe;
    void (*process_probe)(uint32_t irq, vabss_int_probe_t* probe);
} vab_isr_ctx_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief VAB interrupt service routine.
 *
 *  @param[in] param
 *             Interrupt context passed in to handle the ISR.
 *             Is of type vab_isr_ctx_t
 *
 *  @return    None
 */
void vab_isr(void* param);

/**
 *  @brief Process the probe returned after probing RPSS VAB intus
 *
 *  @param[in] irq
 *             VAB IRQ number which the probe is associated with
 *
 *  @param[in] probe
 *             Populated vab intu probe containing the status of all
 *             intu 0 and intu 1 pins
 *
 *  @return    None
 */
void process_rpss_probe(uint32_t irq, vabss_int_probe_t* probe);

/**
 *  @brief Process the probe returned after probing SDMSS VAB intus
 *
 *  @param[in] irq
 *             VAB IRQ number which the probe is associated with
 *
 *  @param[in] probe
 *             Populated vab intu probe containing the status of all
 *             intu 0 and intu 1 pins
 *
 *  @return    None
 */
void process_sdmss_probe(uint32_t irq, vabss_int_probe_t* probe);

/**
 *  @brief Process the probe returned after probing CDEDSS VAB intus
 *
 *  @param[in] irq
 *             VAB IRQ number which the probe is associated with
 *
 *  @param[in] probe
 *             Populated vab intu probe containing the status of all
 *             intu 0 and intu 1 pins
 *
 *  @return    None
 */
void process_cdedss_probe(uint32_t irq, vabss_int_probe_t* probe);
