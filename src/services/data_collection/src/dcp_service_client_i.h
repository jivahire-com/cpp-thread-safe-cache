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
 * @brief Initialize the DCP service client
 *
 * @param[in] ifwi_version The ifwi version in use. version info must be statically allocated for lifetime
 */
void dcp_svc_client_init(p_dcp_msg_ifwi_version_t ifwi_version);

/**
 * @brief Handle an incoming TRP message
 *
 */
void dcp_svc_client_handle_incoming_msgs(void);

/**
 * @brief Handle a DCP message
 *
 * @param[in] trp_msg The TRP message containing the DCP message
 */
void dcp_svc_client_handle_dcp_msg(p_trp_msg_t trp_msg);

/**
 * @brief Flush the incoming dcp svc client queue
 *
 */
void dcp_svc_client_flush_incoming_queue(void);