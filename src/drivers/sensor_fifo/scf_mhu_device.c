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

#include "sensor_fifo_driver_interface.h" // for psensor_fifo_device_prop...

#include <DfwkHost.h>                       // for DfwkDeviceInitialize
#include <FpFwAssert.h>                     // for FPFW_RUNTIME_ASSERT
#include <fpfw_status.h>                    // for FPFW_STATUS_SUCCESS, fpf...
#include <sensor_fifo_driver_interface_i.h> // for (anonymous struct)::(ano...
#include <silibs_platform.h>                // for MMIO_READ32, MMIO_WRITE32

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pscf_mhu_device_config_t sp_scf_mhu_device_cfg;

/*------------- Functions ----------------*/

void scf_mhu_device_initialize(scf_mhu_device_t* scf_mhu_device,
                               DFWK_SCHEDULE* schedule,
                               psensor_fifo_device_properties_t property_table,
                               size_t property_table_array_size,
                               pscf_mhu_device_config_t scf_mhu_device_cfg)
{
    FPFW_RUNTIME_ASSERT(property_table_array_size == SCF_MHU_FIFO_MAX_ID);

    DfwkDeviceInitialize(&(scf_mhu_device->sensor_fifo_device.base_device), schedule);
    scf_mhu_device->sensor_fifo_device.initialized = true;
    scf_mhu_device->sensor_fifo_device.dispatch_sync = scf_mhu_request_dispatch_sync;
    scf_mhu_device->sensor_fifo_device.fifo_property_table = property_table;

    sp_scf_mhu_device_cfg = scf_mhu_device_cfg;
}

fpfw_status_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request)
{
    fpfw_status_t status = FPFW_STATUS_FAIL;
    switch (request->RequestType)
    {
    case SENSOR_FIFO_SYNC_REQUEST_READ_REG: {
        psensor_fifo_drv_inf_read_reg_request read_req = (psensor_fifo_drv_inf_read_reg_request)request;
        read_req->output.value = MMIO_READ32((volatile uint32_t*)read_req->input.address);
        status = FPFW_STATUS_SUCCESS;
        break;
    }
    case SENSOR_FIFO_SYNC_REQUEST_WRITE_REG: {
        psensor_fifo_drv_inf_write_reg_request write_req = (psensor_fifo_drv_inf_write_reg_request)request;
        MMIO_WRITE32((volatile uint32_t*)write_req->input.address, write_req->input.value);
        status = FPFW_STATUS_SUCCESS;
        break;
    }
    default:
        // No other types of requests are supported
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    return status;
}
