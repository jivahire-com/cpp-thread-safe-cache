//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scf_mhu_device.h
 * SCF MHU is a Kingsgate platform implementation of a sensor fifo device.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "sensor_fifo_driver_interface.h"  // for psensor_fifo_device_proper...
#include <DfwkSchedule.h>                  // for DFWK_SCHEDULE
#include <stdbool.h>                       // for bool
#include <stddef.h>                        // for size_t
#include <stdint.h>                        // for uintptr_t, uint32_t


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct __attribute__((aligned(4))){
    sensor_fifo_device_t sensor_fifo_device;

} scf_mhu_device_t, *pscf_mhu_device_t;

typedef struct {
    uintptr_t  scf_mhu_base_address;
    uintptr_t  scf_exp_csr_base_address;
    uintptr_t  scf_ram_base_address;
    uint32_t   scf_ram_buffer_size;
    uint64_t   tile_mask;
    uint64_t   core_mask_lo;
    uint64_t   core_mask_hi;
    bool is_scp;
} scf_mhu_device_config_t, *pscf_mhu_device_config_t;

/*-- Declarations (Statics and globals) --*/
/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize an SCF MHU device
 *
 * @param[in] scf_mhu_device - Pointer to an scf_mhu_device_t instance
 * @param[in] schedule - pointer to the driver framework scheduler
 * @param[in] property_table - array of psensor_fifo_device_properties_t
 * @param[in] property_table_array_size - size of the property table
 * @param[in] scf_mhu_device_cfg - pointer to configuration values
 */
void scf_mhu_device_initialize(pscf_mhu_device_t scf_mhu_device, DFWK_SCHEDULE* schedule, psensor_fifo_device_properties_t property_table, size_t property_table_array_size, pscf_mhu_device_config_t scf_mhu_device_cfg);