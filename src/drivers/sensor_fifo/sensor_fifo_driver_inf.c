//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_inf.c
 *
 * Implements the interface API's and calls Driver Framework to pass calls to the platform specific driver.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>                     // for DfwkInterfaceSendSync
#include <FpFwAssert.h>                     // for FPFW_RUNTIME_ASSERT
#include <fpfw_status.h>                    // for fpfw_status_t, FPFW_STAT...
#include <sensor_fifo_driver_interface.h>   // for sensor_fifo_driver_inter...
#include <sensor_fifo_driver_interface_i.h> // for sensor_fifo_drv_inf_read...
#include <stdbool.h>                        // for true
#include <stddef.h>                         // for NULL
#include <stdint.h>                         // for uint32_t

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

fpfw_status_t sensor_fifo_driver_inf_read_reg(sensor_fifo_driver_interface_t* driver_interface, uint32_t address, uint32_t* value)
{
    if ((NULL == driver_interface) || (NULL == value))
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_read_reg_request read_req;

    read_req.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_READ_REG;
    read_req.input.address = address;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &read_req.header);
    if (status == FPFW_STATUS_SUCCESS)
    {
        *value = read_req.output.value;
    }
    return status;
}

fpfw_status_t sensor_fifo_driver_inf_write_reg(sensor_fifo_driver_interface_t* driver_interface, uint32_t address, uint32_t value)
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_write_reg_request write_req;

    write_req.header.RequestType = SENSOR_FIFO_SYNC_REQUEST_WRITE_REG;
    write_req.input.address = address;
    write_req.input.value = value;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &write_req.header);

    return status;
}
