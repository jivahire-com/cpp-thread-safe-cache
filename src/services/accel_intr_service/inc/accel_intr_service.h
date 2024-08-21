//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr.h
 * Header file for Accel Interrupt Service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "DfwkPtrTypes.h"             // for PDFWK_SCHEDULE
#include "accel_intr_service_dfwk.h"  // for paccel_intr_service_t, paccel_i...

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Init for accel_intr interfaces
 *
 *
 * @param[in] p_device : Device with which interface is initialized
 * @param[in] p_interface : Interface to initialize
 *
 * @retval 
 * void
 */
void accel_intr_service_interface_init(paccel_intr_service_t p_device, paccel_intr_service_interface_t p_interface);

/**
 * @brief Init for accel_intr service : Inits Device and Queue
 *
 *
 * @param[in] p_device : Device with which interface is initialized
 * @param[in] p_schedule : Schedule
 * @param[in] p_config : Configuration
 *
 * @retval 
 * void
 */
void accel_intr_service_init(paccel_intr_service_t p_device, PDFWK_SCHEDULE p_schedule);
