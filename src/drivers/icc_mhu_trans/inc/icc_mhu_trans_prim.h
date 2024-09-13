//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file init_mhu_trans_prim.h
 *  ICC MHU primitive support. 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <icc_mhu_cfg.h>
#include <idhw.h>
#include <silibs_platform.h>

/*--------- Macro Definitions ------------*/
#define ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE    0
#define ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE 1

#define ICC_MHU_TRANS_SCMI_RESP_INDEX      0
#define ICC_MHU_TRANS_SCMI_RECEIVE_INDEX   1

/*--------------- Structures ---------------*/

/*!
 * \brief Structure to associate an icc command with a handler.
 */
typedef struct {
    uint32_t command;
    void* handler;
} icc_handler_t;

/*!
 * \brief Definitions for ICC MHU Message Header
 */
typedef struct {
    uint32_t command;
    uint16_t payload_size;
    uint16_t status;
} icc_msg_header_t;

/**
 * @brief Mailbox config for device init
 * Provides the memory for the timer handles
 */
typedef struct {
    uint32_t command;
	uint16_t size;
    uint8_t  data[];	
} icc_mhu_transport_message_t;

/*!
 * \brief Definitions for ICC MHU transport header
 */
typedef struct {
    uint16_t token;
    uint16_t mhu_interface_id;    
    icc_msg_header_t msg_header;
} icc_mhu_trans_header_t;

/*!
 * \brief Definitions for ICC MHU Packet
 */
typedef struct {
    icc_mhu_trans_header_t header;
    uint32_t payload[];
} icc_mhu_trans_packet_t;


/**
 * @brief API to send command and data to a specified MHU interface channel id.
 *
 * @param mhu_interface_id interface channel id that specifies the source id of the current core and destination core
 * @param command to send
 * @param data to send
 * @param size of data to send
 * @return ICC_MHU_STATUS_SUCCESS - when command with parameters are successfully sent
 */
int icc_mhu_trans_send_message(uint16_t mhu_interface_id, uint32_t command, uint8_t* data, size_t size);

/**
 * @brief API to send command and data to a specified know configuration index.
 *
 * @param index from config table that specifies the source id of the current core and destination core
 * @param command to send
 * @param data to send
 * @param size of data to send
 * @return ICC_MHU_STATUS_SUCCESS - when command with parameters are successfully sent
 */
int icc_mhu_trans_send_message_idx(uint16_t index, uint32_t command, uint8_t* data, size_t size);

/**
 * @brief API to check if there is a message on the channel.
 *
 * @param mhu_interface_id interface channel id that specifies the destination id of the current core and source core
 * @return ICC_MHU_TRANS_STATUS
 */
int icc_mhu_trans_check_message_received(uint16_t mhu_interface_id);

/**
 * @brief API to initialize everything about the transport primitive
 *
 * @param configuration - pointer to the configuration table
 * @param num_configs - number of configurations in the table
 * @return ICC_MHU_STATUS_SUCCESS when initialization was done
 */
int icc_mhu_trans_init(icc_mhu_configuration_t* configuration, uint8_t num_configs);

/**
 * @brief API to support icc SCMI CLI - may need to deprecate later
 *
 * @param index interface index that specifies the source id of the current core and destination core
 * @return None
 */
void icc_mhu_trans_check_scmi_status_bit(uint16_t index);

/**
 * @brief API to obtain the message from a configuration index
 *
 * @param index - interface index that specifies the source id of the current core and destination core
 * @param client_msg - buffer passed by the caller to store the message in the format described by icc_mhu_transport_message_t
 * @return None
 */
int icc_mhu_trans_get_msg_from_index(uint8_t index, icc_mhu_transport_message_t *client_msg);
