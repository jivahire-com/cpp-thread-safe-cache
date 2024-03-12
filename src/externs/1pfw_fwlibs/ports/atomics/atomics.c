//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file atomics.c
 *  This modules provides platform implementation for the atomics library.
 */

/*------------- Includes -----------------*/

#include <AtomicOperations.h>
#include <cortex_m7.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

uint32_t FPFwAtomicCompareExchange(volatile uint32_t* address, uint32_t value, uint32_t compare)
{
    return cortex_m7_atomic_compare_exchange(address, value, compare);
}

uint32_t FPFwAtomicAdd(volatile uint32_t* address, uint32_t increment)
{
    return cortex_m7_atomic_increment(address, increment);
}

uint32_t FPFwAtomicSubtract(volatile uint32_t* address, uint32_t decrement)
{
    return cortex_m7_atomic_decrement(address, decrement);
}