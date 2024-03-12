//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cortex_m7_mocks.c
 * cortex_m7 MOCKS for unit testing. Only built for win32
 */

/*------------- Includes -----------------*/

#include <FpFwCMocka.h>
#include <cortex_m7.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

uint32_t cortex_m7_atomic_increment(volatile uint32_t* address, uint32_t value)
{
    return (*address) + value;
}

uint32_t cortex_m7_atomic_decrement(volatile uint32_t* address, uint32_t value)
{
    return (*address) - value;
}

uint32_t cortex_m7_atomic_compare_exchange(volatile uint32_t* address, uint32_t value, uint32_t compare)
{
    if (*address == compare)
    {
        *address = value;
        return compare;
    }

    return *address;
}