//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scf_mhu_device.c
 * SCF MHU is a Kingsgate platform implementation of a sensor fifo.
 * It implements the abstract sensor_fifo_driver_interface_t for this hardware.
 */

/*------------- Includes -----------------*/
#include "scf_mhu_device.h"

#include "sensor_fifo_driver_interface.h"

#include <DfwkClient.h>
#include <DfwkThreadXHost.h> // IWYU pragma: keep
#include <FpFwAssert.h>      // for FPFW_RUNTIME_ASSERT
#include <scf_mhu_regs.h>
#include <sensor_ram_bridge.h> // IWYU pragma: keep
#include <stdbool.h>           // for false, true

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
static int32_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request);

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static ptr_scf_mhu_reg sp_scf_mhu_regs;

/*------------- Functions ----------------*/

void scf_mhu_device_initialize(scf_mhu_device_t* scf_mhu_device, DFWK_SCHEDULE* schedule, uintptr_t scf_mhu_base_addr)
{
    DfwkDeviceInitialize(&(scf_mhu_device->sensor_fifo_device.base_device), schedule);
    scf_mhu_device->sensor_fifo_device.initialized = true;
    scf_mhu_device->sensor_fifo_device.dispatch_sync = scf_mhu_request_dispatch_sync;

    sp_scf_mhu_regs = (ptr_scf_mhu_reg)scf_mhu_base_addr;
}

static int32_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request)
{
    // sensor_fifo_driver_interface_t* pSensorFifoInterface = (sensor_fifo_driver_interface_t*)request->OwningInterface;

    switch (request->RequestType)
    {
    case SENSOR_FIFO_REQUEST_ID_READ_SYNC: {

        break;
    }
    case SENSOR_FIFO_REQUEST_ID_WRITE_SYNC: {

        break;
    }
    default:
        // No other types of requests are supported
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    return DFWK_SUCCESS;
}
