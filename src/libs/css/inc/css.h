//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file css.h
 *  Initializes various CSS components (such as clocks, PIK, PPU) for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *
 *    The function initializes various CSS components needed by SCP before bringing up the mesh
 *    It configures the following:
 *              - CSS PCR
 *              - MSXP PCR
 *              - Systop PPU
 *              - SCP and system PIK
 * 
 *    @retval none
 */
void css_pre_mesh_init();

/**
 *
 *    The function initializes various CSS components needed by SCP after bringing up the mesh
 *    It configures the following:
 *              - Peripheral PCR
 * 
 *    @retval none
 */
void css_post_mesh_init();