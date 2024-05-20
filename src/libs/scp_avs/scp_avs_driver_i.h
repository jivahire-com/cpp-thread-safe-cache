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
#include <stdint.h>
/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *
 *    Initializes the AVS device.
 *
 *    @param[in]  Device
 *        The device object
 * 
 *    @brief Open the AVS device. Initialize the AVS interrupts. 
 *           The AVS bus will be configured based on static 
 *           configuration information. Called once for each AVS bus.
 *
 */
void scp_avs_driver_initialize(pscp_avs_device Device);

/**
 *
 *    This will handle the AVS interrupt, and will copy the response buffer into the client buffer.
 * 
 *    @param[in]  context
 *        SCP AVS device information.
 *
 */
void scp_avs_isr(void* context);

/**
 *
 *    This will handle the AVS asynchronous calls.
 * 
 *    @param[in]  Request
 *    @param[in]  Context
 *
*/
void scp_avs_dispatch(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);

/**
 *
 *    This will handle the AVS synchronous calls.
 * 
 *    @param[in]  Request
 *
*/
int32_t scp_avs_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER Request);
