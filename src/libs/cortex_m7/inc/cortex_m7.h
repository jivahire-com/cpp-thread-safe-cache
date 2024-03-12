//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file cortex_m7.h
 *  Cortex M7 functions
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *    Increments by a given value the 32-bit variable stored in the given pointer
 *    as an atomic operation.
 *
 *     @param[in] address
 *       A pointer to the variable to be incremented.
 *
 *    @param[in] increment
 *      The value to be added
 *
 *     @retval
 *       The function returns the resulting incremented value.
 *
 */
uint32_t cortex_m7_atomic_increment(volatile uint32_t* address, uint32_t value);

/**
 *    Decrements by a given value the 32-bit variable stored in the given pointer
 *    as an atomic operation.
 * 
 *     @param[in] decrement
 *       The value to be subtracted
 *
 *     @retval
 *       The function returns the resulting decremented value.
 *
 */
uint32_t cortex_m7_atomic_decrement(volatile uint32_t* address, uint32_t value);

/**
 *
 *    The function compares two specified 32-bit values and exchanges with
 *    another 32-bit value based on the outcome of the comparison.
 *
 *    @param[in] address
 *       A pointer to the destination value
 *
 *    @param[in] value
 *      The exchange value. Will be written if the contents of
 *      Destination matches Compare.
 *
 *    @param[in] compare
 *      The value to compare to Destination.
 *
 *    @retval compare
 *      Values did match, Destination contains new value now.
 *
 *    @retval Otherwise
 *      Value did not match, this is the current content of
 *     *Destination.
 *
 */
uint32_t cortex_m7_atomic_compare_exchange(volatile uint32_t* address, uint32_t value, uint32_t compare);
