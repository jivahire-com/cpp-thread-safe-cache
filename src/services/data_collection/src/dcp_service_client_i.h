//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcp_service_client_i.h
 * Internal api's for DCP service client handlers
 */

#pragma once

/*----------- Nested includes ------------*/
#include "telemetry_relay_protocol.h"

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Handle a TRP message
 *
 * @param msg Pointer to the TRP message
 */
void dcp_svc_client_handle_trp_msg(p_trp_msg_t msg);