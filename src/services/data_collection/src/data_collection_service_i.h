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
#define NEW_OUTBOUND_MSG        (1 << 1)

// Max number of inflight messages that are intended for the DCS client
// messages for other clients are queued in the clients queue
#define DCS_MAX_DCS_ClIENT_MESSAGES (DCP_MSG_ID_MAX)

// Max number of messages to be queued for forwarding trp messages
#define DCS_MAX_TRP_FORWARDING_MESSAGES (DCP_MSG_ID_MAX * 2)

#define ROUND_UP_TO_4_BYTE_ALIGN(x) (((x) + 3) & ~3)
#define DCS_TRP_MSG_BLOCK_SIZE ROUND_UP_TO_4_BYTE_ALIGN(sizeof(trp_msg_t))
// threadx allocated 4 bytes per block to manage pool
#define DCS_BLOCK_POOL_SIZE ((sizeof(uint32_t) + DCS_TRP_MSG_BLOCK_SIZE) * (DCS_MAX_TRP_FORWARDING_MESSAGES))

/*-------------- Typedefs ----------------*/

typedef struct {

    // Array of clients, if a client is not registered, the entry will be zeroed out
    p_dcs_client_t clients[DCP_CLIENT_ID_MAX];

    /**
     * RTOS Resources for thread processing
     */
    TX_THREAD thread;
    // The msg queue stores pointers to messages copied into the pool
    // instead of the messages themselves due to queue entry size limitations.
    struct {
        TX_QUEUE queue;
        trp_msg_t* mem[DCS_MAX_TRP_FORWARDING_MESSAGES];
    } trp_outbound_queue;
    struct {
        TX_BLOCK_POOL pool;
        uint8_t mem[DCS_BLOCK_POOL_SIZE];
    } msg_pool;

    TX_EVENT_FLAGS_GROUP thread_ctrl;

} dcs_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Notify DCS thread that a client message was received.
 *
 * @note Thread Safe
 *
 *
 * @return None
 */
void dcs_service_client_notification_from_drv_frmwk(void);

/**
 * @brief   Forward TRP messages to the appropriate client.
 *
 * @param[in] trp_msg - Pointer to the incoming TRP message
 *
 */
void dcs_forward_trp_msg_to_client_from_drv_frmwk(p_trp_msg_t trp_msg);

/**
 * @brief   Forward TRP messages to all non DCP service clients. This is used when messages are sent to the DCS service
 *         client and need to be routed to all of the active clients.
 *
 * @param[in] trp_msg - Pointer to the incoming TRP message
 *
 */
void dcs_forward_trp_msg_to_all_non_dcp_svc_clients(p_trp_msg_t trp_msg);

/**
 * @brief Allocate a block from the clients pool, queue the block and notify the client
 *
 * @param[in]  client_block_pool The block pool to allocate from
 * @param[in]  client_rx_queue The queue to send the message to
 * @param[in] incoming_trp_msg The TRP message to copy
 *
 * @return FPFW_SUCCESS if the message was queued
 */
fpfw_status_t dcs_queue_msg_from_drv_frmwk(TX_BLOCK_POOL* client_block_pool, TX_QUEUE* client_rx_queue, p_trp_msg_t incoming_trp_msg);

/**
 * @brief Validate the DCP message
 *
 * @param[in,out] dcp_msg The DCP message to validate. The header status field will be updated appropriately
 *
 * @return true if the message is valid
 */
bool dcs_is_valid_dcp_msg_from_drv_frmwk(p_dcp_msg_t dcp_msg);

/**
 * @brief Validate the TRP message
 *
 * @param[in,out] dcp_msg The TRP message to validate. The header status field will be updated appropriately
 *
 * @return true if the message is valid
 */
bool dcs_is_valid_trp_msg_from_drv_frmwk(p_trp_msg_t trp_msg);

/**
 * @brief Queue a TRP message for outbound forwarding. Can either be an error response or a TRP hop
 *
 * @param[in] trp_msg The TRP message to forward
 * @param[in] swap_source_dest Swap the source and destination fields
 *
 * @return none
 */
void dcs_queue_for_outbound_from_drv_frmwk(p_trp_msg_t trp_msg, bool swap_source_dest);

/**
 * @brief This api is called from the DCS thread and will block to send the message.
 *
 * @param[in] trp_msg The TRP message to send
 * @param[in] swap_source_dest Swap the source and destination fields of the trp header
 *
 * @return none
 */
void dcs_send_outgoing_msg(p_trp_msg_t trp_msg, bool swap_source_dest);

/**
 * @brief Empty the outbound queue and send the messages via ICC
 *
 * @return none
 */
void dcs_handle_outbound_msgs(void);

/**
 * @brief Flush active messages for reset command
 *
 * @return none
 */
void dcs_flush_outgoing_queue(void);

/**
 * @brief UnRegister all DCS clients
 *  Primarily a test api.
 *
 * @return none
 */
void dcs_unregister_clients(void);