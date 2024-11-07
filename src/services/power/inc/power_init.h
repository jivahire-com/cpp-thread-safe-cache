//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.h
 * Header containing definitions for initialization of power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "power_dfwk.h"
#include "power_runconfig.h"

#include <DfwkTypes.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule, const power_service_config_t* p_config);
void power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface);

/**
 *
 *    Initializes the Power AVS interface  
 *
 *    @param[in]  Interface
 * 
 *    @brief Called for each AVS.
 *
 */
void pwr_avs_initialize(pscp_avs_interface_t pwr_avs_array[]);

#ifdef __cplusplus
}
#endif