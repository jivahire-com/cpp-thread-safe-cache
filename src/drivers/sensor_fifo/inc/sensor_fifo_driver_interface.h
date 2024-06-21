//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_interface.h
 * Defines abstract sensor fifo device and driver interface.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkClient.h>
#include <DfwkCommon.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum SENSOR_FIFO_REQUEST_ID
{
    SENSOR_FIFO_REQUEST_ID_READ_SYNC,
    SENSOR_FIFO_REQUEST_ID_WRITE_SYNC,
} SENSOR_FIFO_REQUEST_ID;


typedef struct __attribute__((aligned(4))) {
    DFWK_DEVICE_HEADER base_device;
    DFWK_REQUEST_DISPATCH_SYNC dispatch_sync;
    bool initialized;

} sensor_fifo_device_t;

typedef struct __attribute__((aligned(4))) {
    DFWK_INTERFACE_HEADER base_interface;
    sensor_fifo_device_t* device;
} sensor_fifo_driver_interface_t;


/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the Interface
 *
 * @param[in] driver_interface - uninitialized driver interface that will be initialized by this API
 * @param[in] device - pointer to a sensor_fifo_device_t that has previously been initialized
 */
void sensor_fifo_driver_inf_init(sensor_fifo_driver_interface_t* driver_interface, sensor_fifo_device_t* device);