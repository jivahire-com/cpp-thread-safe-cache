//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_relay_i.h
 * Internal header for telemetry relay routing
 */

#pragma once

/*----------- Nested includes ------------*/
#include "telemetry_relay_protocol.h"


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the DCP router
 *
 * @param icc_config Configuration for the ICC endpoints
 */
void tlm_relay_init(p_trp_icc_config_t icc_config);

/**
 * @brief Send a DCP message to the Host.
 *
 * @note The message will use synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] dcp_msg Message to send
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t tlm_relay_send_dcp_msg(const p_dcp_msg_t dcp_msg);


/**
 * @brief Send a message to the die and core specified in the trp header.
 *
 * @note The message will use synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] trp_msg Message to send
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t tlm_relay_send_trp_msg(const p_trp_msg_t trp_msg);

/**
 * @brief Callback for when a message is received from the driver framework for icc
 *
 * @param context - p_trp_icc_endpoint_t
 * @param output_size_bytes not used
 * @param status Status from ICC base
 */
void tlm_relay_icc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status);
