//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scf_mhu_device.h
 * SCF MHU is a Kingsgate platform implementation of a sensor fifo.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "DfwkSchedule.h"                  // for DFWK_SCHEDULE
#include "sensor_fifo_driver_interface.h"  // for psensor_fifo_device_proper...
#include <stdbool.h>                       // for bool
#include <stddef.h>                        // for size_t
#include <stdint.h>                        // for uintptr_t, uint32_t


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 * @brief Fifo ID specific to device to remain uncoupled from higher Firmware layers
 *
 */
typedef enum
{
    SCF_MHU_FIFO_PSTATE_TELEMETRY_HW = 0,
    SCF_MHU_FIFO_SCP_MSG_TELEMETRY_HW = 1,
    SCF_MHU_FIFO_TILE_TEMPERATURE_TELEMETRY_HW = 2,
    SCF_MHU_FIFO_TILE_VOLTAGE_TELEMETRY_HW = 3,
    SCF_MHU_FIFO_CORE_CURRENT_TELEMETRY_HW = 4,
    SCF_MHU_FIFO_PVT_TEMP_FW = 5,
    SCF_MHU_FIFO_PVT_VOLTAGE_FW = 6,
    SCF_MHU_FIFO_DIMM_TEMP_FW = 7,
    SCF_MHU_FIFO_VR_TEMP_FW = 8,
    SCF_MHU_FIFO_VR_CURRENT_FW = 9,
    SCF_MHU_FIFO_MAX_ID = 10
} SCF_MHU_FIFO_ID;


typedef struct __attribute__((aligned(4))){
    sensor_fifo_device_t sensor_fifo_device;


} scf_mhu_device_t;

typedef struct {
    uintptr_t  scf_mhu_base_address;
    uintptr_t  scf_csr_base_address;
    uintptr_t  scf_ram_base_address;
    uint32_t   scf_ram_buffer_size;
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
void scf_mhu_device_initialize(scf_mhu_device_t* scf_mhu_device, DFWK_SCHEDULE* schedule, psensor_fifo_device_properties_t property_table, size_t property_table_array_size, pscf_mhu_device_config_t scf_mhu_device_cfg);