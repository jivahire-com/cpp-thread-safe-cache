//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_driver_i.h
 * Private Header for the AVS module.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <scp_avs_driver.h>
#include <debug.h>
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <scp_avs.h>
#include <scp_avs_driver.h>
#include <stdint.h>

/*-------------- Typedefs ----------------*/

typedef struct _scp_avs_device_t {
    DFWK_DEVICE_HEADER Header;
    const scp_avs_config_t *config;
    DFWK_QUEUE avs_queue;
    pscp_avs_request outstanding_request;
    uint8_t avs_bus_num;
    pscp_avs_error_count avs_response_errors;
} scp_avs_device_t, *pscp_avs_device;

typedef struct _scp_avs_interface_t {
    DFWK_INTERFACE_HEADER Header;
    pscp_avs_device Device;
} scp_avs_interface_t, *pscp_avs_interface;

/*--------- Function Prototypes ----------*/

/**
 *
 *    Initializes the AVS device.  
 *
 *    @param[in]  Device
 *        The device object
 * 
 *    @brief Open the AVS device.  Initialize the AVS interrupts. 
 *           The AVS bus will be configured based on static 
 *           configuration information.  Called once for each AVS bus.
 *
 */
void scp_avs_driver_initialize(pscp_avs_device Device);

/**
 *
 *    Initializes the AVS module interface (synchronous and asynchronous).  
 *
 *    @param[in]  Device
 *    @param[in]  Interface
 * 
 *    @brief Called (num of AVS) X number of clients.
 *           If the client makes a synchronous request, then scp_avs_dispatch_sync is called.
 *           If the client makes an asynchronous request, then the request is placed on the Device queue.
 *
 */
void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface Interface);

/**
 *
 *    This will handle the AVS interrupt, and will copy the response buffer into the client buffer.
 *    @param[in]  context
 *        SCP AVS device information.
 *
 */
void scp_avs_isr(void* context);

/**
 *
 *    This will handle the AVS asynchronous calls.
 *    @param[in]  Request
 *    @param[in]  Context        
 *
*/
void scp_avs_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 *
 *    This will handle the AVS synchronous calls.
 *    @param[in]  Request   
 *
*/
int32_t scp_avs_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request);