//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mesh_cli.h
 * Mesh CLI APIs
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkClient.h>
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MESH_CLI_ERROR_ISR_TYPE          0x0
#define MESH_CLI_FAULT_ISR_TYPE          0x1

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *
 *    Initializes the Mesh CLI interface  
 *
 *    @param[in]  Interface
 * 
 *    @brief Function call to initialize the Mesh CLI interface
 *
 */
void mesh_cli_initialize(void);

