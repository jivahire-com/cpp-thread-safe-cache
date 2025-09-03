//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_bwl.h
 * DDR Manager BWL APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <sensor_fifo_service.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    uint16_t low;
    uint16_t high;
    uint16_t crit;
} ddr_dimm_temp_thresholds_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Retrieves the state of the BWL (Bandwidth Limiter) in the DDR Manager.
 *
 * @return true if the BWL is enabled, false otherwise.
 */
bool ddr_manager_get_bwl_engaged();

/**
 * @brief Retrieves the current BWL state.
 *
 * @return The current BWL state as a bitmask.
 */
uint8_t ddr_manager_get_bwl_state();

/**
 * @brief Enables the BWL from I3C polling
 */
void ddr_manager_enable_bwl_i3c();

/**
 * @brief Disables the BWL from I3C polling
 */
void ddr_manager_disable_bwl_i3c();

/**
 * @brief Enables the BWL from MR4 interrupt
 */
void ddr_manager_enable_bwl_mr4();

/**
 * @brief Disables the BWL from MR4 interrupt
 */
void ddr_manager_disable_bwl_mr4();

/**
 * @brief Forces the BWL to be enabled
 */
void ddr_manager_enable_bwl_force();

/**
 * @brief Disables the forced BWL .
 */
void ddr_manager_disable_bwl_force();