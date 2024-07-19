//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_interface.h
 * Defines abstract sensor fifo device and driver interface.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "device_fifo_id.h"
#include <DfwkClient.h>
#include <DfwkCommon.h>
#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define STRIDE_INDEX_UNUSED (UINT16_MAX)

/*-------------- Typedefs ----------------*/
typedef struct {
    DEVICE_FIFO_ID  device_fifo_id;  ///< fifo identifier for specific device register access
    uint16_t entry_count;
    uint16_t entry_size_bytes;
    uint16_t stride_size_bytes;
    uintptr_t start_address;
    uintptr_t end_address;
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

/**
 * @brief Read a hardware register
 *
 * @param[in] driver_interface - driver instance
 * @param[in] address - address to read from
 * @param[out] value - value of register
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_read_reg(sensor_fifo_driver_interface_t* driver_interface,
                                                   uint32_t address,    uint32_t* value);
/**
 * @brief Write a hardware registers
 *
 * @param[in] driver_interface - driver instance
 * @param[in] address - address to write to
 * @param[in] value - data to be written to register
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_write_reg(sensor_fifo_driver_interface_t* driver_interface, uint32_t address, uint32_t value);

/**
 * @brief Global Enable/Disable for all hardware fifos
 *
 * @param[in] driver_interface - driver instance
 * @param[in] enable - true - hardware fifo's enabled via sensor_fifo_svc_enable_fifo will collect data
 *                     false - all hardware fifo's are disabled
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_set_global_hw_enable(sensor_fifo_driver_interface_t* driver_interface, bool enable);

/**
 * @brief Enable / Disable a single fifo
 *
 * @param[in] driver_interface - driver instance
 * @param[in] fifo_id - fifo identifier
 * @param[in] enable - true - data may be added to fifo, false, data is not added to fifo
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_set_fifo_enable(sensor_fifo_driver_interface_t* driver_interface, DEVICE_FIFO_ID fifo_id, bool enable);

/**
 * @brief If a fifo's stride size is > entry size, then the write pointer is incremented by the stride size
 *
 * @param[in] driver_interface - driver instance
 * @param[in] fifo_id - fifo identifier
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_update_write_stride(sensor_fifo_driver_interface_t* driver_interface, DEVICE_FIFO_ID fifo_id);

/**
 * @brief Retrieve enabled status of all of the fifos
 *
 * @param[in] driver_interface - driver instance
 * @param[out] is_enabled[DEVICE_FIFO_MAX_ID] - array of bool status
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_is_enabled(sensor_fifo_driver_interface_t* driver_interface, bool (*is_enabled)[DEVICE_FIFO_MAX_ID]);

/**
 * @brief Retrieve empty status of all of the fifos
 *
 * @param[in] driver_interface - driver instance
 * @param[out]  is_empty[DEVICE_FIFO_MAX_ID] - array of bool status
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_inf_is_empty(sensor_fifo_driver_interface_t* driver_interface,
                                              bool (*is_empty)[DEVICE_FIFO_MAX_ID]);

/**
 * @brief Write 1 or more entries to a fifo
 *
 * @param[in] driver_interface - driver instance
 * @param[in] fifo_id - fifo identifier
 * @param[in] src_data - pointer to source data
 * @param[in] entry_size - size of a single entry
 * @param[in] num_entries - total number of entries in src_data
 * @param[in] stride_index - index into the current stride in fifos that have a stride size > entry size. Ignored otherwise.
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_write_entries(sensor_fifo_driver_interface_t* driver_interface,
                                               DEVICE_FIFO_ID fifo_id,
                                               uint8_t* src_data,
                                               size_t entry_size,
                                               uint16_t num_entries,
                                               uint16_t stride_index);

/**
 * @brief Read an entry from a fifo
 *
 * @param[in] driver_interface - driver instance
 * @param[in] fifo_id - fifo identifier
 * @param[in] entry_size - size of a single entry
 * @param[out] dest_data - destination location where entry will be copied
 * @param[out] num_entries_read - 0 or 1   0 when fifo is empty
 * @param[out] num_entries_remaining - remaining entries in fifo
 * @param[out] stride_index - entry index into the current stride in fifos that have a stride size > entry size. STRIDE_INDEX_UNUSED otherwise.
 * @retval fpfw_status_t
 */
fpfw_status_t sensor_fifo_driver_read_entry(sensor_fifo_driver_interface_t* driver_interface,
                                            DEVICE_FIFO_ID fifo_id,
                                            size_t entry_size,
                                            uint8_t* dest_data,
                                            uint16_t* num_entries_read,
                                            uint16_t* num_entries_remaining,
                                            uint16_t* stride_index);