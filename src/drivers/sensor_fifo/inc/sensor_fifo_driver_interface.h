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
#include <fpfw_status.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/
typedef struct {
    uint8_t  device_fifo_id;  ///< fifo identifier for specific device register access
    uint8_t  fifo_enabled;
    uint16_t entry_count;
    uint16_t entry_size_bytes;
    uint16_t stride_size_bytes;
    uint32_t start_address;
    uint32_t end_address;
    char*    name;
} sensor_fifo_device_properties_t,*psensor_fifo_device_properties_t;

typedef struct __attribute__((aligned(4))) {
    DFWK_DEVICE_HEADER base_device;
    DFWK_REQUEST_DISPATCH_SYNC dispatch_sync;
    bool initialized;

    sensor_fifo_device_properties_t* fifo_property_table;

} sensor_fifo_device_t;

typedef struct __attribute__((aligned(4))) {
    DFWK_INTERFACE_HEADER base_interface;
    sensor_fifo_device_t* device;
} sensor_fifo_driver_interface_t, *psensor_fifo_driver_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the Interface
 *
 * @param[in] driver_interface - uninitialized driver interface that will be initialized by this API
 * @param[in] device - To a sensor_fifo_device_t that has previously been initialized
 */
void sensor_fifo_driver_inf_init(sensor_fifo_driver_interface_t* driver_interface, sensor_fifo_device_t* device);

fpfw_status_t sensor_fifo_driver_inf_read_reg(sensor_fifo_driver_interface_t* driver_interface,
                                                   uint32_t address,    uint32_t* value);

fpfw_status_t sensor_fifo_driver_inf_write_reg(sensor_fifo_driver_interface_t* driver_interface, uint32_t address, uint32_t value);