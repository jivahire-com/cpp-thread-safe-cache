//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_interface_.h
 * Private definitons for sensor_fifo_driver_interface.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkClient.h>
#include <DfwkCommon.h>
#include <fpfw_status.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum SENSOR_FIFO_REQUEST_ID
{
    SENSOR_FIFO_SYNC_REQUEST_READ_REG,
    SENSOR_FIFO_SYNC_REQUEST_WRITE_REG,
} SENSOR_FIFO_REQUEST_ID;

/**
 * @brief Sync request structure for reading a register
 * request used with SENSOR_FIFO_SYNC_REQUEST_READ_REG
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        uint32_t address; //! register address
    } input;
    struct
    {
        uint32_t value; //!  current value of register
    } output;
} sensor_fifo_drv_inf_read_reg_request, *psensor_fifo_drv_inf_read_reg_request;

/**
 * @brief Sync request structure for writing a register
 * request used with SENSOR_FIFO_SYNC_REQUEST_WRITE_REG
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        uint32_t address; //! register address
        uint32_t value; //!  new value for register
    } input;
} sensor_fifo_drv_inf_write_reg_request, *psensor_fifo_drv_inf_write_reg_request;



/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

fpfw_status_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request);