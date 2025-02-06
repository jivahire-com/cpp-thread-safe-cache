//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_inf.c
 *
 * Implements the interface API's and calls Driver Framework to pass calls to the platform specific driver.
 */

/*------------- Includes -----------------*/
#include "device_fifo_id.h"                 // for DEVICE_FIFO_ID, DEVICE_F...
#include "sensor_fifo_driver_interface.h"   // for sensor_fifo_driver_inter...
#include "sensor_fifo_driver_interface_i.h" // for sensor_fifo_drv_inf_read...

#include <DfwkDriver.h>  // for DfwkInterfaceSendSync
#include <FpFwAssert.h>  // for FPFW_RUNTIME_ASSERT
#include <fpfw_status.h> // for fpfw_status_t, FPFW_STAT...
#include <stdbool.h>     // for true
#include <stddef.h>      // for NULL
#include <stdint.h>      // for uint32_t

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

fpfw_status_t sensor_fifo_driver_inf_set_global_hw_enable(sensor_fifo_driver_interface_t* driver_interface, bool enable)
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_global_enable global_enable_req;

    global_enable_req.header.RequestType = SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE;
    global_enable_req.input.enable = enable;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &global_enable_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_inf_set_fifo_enable(sensor_fifo_driver_interface_t* driver_interface,
                                                     DEVICE_FIFO_ID fifo_id,
                                                     bool enable,
                                                     bool (*is_enabled)[DEVICE_FIFO_MAX_ID])
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_fifo_enable fifo_enable_req;

    fifo_enable_req.header.RequestType = SENSOR_FIFO_SYNC_SET_FIFO_ENABLE;
    fifo_enable_req.input.fifo_id = fifo_id;
    fifo_enable_req.input.enable = enable;
    fifo_enable_req.output.is_enabled = is_enabled;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &fifo_enable_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_inf_sync_fifo_enables(sensor_fifo_driver_interface_t* driver_interface,
                                                       bool (*is_enabled)[DEVICE_FIFO_MAX_ID])
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_sync_fifo_enables fifo_sync_enable_req;

    fifo_sync_enable_req.header.RequestType = SENSOR_FIFO_SYNC_SYNCHRONIZE_FIFO_ENABLES;
    fifo_sync_enable_req.input.is_enabled = is_enabled;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &fifo_sync_enable_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_inf_update_write_stride(sensor_fifo_driver_interface_t* driver_interface, DEVICE_FIFO_ID fifo_id)
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_update_write_stride update_stride_req;

    update_stride_req.header.RequestType = SENSOR_FIFO_SYNC_UPDATE_WRITE_PTR;
    update_stride_req.input.fifo_id = fifo_id;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &update_stride_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_inf_is_enabled(sensor_fifo_driver_interface_t* driver_interface,
                                                bool (*is_enabled)[DEVICE_FIFO_MAX_ID])
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_fifo_is_enabled is_enabled_req;

    is_enabled_req.header.RequestType = SENSOR_FIFO_SYNC_QUERY_IS_ENABLED;
    is_enabled_req.output.is_enabled = is_enabled;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &is_enabled_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_inf_is_empty(sensor_fifo_driver_interface_t* driver_interface,
                                              bool (*is_empty)[DEVICE_FIFO_MAX_ID])
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_fifo_is_empty is_empty_req;

    is_empty_req.header.RequestType = SENSOR_FIFO_SYNC_QUERY_IS_EMPTY;
    is_empty_req.output.is_empty = is_empty;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &is_empty_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_write_entries(sensor_fifo_driver_interface_t* driver_interface,
                                               DEVICE_FIFO_ID fifo_id,
                                               uint8_t* src_data,
                                               size_t entry_size,
                                               uint16_t num_entries,
                                               uint16_t stride_index)
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_write_entry write_entry_req;
    write_entry_req.input.fifo_id = fifo_id;
    write_entry_req.input.src_data = src_data;
    write_entry_req.input.entry_size = entry_size;
    write_entry_req.input.num_entries = num_entries;
    write_entry_req.input.stride_index = stride_index;

    write_entry_req.header.RequestType = SENSOR_FIFO_SYNC_WRITE_ENTRY;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &write_entry_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_read_entry(sensor_fifo_driver_interface_t* driver_interface,
                                            DEVICE_FIFO_ID fifo_id,
                                            size_t entry_size,
                                            uint8_t* dest_data,
                                            uint16_t* num_entries_read,
                                            uint16_t* num_entries_remaining,
                                            uint16_t* stride_index)
{
    if (NULL == driver_interface)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }
    sensor_fifo_drv_inf_read_entry read_entry_req;
    read_entry_req.input.fifo_id = fifo_id;
    read_entry_req.input.entry_size = entry_size;

    read_entry_req.output.dest_data = dest_data;
    read_entry_req.output.num_entries_read = num_entries_read;
    read_entry_req.output.num_entries_remaining = num_entries_remaining;
    read_entry_req.output.stride_index = stride_index;

    read_entry_req.header.RequestType = SENSOR_FIFO_SYNC_READ_ENTRY;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &read_entry_req.header);

    return status;
}

fpfw_status_t sensor_fifo_driver_reset(sensor_fifo_driver_interface_t* driver_interface)
{
    DFWK_SYNC_REQUEST_HEADER reset_req;
    reset_req.RequestType = SENSOR_FIFO_SYNC_RESET;

    fpfw_status_t status = DfwkInterfaceSendSync(&driver_interface->base_interface, &reset_req);

    return status;
}
