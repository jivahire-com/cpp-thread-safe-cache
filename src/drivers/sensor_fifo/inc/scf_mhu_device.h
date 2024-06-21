//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scf_mhu_device.h
 * SCF MHU is a Kingsgate platform implementation of a sensor fifo.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "sensor_fifo_driver_interface.h"



/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct __attribute__((aligned(4))){
    sensor_fifo_device_t sensor_fifo_device;

} scf_mhu_device_t;


/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize an scf_mhu_device
 *
 * @param[in] scf_mhu_device - Pointer to an scf_mhu_device_t instance
 * @param[in] schedule - pointer to the driver framework scheduler
 * @param[in] scf_mhu_base_addr - base address for the CSR registers.
 */
void scf_mhu_device_initialize(scf_mhu_device_t* scf_mhu_device, DFWK_SCHEDULE* schedule, uintptr_t scf_mhu_base_addr);