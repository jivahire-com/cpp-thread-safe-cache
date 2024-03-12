//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file cortex_m7.h
 *  cortex_m7 MOCKS for unit testing. Only built for win32
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

uint32_t cortex_m7_atomic_increment(volatile uint32_t* address, uint32_t value);

uint32_t cortex_m7_atomic_decrement(volatile uint32_t* address, uint32_t value);

uint32_t cortex_m7_atomic_compare_exchange(volatile uint32_t* address, uint32_t value, uint32_t compare);
