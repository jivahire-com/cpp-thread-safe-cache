//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_service_i.h
 * Private header for sensor service internals
 */

#pragma once

/*----------- Nested includes ------------*/
#include <sensor_fifo_driver_interface.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern psensor_fifo_driver_interface_t pSensor_fifo_driver_inf;

/*--------- Function Prototypes ----------*/
sensor_ram_poll_status_t fifo_poll_helper(DEVICE_FIFO_ID fifo_id, size_t entry_size, uint8_t* dest_data, uint16_t* stride_index);