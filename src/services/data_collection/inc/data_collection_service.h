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
#include <fpfw_status.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct {
    /**
     * @brief Client handler for DCP messages 
     * 
     * @param[in]  msg     A pointer to a DCP Message, from the clients memory pool
     * @param[in]  context A pointer to client provided context
     */
    void (*handle_msg)(const dcp_msg_t * msg, void * context);
    void *context;
} dcp_msg_handler_t;

typedef struct {
    uint32_t id;
    uint64_t pool_size;
    TX_BLOCK_POOL pool;
    dcp_msg_handler_t MSG_handler;
} dcs_client_t;

typedef struct {
    struct
    {
        void* p_stack;
        size_t stack_size;
        uint32_t priority;
        uint32_t time_slice_option;
    } thread_config;
} dcs_config_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Registers a client to receive DCS commands
 *
 * @note Thread and ISR Safe
 * 
 * @param[in]  client Client to register. Memory copied to internal storage.
 * 
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_register_client(dcs_client_t * client);

/**
 * @brief Send a message to the AP. Can be a request or a response.
 * 
 * @note The message will use the synchronous icc functionality, and will block until the message is sent.
 * @note Thread Safe
 * @note Not ISR Safe
 * 
 * @param[in] msg Message to send
 * 
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_send_msg(const dcp_msg_t * msg);
