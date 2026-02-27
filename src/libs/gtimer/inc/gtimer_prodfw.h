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
    bool d2d_sync_point_required;
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
 *   This function adds a periodic timer whose expiry grid is anchored to the
 *   system counter epoch (tick 0) rather than the current counter value.
 *
 *   Unlike gtimer_add_periodic(), which anchors the first expiry relative to
 *   "now" (the counter value at call time), this function uses an absolute
 *   init tick of 0. The timer queue computes the next expiry as:
 *       0 + n * tick_interval >= now_tick   (for the smallest n)
 *   If now_tick is exactly on a grid boundary (e.g., now_tick == k * tick_interval),
 *   the timer will fire at k * interval rather than skipping to (k+1) * interval.
 *
 *   This guarantees that all callers sharing the same synchronized system
 *   counter -- even across dies with software scheduling jitter -- will fire
 *   on identical absolute ticks (0, interval, 2*interval, ...).
 *
 *   Typical use case: cross-die synchronization of power loop timers on SCP0
 *   and SCP1 where the system counters are aligned via d2d sync but timer
 *   setup may occur at different wall-clock times on each die.
 *
 *   @param tmr - timer entry
 *   @param tick_interval - interval in ticks
 *   @param cb - callback function (void* ctx, uint64_t exp_tick, uint64_t now_tick)
 *      exp_tick - expiry tick
 *      now_tick - current tick
 *   @param ctx - context
 */
void gtimer_add_abs_init_periodic(fpfw_tmr_entry_t* tmr, uint64_t tick_interval, void (*cb)(void* ctx, uint64_t exp_tick, uint64_t now_tick), void* ctx);

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

/**
 * Get the current counter value of the gtimer
 *
 * @return  current counter value of the gtimer
 */
uint64_t gtimer_prodfw_get_counter();

/**
 * @brief Get the current timestamp in microseconds
 * 
 * @return uint64_t 
 */
uint64_t gtimer_get_timestamp_us();

/**
 * @brief Get the current timestamp in milliseconds
 * 
 * @return uint64_t 
 */
uint64_t gtimer_get_timestamp_ms();

/**
 * @brief Utility function to get the timer base address, which can be used for gtimer init state
 * 
 * @return uintptr_t - timer base address
 */
uintptr_t gtimer_prodfw_get_timer_base_address();
