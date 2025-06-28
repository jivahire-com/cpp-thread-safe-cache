//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file systick_update.c
 * This file will implement the functionality of systick_update
 */

/*-------------------------------- Includes ---------------------------------*/
#include <cmsis_m7.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <kng_error.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <systick_update.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
// TODO - Move this into config system knob
#define SVP_REFCLK_FREQUENCY  125000000U
#define FPGA_REFCLK_FREQUENCY 10000000U
#define SOC_REFCLK_FREQUENCY  1000000000U

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
static uint32_t emcpu_clock;
static uint32_t systick_reload_val;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
int32_t systick_update_init(void)
{
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        emcpu_clock = SVP_REFCLK_FREQUENCY;
        break;
    case PLATFORM_RVP_EVT_SILICON:
        emcpu_clock = SOC_REFCLK_FREQUENCY;
        break;
    default:
        emcpu_clock = FPGA_REFCLK_FREQUENCY;
        break;
    }

    systick_reload_val = (emcpu_clock / TX_TIMER_TICKS_PER_SECOND) - 1;

    SysTick->LOAD = systick_reload_val;
    SysTick->VAL = 0;

    // Confirm that the systick timer is updated
    if (SysTick->LOAD != systick_reload_val)
    {
        return KNG_E_FAIL;
    }

    return KNG_SUCCESS;
}

uint32_t systick_get_reload_val(void)
{
    return systick_reload_val;
}

uint32_t systick_get_emcpu_clock(void)
{
    return emcpu_clock;
}