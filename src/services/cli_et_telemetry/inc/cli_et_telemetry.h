//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_et_telemetry.h
 * Header file for EVT Telemetry CLI
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initialize EVT Telemetry CLI
 * 
 * This function initializes the EVT Telemetry CLI which is used to check the status of the EVT Telemetry 
 * DDR buffers.
 * 
 * @param None
 * 
 * @return void
 */
void evt_telemetry_cli_init(void);
