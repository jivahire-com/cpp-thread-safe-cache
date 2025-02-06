//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_service_init.h
 * Sensor Fifo Service Interface
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_icc_base.h>
#include <sensor_fifo_driver_interface.h>


/*-------------- Typedefs ----------------*/
 typedef struct {
    fpfw_icc_base_ctx_t*  mscp_icc_base_ctx;
    bool is_scp;
 } sensor_fifo_svc_config_t, *psensor_fifo_svc_config_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the service.
 *
 * @note Thread Safe - Yes
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 *
 * @param[in] driver_interface - Pointer to the driver interface
 * @param[in] svc_config - Pointer to the configuration items for the service
 */
void sensor_fifo_svc_initialize(psensor_fifo_driver_interface_t driver_interface, psensor_fifo_svc_config_t svc_config);
