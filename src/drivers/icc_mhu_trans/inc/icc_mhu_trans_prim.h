//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file init_mhu_trans_prim.h
 *  ICC MHU primitive support. This header is intended to separate
 *  the primitives API from any framework approach for future portability 
 *  on another framework. This primitives library will mostly be used
 *  for the framework that inherits it, and test purposes.
 *  
 */

#pragma once

/*----------- Nested includes ------------*/
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <idhw.h>
#include <limits.h>
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

#define member_size(type, member) (sizeof( ((type *)0)->member ))

#define ICC_MHU_TRANS_SCMI_RESP_INDEX      0
#define ICC_MHU_TRANS_SCMI_RECEIVE_INDEX   1

#define ICC_MHU_TRANS_INVALID_INDEX        0xFFFF

#define ICC_MHU_CMD_BIT_OFFSET ((offsetof(icc_mhu_header_t, msg_header) + offsetof(msg_header_t, command)) * CHAR_BIT)
#define ICC_MHU_CMD_SIZE_BITS (member_size(msg_header_t, command) * CHAR_BIT)

#define ICC_MHU_TOKEN_BIT_OFFSET (offsetof(icc_mhu_header_t, token) * CHAR_BIT) 
#define ICC_MHU_TOKEN_SIZE_BITS (member_size(icc_mhu_header_t, token) * CHAR_BIT)

/*-------------- Typedefs ----------------*/

/*!
 * \brief list of status for icc mhu transport
 */

enum {
    ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE = 0,
    ICC_MHU_TRANS_INTF_MSG_AVAILABLE,
    ICC_MHU_TRANS_NO_CMD_MESSAGE_AVAILABLE,
    ICC_MHU_TRANS_NO_RECEIVE_MESSAGE
};

typedef struct {
    icc_mhu_header_t header;
    uint8_t payload[];
} icc_mhu_request_t;

/*--------- Function Prototypes ----------*/

/**
 * @brief API to initialize everything about the transport primitive
 *
 * @param p_config_table - pointer to the configuration table
 * @param table_size - number of configurations in the table
 * @return ICC_MHU_STATUS_SUCCESS when initialization was done
 */
int icc_mhu_trans_init(void *p_config_table, uint8_t table_size);

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
 * @brief Read a message from the specified channel and store it in the client_msg buffer
 *        if the command matches the client_msg command.
 *
 * @param index - Id representing the channel to operate on
 * @param client_msg - buffer passed by the caller to store the message. Must contain the command to validate
 *                     the message received against.
 * @return ICC_MHU_STATUS_SUCCESS olr failure
 */
int icc_mhu_trans_get_cmd_msg_from_index(uint16_t core_2_core_id, icc_mhu_request_t *client_msg);

/**
 * @brief Read a message from the specified channel and store it in the client_msg buffer
 *        regardless of what the command is.
 *
 * @param index - Id representing the channel to operate on
 * @param client_msg - buffer passed by the caller to store the message
 * @return ICC_MHU_STATUS_SUCCESS olr failure
 */
int icc_mhu_trans_get_msg_from_index(uint16_t core_2_core_id, icc_mhu_request_t *client_msg);

/**
 * @brief API to support icc SCMI CLI - may need to deprecate later
 *
 * @param index MHU configuration index
 */
void icc_mhu_trans_check_scmi_status_bit(uint16_t index);

