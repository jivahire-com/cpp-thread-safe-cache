//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_pldm_scp.h
 * This file contains the definitions and declarations for the Power PLDM SCP service.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include "power_runconfig_i.h"
#include <pldm_common_power.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the Power PLDM service. Only for Die 0
 *
 * @param p_runconfig Pointer to the power run configuration.
 */
void power_pldm_service_init(power_runconfig_t* p_runconfig);

/**
 * @brief Handle the performance state request from MCP. Called inside ICC receive callback in SCP0.
 *
 * @param p_throt_request Pointer to the performance state request of type `icc_pwr_throt_state_request_t`.
 * @return pldm_performance_states_t normal, throttled, degraded
 */
pldm_performance_states_t handle_performance_state_request(icc_pwr_throt_state_request_t* p_throt_request);

/**
 * @brief Handle the power cap request from MCP. Called inside ICC receive callback in SCP0.
 *
 * @param p_cap_request Pointer to the power cap request.
 * @return uint16_t The requested power cap after processing.
 */
uint16_t handle_power_cap_request(icc_pwr_cap_request_t* p_cap_request);