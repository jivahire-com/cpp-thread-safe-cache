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
#include <fpfw_icc_dispatcher.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-------------- Typedefs ----------------*/

typedef struct {
    uint16_t send_core_2_core_id;
    uint16_t receive_core_2_core_id;
    uint16_t irq_num;
} icc_mhu_transport_config_t;

typedef struct {
    DFWK_QUEUE queue;
    DFWK_ASYNC_REQUEST_DISPATCH dispatch_routine;
    void* request_ref;
    void* mhu_device;
    uint8_t timer_retry_count;
    TX_TIMER timer;
} icc_mhu_transport_async_ctx_t;

typedef struct {
    DFWK_DEVICE_HEADER header;
    icc_mhu_transport_config_t config;
    size_t ref_count;
    DFWK_QUEUE default_queue; //! default dispatch queue that gets all requests initially
    icc_mhu_transport_async_ctx_t async_send_ctx;
    icc_mhu_transport_async_ctx_t async_recv_ctx;
} icc_mhu_transport_device_t;

/**
 * @brief icc mhu interface, initialized in `icc_mhu_trans_dfwk_interface_init`
 *
 */
typedef struct {
    DFWK_INTERFACE_HEADER header;
    icc_mhu_transport_device_t* device;
} icc_mhu_transport_intrf_t;

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
 * @param p_interface The icc mhu transport interface to initialize
 *
 * @return status is DFWK_SUCCESS or 0 if success, or one of the error codes for ICC transport of type fpfw_status_t
 */
int32_t icc_mhu_trans_dfwk_interface_init(icc_mhu_transport_device_t* icc_mhu_dev, icc_mhu_transport_intrf_t* p_interface);

/**
 * @brief Checks a received message from the icc mhu transport interface against an icc dispatcher entry
 * 
 * @param[in] recv_req The received request
 * @param[in] current_entry The current dispatcher entry
 * @param[in] ctx The context provided when setting up the dispatcher
 * 
 * @return true if the received message matches the dispatcher entry, false otherwise
 */
bool icc_mhu_trans_dwfk_icc_dispatcher_match_cb(PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req, fpfw_icc_dispatch_table_entry* current_entry, void* ctx);

#ifdef __cplusplus
}
#endif