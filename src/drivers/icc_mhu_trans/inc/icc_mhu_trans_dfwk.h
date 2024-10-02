//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_mhu_trans_dfwk.h
 * This is a icc mhu driver supporting both single die and multi die icc
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>                    // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <stdint.h>
#include <stdio.h>

/*-------------- Typedefs ----------------*/

/**
 * @brief Mailbox config for device init
 * Provides the memory for the timer handles
 */
typedef struct {
    uint16_t send_core_2_core_id;
    uint16_t receive_core_2_core_id;
} icc_mhu_transport_config_t;

/**
 * @brief icc mhu device initialized in `icc_mhu_trans_dfwk_init`

 */
typedef struct {
    DFWK_DEVICE_HEADER header;
    icc_mhu_transport_config_t config;
    size_t ref_count;
    DFWK_QUEUE default_queue; //! default dispatch queue that gets all requests initially
} icc_mhu_transport_device_t;

/**
 * @brief icc mhu interface, initialized in `icc_mhu_trans_dfwk_interface_init`
 *
 */
typedef struct {
    DFWK_INTERFACE_HEADER header;
    icc_mhu_transport_device_t* device;
} icc_mhu_transport_intrf_t;

typedef struct {
    DFWK_SYNC_REQUEST_HEADER Header;
    int32_t status;
    uint32_t command;
    uint16_t size;
    uint8_t*  payload;
} icc_mhu_request_t, icc_mhu_send_request_t, icc_mhu_read_request_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Routine for initializing the icc mhu device.
 *
 * @note: Not reentrant
 *
 * @param icc_mhu_dev The mailbox device instance to initialize
 * @param config - The required config to initialize the icc_mhu driver.
 *
 * @return int32_t status is DFWK_SUCCESS or 0 if success, or one of the error codes for ICC transport of type fpfw_status_t
 */
int32_t icc_mhu_transport_dfwk_device_init(icc_mhu_transport_device_t* icc_mhu_dev, DFWK_SCHEDULE* schedule, icc_mhu_transport_config_t* config);

/**
 * @brief Routine for initializing the icc mhu transport interface.
 *
 * @note: Not reentrant
 * @note: DfwkInterfaceOpen needs to be done independently from this
 *
 * @param icc_mhu_dev The icc mhu device instance to associate the interface against
 * @param interface The icc mhu transport interface to initialize
 *
 * @return status is DFWK_SUCCESS or 0 if success, or one of the error codes for ICC transport of type fpfw_status_t
 */
int32_t icc_mhu_trans_dfwk_interface_init(icc_mhu_transport_device_t* icc_mhu_dev, icc_mhu_transport_intrf_t* interface);


/**
 * @brief Routine for sending a synchronous command through MHU Transport, dfwk
 *        (note that the transport interface library API can also be used)
 *
 * @note: Not reentrant, blocking
 *
 * @param interface The icc mhu transport interface used for sending to another core
 * @param command - Command to send
 * @param payload
 * @param payload_size - size of "payload" in bytes
 *
 * @return status is DFWK_SUCCESS or 0 if success, or one of the error codes for ICC transport of type fpfw_status_t
 */
fpfw_status_t icc_mhu_trans_dfwk_send_sync_message(DFWK_INTERFACE_HEADER* interface, uint32_t command, uint8_t* payload, size_t payload_size);


/**
 * @brief Routine for checking to receive an entire packet message from MHU Transport, dfwk
 *        (note that the transport interface library API can also be used)
 *
 * @note: Not reentrant, blocking
 *
 * @param interface The icc mhu transport interface channel to check whether a message is received from it
 * @param payload   buffer to store the payload received
 * @param payload_size size of payload
 *
 * @return status is ICC_MHU_TRANS_NO_RECEIVE_MESSAGE if not receive message
 *                   ICC_MHU_TRANS_INTF_MSG_AVAILABLE if a message is received
 */

fpfw_status_t icc_mhu_trans_dfwk_recv_sync_message(DFWK_INTERFACE_HEADER* interface, uint8_t* payload, size_t payload_size);

/**
 * @brief Routine for checking to receive a command from a from MHU Transport channel, dfwk
 *
 * @note: Not reentrant
 *
 * @param interface The icc mhu transport interface channel to check whether a message is received from it
 * @param command - command we are interested to check whether it was received.
 * @param payload   buffer to store the payload received
 * @param payload_size size of payload
 *
 * @return status is ICC_MHU_TRANS_CMD_MESSAGE_AVAILABLE when a command message is available
 */
fpfw_status_t icc_mhu_trans_dfwk_chk_recv_cmd_sync(DFWK_INTERFACE_HEADER* interface, uint32_t command, uint8_t* payload, size_t payload_size);


#ifdef __cplusplus
}
#endif