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
#include "data_collection_service.h"
#include "telemetry_relay_protocol.h"

#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
// event flags
#define NEW_DCS_CLIENT_MSG      (1 << 0)
#define NEW_OUTGOING_MSG        (1 << 1)


// Max number of inflight messages that are intended for the DCS client
// messages for other clients are queued in the clients queue
#define DCS_MAX_DCS_ClIENT_MESSAGES (DCP_MSG_ID_MAX)

// Max number of messages to be queued for sending  error responses
#define DCS_MAX_DCP_ERROR_MESSAGES (DCP_MSG_ID_MAX)

// Max number of messages to be queued for fowarding trp messages
#define DCS_MAX_TRP_FORWARDING_MESSAGES (DCP_MSG_ID_MAX)

#define ROUND_UP_TO_4_BYTE_ALIGN(x) (((x) + 3) & ~3)
#define DCS_MSG_BLOCK_SIZE ROUND_UP_TO_4_BYTE_ALIGN(sizeof(trp_msg_t))
// threadx allocated 4 bytes per block to manage pool
#define DCS_BLOCK_POOL_SIZE ((sizeof(uint32_t) + DCS_MSG_BLOCK_SIZE) * (DCS_MAX_DCP_ERROR_MESSAGES + DCS_MAX_TRP_FORWARDING_MESSAGES))

/*-------------- Typedefs ----------------*/

typedef struct {

    // Array of clients, if a client is not registered, the entry will be zeroed out
    dcs_client_t clients[DCP_CLIENT_ID_MAX];

    /**
     * RTOS Resources for thread processing
     */
    TX_THREAD thread;
    // The msg queue stores pointers to messages copied into the pool
    // instead of the messages themselves due to queue entry size limitations.
    struct {
        TX_QUEUE queue;
        trp_msg_t* mem[DCS_MAX_DCP_ERROR_MESSAGES];
    } dcp_error_queue;
    struct {
        TX_QUEUE queue;
        trp_msg_t* mem[DCS_MAX_TRP_FORWARDING_MESSAGES];
    } trp_forward_queue;
    struct {
        TX_BLOCK_POOL pool;
        uint8_t mem[DCS_BLOCK_POOL_SIZE];
    } msg_pool;

    TX_EVENT_FLAGS_GROUP thread_ctrl;

} dcs_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/


/**
 * @brief   Forward TRP messages to the appropriate client.
 *
 * @param trp_hdr - Generated TRP header
 * @param trp_msg - Pointer to the incoming TRP message
 */
void dcs_forward_trp_msg_to_client_from_drv_frmwk(p_trp_msg_t trp_msg);