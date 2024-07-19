//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_service_init.h
 * Sensor Fifo Service Interface
 */

#pragma once

/*----------- Nested includes ------------*/
#include <sensor_fifo_driver_interface.h>


/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the service.
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] driver_interface - Pointer to a platform driver interface
 */
void sensor_fifo_svc_initialize(sensor_fifo_driver_interface_t* driver_interface);
