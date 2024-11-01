//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file gtimer_prodfw.h
 *  Initializes various gtimer and system counter components
 */

#pragma once

/*--------------- Includes ---------------*/
#include <fpfw_tmr_queue.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/*
 *    @param counter_control_base - base address of the counter control register
 *    @param frequency_hz - frequency of the counter
 *    @param scaling_factor - scaling factor for the counter
 *    @param timer_control_base - base address of the timer control register
 *    @param timer_base_address - base address of the timer register
 */
typedef struct {
    uintptr_t counter_control_base;
    uintptr_t timer_control_base;
    uintptr_t timer_base_address;
    uint32_t frequency_hz;
    uint8_t scaling_factor;
    uint32_t timer_irq;
} gtimer_prodfw_init_config_t;

/*--------- Function Prototypes ----------*/
// TODO: Conform to fpfw_timer interface instead
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2149866

/**
 *
 *    The function initializes the system counter
 *    @param config - configuration for the gtimer
 * 
 *    @retval none
 */
void gtimer_prodfw_init(gtimer_prodfw_init_config_t* config);

/**
 *   This function adds a one-shot timer
 *   @param tmr - timer entry
 *   @param tick_interval - interval in ticks
 *   @param cb - callback function (void* ctx, uint64_t exp_tick, uint64_t now_tick)
 *      exp_tick - expiry tick
 *      now_tick - current tick
 *   @param ctx - context
 */
void gtimer_add_oneshot(fpfw_tmr_entry_t* tmr, uint64_t tick_interval, void (*cb)(void* ctx, uint64_t exp_tick, uint64_t now_tick), void* ctx);

/**
 *   This function adds a periodic timer
 *   @param tmr - timer entry
 *   @param tick_interval - interval in ticks
 *   @param cb - callback function (void* ctx, uint64_t exp_tick, uint64_t now_tick)
 *      exp_tick - expiry tick
 *      now_tick - current tick
 *   @param ctx - context
 * 
 */
void gtimer_add_periodic(fpfw_tmr_entry_t* tmr, uint64_t tick_interval, void (*cb)(void* ctx, uint64_t exp_tick, uint64_t now_tick), void* ctx);

/**
 * Removes a timer from the queue
 * 
 * @param   tmr pointer to the timer entry
 */
void gtimer_remove(fpfw_tmr_entry_t* tmr);

/**
 * Get the frequency of the gtimer
 * 
 * @return  frequency of the gtimer
 */
uint32_t gtimer_prodfw_get_frequency();