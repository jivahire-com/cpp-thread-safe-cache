//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utils.h
 * This file contains some common macros, variables and inline
 * functions used across the project that is specific to
 * MSCP and accelerators firmware.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define KB          (1024)
#define MB          (KB * 1024)
#define UNUSED(x)   (void)(x)
#define PLACED_CODE __attribute__((section(".placed.code")))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *
 * Initialize delay utils with the proper CPU frequency and counter source.
 * Must be called once after the generic timer is initialized (from gtimer_init_internal).
 *
 *    @param[in] cpu_freq_hz
 *              CPU/counter frequency in Hz.
 *
 *    @param[in] get_counter
 *              Function that returns the current 64-bit hardware counter value.
 *
 *    @retval none
 */
void delay_init(uint32_t cpu_freq_hz, uint64_t (*get_counter)(void));

/**
 *
 * This function is used to sleep (blocking) in milliseconds.
 *
 * Uses the gtimer for wall-clock accurate timing.
 * Falls back to a NOP loop if called before gtimer is initialized.
 * The bootloader has its own independent copy in kingsgate_boot.c.
 *
 *    @param[in] milliseconds
 *              Sleep duration (blocking) in milliseconds.
 *
 *    @retval none
 */
void sleep_ms(uint32_t milliseconds);
