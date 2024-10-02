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
#include <icc_mhu_cfg.h>
#include <idhw.h>
#include <silibs_platform.h>


/*-- Symbolic Constant Macros (defines) --*/

#define ICC_MHU_TRANS_SCMI_RESP_INDEX      0
#define ICC_MHU_TRANS_SCMI_RECEIVE_INDEX   1

#define ICC_MHU_TRANS_INVALID_INDEX        0xFFFF



#ifndef MAILBOX_MSCP_OFFSET
    #define MAILBOX_MSCP_OFFSET (0x60000000)
#endif


// ** TODO, Move this definitions in silibs
// https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2029674
#define MHU_SCP_MCP_SEND_BASE_ADDRESS (MHU_SCP_2_MCP_BASE_ADDRESS)
#define MHU_SCP_MCP_REC_BASE_ADDRESS  (MHU_SCP_2_MCP_BASE_ADDRESS + MHU_RECEIVE_OFFSET)
#define MHU_MCP_SCP_SEND_BASE_ADDRESS (MHU_MCP_2_SCP_BASE_ADDRESS)
#define MHU_MCP_SCP_REC_BASE_ADDRESS  (MHU_MCP_2_SCP_BASE_ADDRESS + MHU_RECEIVE_OFFSET)

// For the single Die approach, use the EXP RAM Base 1 for the moment. We need to commonize or agree
// whether to retain this within the EXP RAM or should we move this into the ARSM 1
#ifndef SCP_EXP_TOP_RAM1_BASE
#define SCP_EXP_TOP_RAM1_BASE (0x130000)
#endif

#ifndef SCP_2_MCP_SMT_SIZE
#define SCP_2_MCP_SMT_SIZE 128
#endif

#ifndef SCP_2_MCP_SEND_BASE_SMT
#define SCP_2_MCP_SEND_BASE_SMT SCP_EXP_TOP_RAM1_BASE
#endif

#ifndef SCP_2_MCP_RECEIVE_BASE_SMT
#define SCP_2_MCP_RECEIVE_BASE_SMT (SCP_2_MCP_SEND_BASE_SMT + SCP_2_MCP_SMT_SIZE)
#endif

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

typedef struct {
    uint32_t command;
    uint16_t size;
    uint8_t* payload;
} icc_mhu_transport_sync_message_t;

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


/*--------- Function Prototypes ----------*/

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
 * @brief API to initialize everything about the transport primitive
 *
 * @param p_config_table - pointer to the configuration table
 * @param table_size - number of configurations in the table
 * @return ICC_MHU_STATUS_SUCCESS when initialization was done
 */
int icc_mhu_trans_init(void *p_config_table, uint8_t table_size);

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
