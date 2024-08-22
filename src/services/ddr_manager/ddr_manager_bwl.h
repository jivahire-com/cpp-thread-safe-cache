//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_bwl.h
 * DDR Manager BWL APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Retrieves the state of the BWL (Bandwidth Limiter) in the DDR Manager.
 *
 * @return true if the BWL is enabled, false otherwise.
 */
bool ddr_manager_get_bwl_engaged();

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