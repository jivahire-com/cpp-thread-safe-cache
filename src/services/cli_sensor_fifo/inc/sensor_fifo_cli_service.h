//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_cli_service.h
 *
 * Sensor Fifo Service CLI
 */

#pragma once

/*----------- Nested includes ------------*/
#include <sensor_fifo_driver_interface.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Connect the CLI service to the same interface as sensor fifo service.  That will handle multi-threaded access.
 *
 * @param[in] driver_interface - driver interface for this platform
 */
void sensor_fifo_cli_svc_initialize(sensor_fifo_driver_interface_t* driver_interface);