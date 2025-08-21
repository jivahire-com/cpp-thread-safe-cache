//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file et_mts_client.h
 * Header definitions for MCP and SCP Firmware Event Trace MTS Client.
 *
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/  
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initializes the Event Trace MTS client. 
 *
 * @param[in] void
 *
 * @retval void
 */
void event_trace_mts_client_init(void);

/**
 * @brief Callback function to notify about a new MTS message for Event Trace.
 *
 * This function is invoked when a new MTS message is received, allowing the client
 * to handle or process the message as needed.
 *
 * @param[in] void
 *
 * @retval void
 */
void event_trace_mts_client_notify_new_msg_cb(void);