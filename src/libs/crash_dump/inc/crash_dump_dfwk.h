//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_dfwk.h
 * Interface of crash dump driver framework.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief Crash dump Device structure.
 * 
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} crash_dump_device_t, *pcrash_dump_device_t;

/**
 * @brief Crash dump Request types.
 * 
 */
typedef enum {
    CRASH_DUMP_START_TRANSFER_ASYNC = 0xCD01,
} CRASH_DUMP_REQUEST_TYPE;

/**
 * @brief Crash dump Request structure.
 * 
 */
typedef struct
{
    DFWK_ASYNC_REQUEST_HEADER Header;

    uint32_t status; // Status of the request
} crash_dump_request_t, *pcrash_dump_request_t;

/**
 * @brief Crash dump Interface structure.
 * 
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    pcrash_dump_device_t Device;
} crash_dump_interface_t, *pcrash_dump_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the crash dump device.
 * 
 * @param device 
 * @param schedule 
 */
void crash_dump_device_initialize(pcrash_dump_device_t device, PDFWK_SCHEDULE schedule);

/**
 * @brief Initialize the crash dump interface.
 * 
 * @param intf 
 */
void crash_dump_interface_initialize(pcrash_dump_interface_t intf, pcrash_dump_device_t device);

/**
 * @brief Start the crash dump transfer asynchronously.
 * 
 * @param iface Pointer to the crash dump interface.
 * @param request Pointer to the crash dump request.
 * @param callback Completion routine to call when the operation is complete.
 * @param context Context to be passed to the completion routine.
 * 
 * @return uint32_t Status of the operation.
 */
uint32_t crash_dump_start_transfer_async(pcrash_dump_interface_t iface,
                                     pcrash_dump_request_t request,
                                     DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE callback,
                                     void* context);
