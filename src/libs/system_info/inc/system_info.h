//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file system_info.h
 *  APIs for clients to get system information
 */

#pragma once

/*--------------- Includes ---------------*/
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Checks if the HSP (Hardware Security Processor) is present.
 * @note This function should be called after system_info_init() is called.
 * @return true if the HSP is present, false otherwise.
 */
bool system_info_is_hsp_present();

/**
 * @brief Caches the system information.
 * 
 */
void system_info_init();