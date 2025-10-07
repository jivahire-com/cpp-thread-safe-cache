//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_driver_interface_.h
 * Private definitions for sensor_fifo_driver_interface.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include "device_fifo_id.h"
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
    SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE,
    SENSOR_FIFO_SYNC_SET_FIFO_ENABLE,
    SENSOR_FIFO_SYNC_SYNCHRONIZE_FIFO_ENABLES,
    SENSOR_FIFO_SYNC_UPDATE_WRITE_PTR,
    SENSOR_FIFO_SYNC_QUERY_IS_ENABLED,
    SENSOR_FIFO_SYNC_QUERY_IS_EMPTY,
    SENSOR_FIFO_SYNC_WRITE_ENTRY,
    SENSOR_FIFO_SYNC_READ_ENTRY,
    SENSOR_FIFO_SYNC_RESET,
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

/**
 * @brief Structure for enabling or disabling global hw fifos. Does not affect firmware fifos.
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        bool enable;
    } input;
} sensor_fifo_drv_inf_global_enable, *psensor_fifo_drv_inf_global_enable;

/**
 * @brief Structure for updating the stride of a fifo
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        DEVICE_FIFO_ID fifo_id;
    } input;
} sensor_fifo_drv_inf_update_write_stride, *psensor_fifo_drv_inf_update_write_stride;

/**
 * @brief Structure for enabling or disabling a single fifo
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        DEVICE_FIFO_ID fifo_id;
        bool enable;
    } input;
    struct
    {
        bool (*is_enabled)[DEVICE_FIFO_MAX_ID];
    } output;
} sensor_fifo_drv_inf_fifo_enable, *psensor_fifo_drv_inf_fifo_enable;

/**
 * @brief Structure synchronizing all fifo enables
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        bool (*is_enabled)[DEVICE_FIFO_MAX_ID];
    } input;
} sensor_fifo_drv_inf_sync_fifo_enables, *psensor_fifo_drv_inf_sync_fifo_enables;


/**
 * @brief Structure for querying if fifo is enabled
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        bool (*is_enabled)[DEVICE_FIFO_MAX_ID];
    } output;
} sensor_fifo_drv_inf_fifo_is_enabled, *psensor_fifo_drv_inf_fifo_is_enabled;

/**
 * @brief Structure for querying if fifo is empty
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        bool (*is_empty)[DEVICE_FIFO_MAX_ID];
    } output;
} sensor_fifo_drv_inf_fifo_is_empty, *psensor_fifo_drv_inf_fifo_is_empty;

/**
 * @brief Structure for writing a fifo entry
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        DEVICE_FIFO_ID fifo_id;
        uint8_t* src_data;
        size_t entry_size;
        uint16_t num_entries;
        uint16_t stride_index; ///< pertinent when fifo stride > entry
    } input;
} sensor_fifo_drv_inf_write_entry, *psensor_fifo_drv_inf_write_entry;

/**
 * @brief Structure for reading a fifo entry
 *
 */
typedef struct
{
    DFWK_SYNC_REQUEST_HEADER header;
    struct
    {
        DEVICE_FIFO_ID fifo_id;
        size_t entry_size;
    } input;
    struct
    {
        uint8_t* dest_data;
        uint16_t* num_entries_read;
        uint16_t* num_entries_remaining;
        uint16_t* stride_index;
    } output;
} sensor_fifo_drv_inf_read_entry, *psensor_fifo_drv_inf_read_entry;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Dispatch a synchronous request to the SCF MHU device from DFWK
 *
 * @param[in] request Input request to process
 * @return FPFW_STATUS_SUCCESS or an error code
 */
fpfw_status_t scf_mhu_request_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER request);

/**
 * @brief Dispatch an asynchronous request to the SCF MHU Device from DFWK
 * 
 * @param[in] Request Input request to process
 * @param[in] Context Context for the asynchronous request
 * @return None
 */
void scf_mhu_request_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context);
