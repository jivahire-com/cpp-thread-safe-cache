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

/*-- Symbolic Constant Macros (defines) --*/
#define VAB_RPSS_COMBINED_INT (BIT12)
#define VAB_INTU_CLEAR_MASK   (0xffffffff)

/*-------------- Typedefs ----------------*/
typedef struct _vab_isr_ctx_t
{
    uint32_t vab_irq_num;
    uintptr_t intu0_base;
    uint32_t intu0_sts;
    uintptr_t intu1_base;
    uint32_t intu1_sts;
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
