//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_collection_service_i.h
 * Private header for the Data Collection Service
 */

#pragma once

/*----------- Nested includes ------------*/

#include "data_collection_protocol.h"
#include <data_collection_service.h>
#include <fpfw_status.h>
#include <icc_mhu_trans_prim.h>
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define DCS_MAX_DCP_MESSAGES (DCP_MSG_ID_MAX * 3)

#define ETR_WORKER_THREAD_NAME ("etr-worker_thread")
#define ETR_WORK_POOL_NAME ("etr-work_pool")
#define ETR_WORK_QUEUE_NAME ("etr-work_queue")

/*-------------- Typedefs ----------------*/

typedef struct {

    // Array of clients, if a client is not registered, the entry will be zeroed out
    dcs_client_t clients[DCP_CLIENT_ID_MAX];

    // ICC Requests for receiving DCP messages. Capable of receiving multiple
    // of each message type.
    icc_mhu_transport_message_t icc_receive_msgs[DCS_MAX_DCP_MESSAGES];

    /**
     * RTOS Resources for thread processing
     */
    TX_THREAD thread;
    // The msg queue stores pointers to messages copied into the pool
    // instead of the messages themselves due to queue entry size limitations.
    struct {
        TX_QUEUE queue;
        dcp_msg_t* mem[DCS_MAX_DCP_MESSAGES];
    } msg_queue;
    struct {
        TX_BLOCK_POOL pool;
        dcp_msg_t mem[DCS_MAX_DCP_MESSAGES];
    } msg_pool;
    // Control client list access
    TX_MUTEX client_mutex;

} dcs_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the Data Collection Service
 * 
 * @note Not Thread Safe
 * @note Not ISR Safe
 * 
 * @param context Pointer to the context to initialize
 * @param config Pointer to the configuration to use
 * 
 * @return FPFW_SUCCESS on success, error code otherwise
 */
fpfw_status_t dcs_init(dcs_context_t* context, dcs_config_t* config);

/**
 * @brief Processes a DCP message, dispatch to clients as needed.
 * 
 * @param context DCS context
 * @param msg DCP Message to process
 */
void dcs_process_msg(dcs_context_t* context, dcp_msg_t* msg);
