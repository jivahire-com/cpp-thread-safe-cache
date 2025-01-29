//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_prim.h
 *  SCMI primitive support. 
 */

#pragma once

/*--------------- Includes ---------------*/

/*--------- Macro Definitions ------------*/

#define SCMI_PROTOCOL_CMD_SUCCESS 0
#define SCMI_PROTOCOL_CMD_UKNOWN  1
#define SCMI_PROTOCOL_NOT_SUPPORTED 2


/*--------------- Structures ---------------*/

/**
 * @brief API sets debug mode to spew payloads on ICC transactions
 *
 * @param mode value for debugging
 * @return None
 */
void scmi_set_debug_mode(uint8_t mode);

/**
 * @brief API sends a response
 *
 * @param protocol_id - scmi protocol used for responding, make sure it is supported by host
 * @param cmd_id this is the message id supported by the protocol 
 * @param payload - response payload
 * @param size - payload size
 * @return SUCCESS
 */
int scmi_send_resp(uint8_t protocol_id, uint8_t cmd_id, uint8_t* payload, size_t size);
