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
#define MODULE_NAME "[SDS_SVC] "
#define NEWLINE     "\n"

#define SDS_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SDS_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SDS_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
/**
 *  @brief SDS service interface type
 *
 *    SDS module will provide a read/write type through the interface to the clients.
 *
 */
typedef enum _sds_api_type
{
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
    struct 
    {
        void* buffer;
        size_t byte_count;
    } input;
    struct
    {
        size_t processed_bytes;
    } output;
} sds_service_request_t, *psds_service_request_t;

/*--------- Function Prototypes ----------*/
/**
 * Read data from within a Shared Data Structure, asynchronously. 
 *
 *  @param pInterface An interface to a driver that implements the SDS interface.
 *
 *  @param buffer A buffer to read data into.
 * 
 *  @param buffer_size The size of the buffer.
 * 
 *  @param processed_bytes The number of bytes read.
 */
int32_t sds_read(PDFWK_INTERFACE_HEADER pInterface, void *buffer, size_t buffer_size, size_t *processed_bytes);

/**
 *  Write data within a Shared Data Structure, asynchronously.
 *
 *  @param pInterface An interface to a driver that implements the SDS interface.
 *
 *  @param buffer A buffer to read data into.
 * 
 *  @param buffer_size The size of the buffer.
 * 
 *  @param processed_bytes The number of bytes written.
 */
int32_t sds_write(PDFWK_INTERFACE_HEADER pInterface, void *buffer, size_t buffer_size, size_t *processed_bytes);