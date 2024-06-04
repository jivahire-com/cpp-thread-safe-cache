//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utils.c
 *
 * This file contains some common macros, variables and inline
 * functions used across the project that is specific to
 * MSCP and accelerators firmware.
 *
 */

/*------------- Includes -----------------*/
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
// This is based on estimate CORTEX M7 runs @ 1GHz
#define CORTEX_M7_ONE_MS_CPU_CYCLES (1000 * 1000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void dummy_function(void)
{
    /* dummy function, do nothing */
}

void sleep_ms(uint32_t milliseconds)
{
    uint32_t count = 0;
    uint32_t i = 0;

    while (count < milliseconds)
    {
        for (i = 0; i < CORTEX_M7_ONE_MS_CPU_CYCLES; i++)
        {
            asm volatile("nop");
        }

        count++;
    }
}
