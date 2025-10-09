//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file afm_cli.h
 * Header file for UART AFM cli
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize AFM CLI
 * @pre This function should be called after gpio is initialized.
 * 
 * This function initializes the AFM CLI functionality on MCP,
 * allowing it to configure AFM and Die config on both dies.
 * 
 * @param None
 * 
 * @return void
 */
void afm_cli_init(void);
