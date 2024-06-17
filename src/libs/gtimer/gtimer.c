//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file gtimer.c
 *  This modules initializes various gtimer and system counter components
 */

/*------------- Includes -----------------*/
#include <refclk_counter_cnt_control_regs.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void gtimer_init(uintptr_t counter_control_base, uint32_t frequency_hz, uint8_t scaling_factor)
{
    // Disable counter before configuring
    vptr_refclk_counter_cnt_control_reg counter_control = (vptr_refclk_counter_cnt_control_reg)counter_control_base;
    counter_control->cntcr.en = 0;

    // Set the frequency of the counter
    counter_control->cntfid0.frequency = frequency_hz;
    counter_control->cntcr.fcreq = 1;
    counter_control->cntincr.incr_value = scaling_factor;

    // Enable the counter
    counter_control->cntcr.en = 1;
}