//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_remote_die_i.h
 * Header containing definitions for power remote die exchanges
 */

#pragma once

/*----------- Nested includes ------------*/
#include "power_i.h"
#include "power_loops_i.h"
#include "power_runconfig_i.h"

#include <fpfw_status.h>       // for FPFW_STATUS_SUCCESS, FPFW_STATUS_INVA...
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize remote die power exchange
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 *
 */
void power_remote_die_init(power_runconfig_t* p_runconfig);

/**
 * @brief Function to exchange collected data with remote die
 * 
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 */
void power_remote_die_exchange_inputs(power_runconfig_t* p_runconfig);

/**
 * @brief Function to exchange completion data with remote die
 * 
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 */
void power_remote_die_exchange_complete(power_runconfig_t* p_runconfig);

/**
 * @brief Function to reset the remote die exchange state
 * 
 */
void power_remote_die_idle_reset();

#ifdef __cplusplus
}
#endif
