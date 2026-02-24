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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DEFAULT_CPU_FREQ_HZ  (1000000000U)
#define DEFAULT_TICKS_PER_MS (DEFAULT_CPU_FREQ_HZ / 1000U)

/*------------- Typedefs -----------------*/
typedef struct
{
    uint64_t (*get_counter)(void);
    uint32_t ticks_per_ms;
    bool initialized;
} delay_context_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static delay_context_t s_delay = {0};

/*------------- Functions ----------------*/
void delay_init(uint32_t cpu_freq_hz, uint64_t (*get_counter)(void))
{
    if (cpu_freq_hz != 0 && get_counter != NULL)
    {
        s_delay.ticks_per_ms = cpu_freq_hz / 1000U;
        s_delay.get_counter = get_counter;
        s_delay.initialized = true;
    }
}

void sleep_ms(uint32_t milliseconds)
{
    /*
     * Wall-clock accurate delay using a hardware counter.
     *
     * delay_init() must be called once (from gtimer_init) to provide the
     * counter read function and CPU frequency. After init, spins on the
     * 64-bit hardware counter - no wrapping concerns.
     *
     * If called before delay_init(), falls back to a NOP loop with
     * best-effort timing using DEFAULT_TICKS_PER_MS.
     *
     * The bootloader has its own independent sleep_ms in kingsgate_boot.c.
     */
    if (!s_delay.initialized)
    {
        uint32_t nops_per_ms = DEFAULT_TICKS_PER_MS / 4U;
        for (uint32_t ms = 0; ms < milliseconds; ms++)
        {
            for (volatile uint32_t i = 0; i < nops_per_ms; i++)
            {
                asm volatile("nop");
            }
        }
        return;
    }

    uint64_t start = s_delay.get_counter();
    uint64_t delta = (uint64_t)milliseconds * s_delay.ticks_per_ms;
    uint64_t current;
    do
    {
        current = s_delay.get_counter();
    } while ((current - start) < delta);
}
