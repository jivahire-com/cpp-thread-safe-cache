//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_cli.h
 * Header file for variable services cli commands
 */

#pragma once

/*----------- Nested includes ------------*/
#include <variable_services.h>    // for var_service_shared_mem_t, var_serv...

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
/**
 * @brief 
 * @param mem_ctx 
 */
void variable_services_cli_init(var_service_shared_mem_t *mem_ctx);