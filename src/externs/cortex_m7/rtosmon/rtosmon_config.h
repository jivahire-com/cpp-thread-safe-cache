#pragma once

#include <stddef.h>              // for NULL
#include <stdint.h>

//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
* @file rtosmon_config.h
* @brief Declarations for RTOSMon configuration and initialization
* This header exposes the initialization entry point used to configure
* and start the RTOSMon plugin on Cortex-M7 (SCP core).
*
*/

/*------------- Nested Includes -----------------*/
#include <stdint.h>
#include <stddef.h>              // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/**
 * @Brief: Initialize RTOSMon plugin for the SCP Cortex-M7 core.
 * @param[in]: None
 * @params[out]: Initializes internal RTOSMon data structures; no direct outputs
 */
void rtosmon_plugin_init(void);