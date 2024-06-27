//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_dfwk.h
 * Header containing definitions for the dfwk portion of power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <power_runconfig.h>
#include <stdint.h>

/*-------------- Typedefs ----------------*/

/**
 *  @brief Synchonous and Asynchronous Interface Request IDs
 */
typedef enum {
    CLI_COMMANDS_START_ID = 0x1000,
    CLI_COMMANDS_POWER_CONFIG,
    CLI_COMMANDS_POWER_SET,
    CLI_COMMANDS_POWER_STATUS,
    CLI_COMMANDS_POWER_LOG,
} e_cli_power_command_id_t;

// struct for the power service/device
typedef struct
{
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} power_service_t, *ppower_service_t;

// struct for an interface to the power service
typedef struct
{
    DFWK_INTERFACE_HEADER header;
    ppower_service_t p_device;
} power_service_interface_t, *ppower_service_interface_t;

/* Structure for the async dfwk CLI request to the power interface */
typedef struct {
    DFWK_ASYNC_REQUEST_HEADER header;
    char* sub_command;
    void* p_requested_data;
    power_runconfig_element_t power_runconfig_element_id;
} power_service_cli_request_t, *ppower_service_cli_request_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif