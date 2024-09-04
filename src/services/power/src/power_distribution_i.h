//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_distribution_i.h
 * Header containing definitions for power resource distribution
 */

#pragma once

/*----------- Nested includes ------------*/
#include "power_i.h"
#include "power_loops_i.h"
#include "power_runconfig_i.h"

#include <dvfs_struct.h> // for NUM_PSTATES
#include <fpfw_status.h>       // for FPFW_STATUS_SUCCESS, FPFW_STATUS_INVA...
#include <kng_error.h>
#include <power_runconfig_i.h>
#include <FpFwAssert.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get minimum plimit based on knobs and collected data
 *
 * \b Description:
 *      Called to determine the minimum acceptable plimit of a core
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 * @param[in] p_cores - Pointer to core structure array
 * @param[in] core - Core number for which to determine minimum plimit
 *
 * @return minimum plimit
 *
 */
uint8_t power_distribution_get_minimum_plimit(power_runconfig_t* p_runconfig, power_cores_t* p_cores, unsigned int core);

/**
 * @brief Distribute available resources in the form of plimit selections
 *
 * \b Description:
 *      Takes various input from ctrl loop / core structures to determine
 * plimits based on current available resources.  Inputs are from global power
 * structures; output is per-core selected plimit, plimits_pending bitfield
 * with bit for each core that has a change pending, and ctrl loop throttling
 * status.
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 *
 * @return none
 *
 */
void power_distribution_distribute_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop);

/**
 * @brief Distribute function used to pre-select specific warmstart plimits
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 *
 * @return none
 *
 */
void power_distribution_distribute_warmstart_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop);

/**
 * @brief Distribute minimum plimit updates
 *
 * \b Description:
 *      It may be necessary to regularly update plimit requests even if plimits
 * don't change.  This function does that based on the minimum_plimit_updates knob.
 *
 * @param[in] p_runconfig - Pointer to runtime configuration structure
 * @param[in] p_ctrlloop - Pointer to control loop data structure
 *
 * @return none
 *
 */
void power_distribution_minimum_plimit_updates(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop);


#ifdef __cplusplus
}
#endif
