//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_api.h
 * Header containing definitions for the SDS module driver interface.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <DfwkTypes.h>
#include <DfwkCommon.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 *  @brief SDS service interface type
 *
 *    SDS module will provide a read/write type through the interface to the clients.
 *
 */
typedef struct {
    void* base;
    uint32_t size;
} sds_region_desc_t;

typedef struct {
    const sds_region_desc_t *regions;
    uint32_t region_count;
} sds_config_t;

typedef enum _sds_api_type
{
    SDS_IO_REQUEST_CREATION_SYNC,
    SDS_IO_REQUEST_READ_SYNC,
    SDS_IO_REQUEST_WRITE_SYNC,
} sds_api_type_t;

// struct for the sds device object
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} sds_service_t, *psds_service_t;

// struct for an interface to the sds service
typedef struct  {
    DFWK_INTERFACE_HEADER header;
    psds_service_t p_device;
} sds_service_interface_t, *psds_service_interface_t;

// struct for an request to the sds service
typedef struct {
    DFWK_SYNC_REQUEST_HEADER header;
    uint32_t sds_module_id;
    uint32_t sds_module_size;
    uint32_t sds_module_region_id;
    struct 
    {
        void* buffer;
        uint32_t byte_count;
    } io;
} sds_service_request_t, *psds_service_request_t;

/*--------- Function Prototypes ----------*/
/**
 *  query the SDS configuration information
 * 
 *  @return sds_config_t sds configuration data.
 */
sds_config_t* retrieve_sds_config_info();

/**
 *  Initialize the SDS block if doesn't exist. if exists, return success.
 *
 * 
 *  @param sds_module_id The id of the module to create the block for
 * 
 *  @param request_size The size of memory that module is requesting
 * 
 *  @return int32_t The status of the operation.
 */
int32_t sds_block_creation(uint32_t sds_module_id, uint32_t request_size, uint32_t region_id);

/**
 * Read data from within a Shared Data Structure. 
 *
 *  @param sds_module_id The id of the module to create the block for
 *
 *  @param buffer A buffer to read data into.
 * 
 *  @param buffer_size The size of the buffer.
 * 
 *  @return The status of the operation.
 */
int32_t sds_block_read(uint32_t sds_module_id, void* buffer, size_t buffer_size);

/**
 *  Write data within a Shared Data Structure.
 *
 *
 *  @param buffer A buffer to write data into sds.
 *
 *  @param buffer_size The size of the buffer.
 *
 *  @return The status of the operation.
 */
int32_t sds_block_write(uint32_t sds_module_id, void* buffer, size_t buffer_size);