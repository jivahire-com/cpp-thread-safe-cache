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
#include <idsw_kng.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef void (*rx_msg_notification)(void);


typedef struct {
    TX_BLOCK_POOL rx_pool;  // block size must be > sizeof(trp_msg_t)
    TX_QUEUE rx_queue; // Queue of pointers to messages in the pool
    rx_msg_notification notify_from_drv_frmwk; // called from driver framework thread
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
    dcp_msg_ifwi_version_t ifwi_version;
} dcs_config_t, *p_dcs_config_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the Data Collection Service
 *
 * @note Not Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] config Pointer to the configuration to use. config must be statically allocated for lifetime
 *
 * @return None
 */
void dcs_init(p_dcs_config_t config);

/**
 * @brief Copy metadata for this core to staging area
 *
 * @note Not Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] atu_base_addr Base address of the ATU mapped area to copy metadata to
 * @param[in] max_size Maximum size of the area to copy metadata to
 *
 * @return None
 */
void dcs_create_manifest_from_elf(uintptr_t atu_base_addr, size_t max_size);

/**
 * @brief Build the diagnostic decoder schema
 *
 * @note Not Thread Safe
 * @note Not ISR Safe
 *
 * @return None
 */
void dcs_build_diag_decoder_full_manifest(void);

/**
 * @brief The primary instance is the MCP on Die 0, as that is the core that interfaces directly
 * with the Host. Client behavior may differ based on whether it is the primary instance.
 *
 * @note Thread Safe
 *
 *
 * @return true if this is the primary instance
 */
bool dcs_is_primary_instance(void);

/**
 * @brief Check if the die and core id match this device
 *
 * @note Thread Safe
 *
 * @param[in] die_id Die ID to check
 * @param[in] cpu_id Cpu ID to check
 *
 * @return true if the die and cpu id match this device
 */
bool dcs_is_this_device(uint8_t die_id, uint8_t cpu_id);

/** @brief Get the die id of this device
 *
 * @note Thread Safe
 *
 * @return Die ID of this device
 */
uint8_t dcs_get_this_die_id(void);

/** @brief Get the cpu id of this device
 *
 * @note Thread Safe
 *
 * @return Cpu ID of this device
 */
uint8_t dcs_get_this_cpu_id(void);

/**
 * @brief Registers a client to receive DCS commands
 *
 * @note Thread and ISR Safe
 * @note Client must ensure rx_pool block size is > sizeof(trp_msg_t)
 *
 * @param[in]  id Client ID to register
 * @param[in]  client Client to register. Client must ensure statically allocated for lifetime
 *
 * @return FPFW_STATUS_SUCCESS on success or an error code on failure
 */
fpfw_status_t dcs_client_register(dcp_client_id_t id, p_dcs_client_t client);


/**
 * @brief Send a new message.
 * @note The message will be copied and queued for the DCS thread to handle. The caller may free the message after this function returns.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in,out] trp_msg Message to send.
 *
 * @return none
 */
void dcs_client_send_new_trp_msg(p_trp_msg_t trp_msg);

/**
 * @brief Forward an existing message to the die and core specified in the trp header. Use for forwarding incoming messages.
 * @note The message will be copied and queued for the DCS thread to handle. The caller may free the message after this function returns.
 * @note The original sequence number is between the host and the primary. With this forwarding, a new sequence number is generated and
 * @note init_cmd is set to 1 for proper handling. 
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in,out] trp_msg Message to send. The header broadcast_type field is set to TRP_BROADCAST_NONE prior to returning
 * @param[in] broadcast_option Option to broadcast to all dies that match the destination core.
 *
 * @return none
 */
void dcs_client_forward_trp_msg(p_trp_msg_t trp_msg, trp_broadcast_t broadcast_option);

/**
 * @brief Send a response to a TRP message received from the remote core.
 *   Consumer only needs to update payload.  This function will swap the source and destination fields in the header.
 *
 * @note The message will be copied and queued for the DCS thread to handle. The caller may free the message after this function returns.
 * @note Thread Safe
 * @note Not ISR Safe
 *
 * @param[in] trp_msg Message to send
 *
 * @return none
 */
void dcs_client_send_trp_response(p_trp_msg_t trp_msg);

/**
 * @brief Send a DCP notification message to the host.
 *
 * @param[in] client_id - The client ID send from
 * @param[in] notification - The notification type
 */
void dcs_client_send_dcp_notification(dcp_client_id_t client_id, dcp_notification_type_t notification);

/**
 * @brief Loop through the clients queue, pop the block and free it
 * @note Helper function for all clients to use. If a client has a specific need, it should implement its own version.
 *
 */
void dcs_client_flush_incoming_queue(dcp_client_id_t id);

