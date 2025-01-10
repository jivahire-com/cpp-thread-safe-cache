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
#include <icc_mhu.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
// ICC Base api's require the transport header
typedef struct
{
    icc_mhu_header_t header;
    union
    {
        dcp_msg_t dcp_msg;
        trp_msg_t trp_msg;
    };
} icc_msg_t, *p_icc_msg_t;

/*-- Declarations (Statics and globals) --*/
extern p_trp_icc_config_t trp_icc_config;

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the DCP router
 *
 * @param icc_config Configuration for the ICC endpoints
 */
void tlm_relay_init(p_trp_icc_config_t icc_config);

/**
 * @brief The primary instance is the MCP on Die 0, as that is the core that interfaces directly
 * with the Host. Behavior may differ based on whether it is the primary instance.
 *
 *
 * @return true if this is the primary instance
 */
bool tlm_relay_is_primary_instance(void);

/** @brief Check if the device is the one specified by the die and cpu id
 *
 * @note Thread Safe
 *
 * @param[in] die_id Die ID to check
 * @param[in] cpu_id Cpu ID to check
 *
 * @return true if this is the device specified by the die and cpu id
 */
bool tlm_relay_is_this_device(uint8_t die_id, uint8_t cpu_id);

/** @brief Get the die id of this device
 *
 * @note Thread Safe
 *
 * @return Die ID of this device
 */
uint8_t tlm_relay_get_this_die_id(void);

/** @brief Get the cpu id of this device
 *
 * @note Thread Safe
 *
 * @return Cpu ID of this device
 */
uint8_t tlm_relay_get_this_cpu_id(void);

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
 * @param[in] context - p_trp_icc_endpoint_t
 * @param[in] output_size_bytes not used
 * @param[in] status Status from ICC base
 */
void tlm_relay_icc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status);

/*
 * @brief  Look up destination in routing table and send out message
 * @param[in] trp_msg
 */
void tlm_relay_send_outgoing_msg(p_trp_msg_t trp_msg);

/**
 * @brief Check if the message should be broadcast to this route
 *
 * @param[in] trp_msg The TRP message to check
 * @param[in] trp_route The route to check
 *
 * @return true if the message should be broadcast to this route
 */
bool tlm_relay_should_broadcast_to_this_route(p_trp_msg_t trp_msg, p_trp_route_t trp_route);

/**
 * @brief Send a TRP message to the specified ICC endpoint
 *
 * @param[in] trp_msg The TRP message to send
 * @param[in] icc_endpoint The ICC endpoint to send the message to
 */
void tlm_relay_send_trp_via_icc(p_trp_msg_t trp_msg, p_trp_icc_endpoint_t icc_endpoint);
