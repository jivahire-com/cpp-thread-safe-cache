//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_collection_service.h
 * Public header for the Data Collection Service
 */

#pragma once

/*----------- Nested includes ------------*/

#include "data_collection_protocol.h"
#include "telemetry_relay_protocol.h"
#include <fpfw_icc_base.h>
#include <fpfw_status.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef void (*rx_msg_notification)(void);


typedef struct {
    dcp_client_id_t id;
    TX_BLOCK_POOL rx_pool;  // block size must be > sizeof(trp_msg_t)
    TX_QUEUE rx_queue; // Queue of pointers to messages in the pool
    rx_msg_notification notify; // called from driver framework thread
} dcs_client_t, *p_dcs_client_t;

typedef struct
{
    void* p_stack;
    size_t stack_size;
    uint32_t priority;
    uint32_t time_slice_option;
} dcs_thread_config_t, *p_dcs_thread_config_t;

typedef struct {

    dcs_thread_config_t thread_config;
    trp_icc_config_t trp_icc_config;
} dcs_config_t, *p_dcs_config_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the Data Collection Service
 *
 * @note Not Thread Safe
 * @note Not ISR Safe
 *
 * @param config Pointer to the configuration to use
 *
 * @return None
 */
void dcs_init(p_dcs_config_t config);

/**
 * @brief Registers a client to receive DCS commands
 *
 * @note Thread and ISR Safe
 *
 * @param[in]  client Client to register. Memory copied to internal storage.
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_register_client(dcs_client_t client);

/**
 * @brief Send a message to the AP. Can be a request or a response.
 *
 * @note The message will use the synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] dcp_msg Message to send
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_client_send_dcp_msg(const p_dcp_msg_t dcp_msg);


/**
 * @brief Send a message to the die and core specified in the trp header. Use for initial request.
 * @note The message will use the synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] trp_msg Message to send
 * @param[in] broadcast_option Option to broadcast to all dies that match the destination core
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_client_send_trp_msg(const p_trp_msg_t trp_msg, trp_broadcast_t broadcast_option);

/**
 * @brief Send a response to a TRP message received from the remote core.
 *   Consumer only needs to update payload.  This function will swap the source and destination fields in the header.
 *
 * @note The message will use the synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] trp_msg Message to send
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_client_send_trp_response(const p_trp_msg_t trp_msg);