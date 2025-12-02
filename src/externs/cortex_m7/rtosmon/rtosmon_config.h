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
 * @param[out]: None
 */
void rtosmon_plugin_init(void);

/**
 * @brief This API sets _tx_execution_time_source to a valid source to avoid
 * crashes in tx_execution_profle APIs
 * @param[in]: None
 * @param[out]: None
 */
void rtosmon_disabled_init();
