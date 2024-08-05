//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atu_api.h
 * Header containing definitions for the atu module driver interface.
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
#define ATU_AP_ARSM_ADDRESS (0x60000000U)
#define ATU_AP_CORE_CLUSTER_ADDRESS (0x70000000U)

#define MODULE_NAME "[ATU_SVC] "
#define NEWLINE     "\n"

#define ATU_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define ATU_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define ATU_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
/**
 *  @brief ATU service interface type
 *
 *    ATU module will provide a read/write type through the interface to the clients.
 *
 */
typedef enum _atu_api_type
{
    ATU_IO_REQUEST_MAP_SYNC,
    ATU_IO_REQUEST_UNMAP_SYNC,
} atu_api_type_t;

// struct for the atu device object
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} atu_device_t;

// struct for an interface to the atu service
typedef struct  {
    atu_device_t* atu_device;
    DFWK_INTERFACE_HEADER header;
} atu_service_t;

// struct for an request to the atu service
typedef struct {
    DFWK_SYNC_REQUEST_HEADER header;
} atu_service_request_t;
