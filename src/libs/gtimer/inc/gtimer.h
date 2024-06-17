//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file gtimer.h
 *  Initializes various gtimer and system counter components
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *
 *    The function initializes the system counter
 *    @param counter_control_base - base address of the counter control register
 *    @param frequency_hz - frequency of the counter
 *    @param scaling_factor - scaling factor for the counter
 * 
 *    @retval none
 */
void gtimer_init(uint32_t counter_control_base, uint32_t frequency_hz, uint8_t scaling_factor);