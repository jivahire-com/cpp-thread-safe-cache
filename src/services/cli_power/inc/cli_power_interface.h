//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_interface.h
 * Header file for power cli interface with the power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <power_dfwk.h>
#include <power_runconfig.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef void (*print_function)(void* power_runconfig_data);

typedef struct
{
    ppower_service_interface_t p_interface;
    power_service_cli_request_t request;
    e_cli_power_command_id_t command_id;
} cli_power_cmd_context_t, *pcli_power_cmd_context_t;

typedef struct {
    const char* sub_command;
    print_function fn;
    power_if_cmd_t power_ext_if_cmd_id;
} power_cli_sub_command_dictionary_element_t, *ppower_cli_sub_command_dictionary_element_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
