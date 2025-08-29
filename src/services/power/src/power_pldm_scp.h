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