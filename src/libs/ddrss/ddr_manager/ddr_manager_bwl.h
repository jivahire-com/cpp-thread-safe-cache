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

// BWL State bitmask values
typedef enum
{
    BWL_STATE_DISABLED = 0,
    BWL_STATE_ENABLED_I3C = 0x1,
    BWL_STATE_ENABLED_MR4 = 0x2,
    BWL_STATE_ENABLED_FORCED = 0x4,
} bwl_state_t;

/* Verify that BWL state values stay aligned with DIMM throttle sources */
/* Cast to int to avoid -Wdeprecated-enum-compare diagnostics */
_Static_assert((int)BWL_STATE_DISABLED       == (int)DIMM_THROTTLE_SOURCE_NONE,   "BWL/DIMM throttle source mismatch");
_Static_assert((int)BWL_STATE_ENABLED_I3C    == (int)DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR,    "BWL/DIMM throttle source mismatch");
_Static_assert((int)BWL_STATE_ENABLED_MR4    == (int)DIMM_THROTTLE_SOURCE_MR4,    "BWL/DIMM throttle source mismatch");
_Static_assert(((int)BWL_STATE_ENABLED_I3C | (int)BWL_STATE_ENABLED_MR4) == (int)DIMM_THROTTLE_SOURCE_BOTH, "BWL/DIMM throttle source mismatch");

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