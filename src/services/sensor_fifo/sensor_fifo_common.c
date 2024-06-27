//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_common.c
 * Contains functionality that is common to all fifo id's.
 */

/*------------- Includes -----------------*/
#include "sensor_fifo_driver_interface.h" // for psensor_fifo_device_proper...
#include "sensor_fifo_service.h"          // for SENSOR_FIFO_ID, SENSOR_FIF...

#include <DfwkClient.h> // for DfwkClientInterfaceOpen
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <stddef.h>     // for NULL
#include <stdint.h>     // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
psensor_fifo_driver_interface_t pSensor_fifo_driver_inf;

/*------------- Functions ----------------*/
void sensor_fifo_svc_initialize(sensor_fifo_driver_interface_t* driver_interface)
{
    pSensor_fifo_driver_inf = driver_interface;

    int32_t status = DfwkClientInterfaceOpen(&driver_interface->base_interface);

    FPFW_RUNTIME_ASSERT(status == DFWK_SUCCESS);
}

void sensor_fifo_svc_get_properties(SENSOR_FIFO_ID fifo, psensor_fifo_properties_t properties)
{
    FPFW_RUNTIME_ASSERT(fifo < SENSOR_FIFO_MAX_ID);
    FPFW_RUNTIME_ASSERT(properties != NULL);

    psensor_fifo_device_properties_t device_properties = &pSensor_fifo_driver_inf->device->fifo_property_table[fifo];

    properties->entry_size_bytes = device_properties->entry_size_bytes;
    properties->stride_size_bytes = device_properties->stride_size_bytes;
    properties->start_address = device_properties->start_address;
    properties->end_address = device_properties->end_address;
    properties->epoch_count = device_properties->entry_count;
    properties->name = device_properties->name;
}
