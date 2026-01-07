//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_et_telemetry.h
 * Header file for EVT Telemetry CLI
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace_relay.h> // for etr_service_context_t

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
 * @param p_context Pointer to the ETR service context
 * 
 * @return void
 */
void evt_telemetry_cli_init(p_etr_service_context_t p_context);
