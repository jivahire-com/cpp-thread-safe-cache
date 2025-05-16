//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_service_dfwk.h
 * Header containing definitions for the dfwk portion of Accel Interrupt service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <accelip_id.h>
#include <stdint.h>

/*-------------- Typedefs ----------------*/

/**
 *  @brief Synchonous and Asynchronous Interface Request IDs
 */
typedef enum {
    ACCEL_INTR_SERVICE_COMMANDS_START_ID = 0x1000,
    ACCEL_INTR_SERVICE_FATAL_INTR_RECVD,
    ACCEL_INTR_SERVICE_MAX_COMMAND
} e_accel_intr_service_command_id_t;

// struct for the Accel Interrupt service/device
typedef struct
{
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} accel_intr_service_t, *paccel_intr_service_t;

// struct for an interface to the Accel Interupt service
typedef struct
{
    DFWK_INTERFACE_HEADER header;
    paccel_intr_service_t p_device;
} accel_intr_service_interface_t, *paccel_intr_service_interface_t;

/* Structure for the async dfwk request to the accel interrupt interface */
typedef struct {
    DFWK_ASYNC_REQUEST_HEADER header;
    ACCEL_ID accel_type;
} accel_intr_service_request_t, *paccel_intr_service_request_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif