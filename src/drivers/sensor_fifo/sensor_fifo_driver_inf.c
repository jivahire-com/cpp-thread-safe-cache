//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_inf.c
 *
 * Implements the interface API's and calls Driver Framework to pass calls to the platform specific driver.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>                   // for DfwkInterfaceInitialize
#include <FpFwAssert.h>                   // for FPFW_RUNTIME_ASSERT
#include <sensor_fifo_driver_interface.h> // for sensor_fifo_device_t, sens...
#include <stdbool.h>                      // for true
#include <stddef.h>                       // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void sensor_fifo_driver_inf_init(sensor_fifo_driver_interface_t* driver_interface, sensor_fifo_device_t* device)
{
    FPFW_RUNTIME_ASSERT(device != NULL);
    FPFW_RUNTIME_ASSERT(device->initialized == true);
    FPFW_RUNTIME_ASSERT(device->dispatch_sync != NULL);

    DfwkInterfaceInitialize(&(driver_interface->base_interface), &(device->base_device), NULL, device->dispatch_sync);
    driver_interface->device = device;
}