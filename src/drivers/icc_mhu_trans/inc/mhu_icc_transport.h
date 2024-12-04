//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mhu_icc_transport.h
 * 
 * @brief This is a mhu icc transport driver that implements the ICC transport interface.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkDriver.h>
#include <fpfw_icc_transport_interface.h>
#include <fpfw_icc_dispatcher.h>
#include <fpfw_status.h>
#include <fpfw_timer.h>
#include <fpfw_timer_port.h>
#include <fpfw_timer_types.h>
#include <icc_mhu.h>
#include <stdint.h>
#include <stdbool.h>

/*-------------- Macros ------------------*/

#define member_size(type, member) (sizeof( ((type *)0)->member ))

#define ICC_MHU_CMD_BIT_OFFSET ((offsetof(icc_mhu_header_t, msg_header) + offsetof(msg_header_t, command)) * CHAR_BIT)
#define ICC_MHU_CMD_SIZE_BITS (member_size(msg_header_t, command) * CHAR_BIT)

#define ICC_MHU_TOKEN_BIT_OFFSET (offsetof(icc_mhu_header_t, token) * CHAR_BIT) 
#define ICC_MHU_TOKEN_SIZE_BITS (member_size(icc_mhu_header_t, token) * CHAR_BIT)

/*-------------- Typedefs ----------------*/

//
// MHU ICC Transport Device
//

typedef struct {
    uint32_t recv_irq_num;

    icc_mhu_channel_t recv_channel;
    icc_mhu_channel_t send_channel;

    fpfw_dur_t async_send_retry_period;
    uint8_t async_send_retry_max;
} mhu_icc_transport_device_config_t, *pmhu_icc_transport_device_config_t;

typedef struct {
    // Serialized async send queue
    DFWK_QUEUE queue;
    // The active async send request
    PDFWK_ASYNC_REQUEST_HEADER req;
    // Timer control for send retries
    fpfw_timer_t timer;
    bool timer_active;
    uint8_t timer_retry_max;
    uint8_t timer_retry_count;
} mhu_icc_transport_async_send_t, *pmhu_icc_transport_async_send_t;

typedef struct {
    // Serialized async recv queue
    DFWK_QUEUE queue;
    // The active async recv request
    PDFWK_ASYNC_REQUEST_HEADER req;
    // Recv IRQ
    uint32_t recv_irq_num;
} mhu_icc_transport_async_recv_t, *pmhu_icc_transport_async_recv_t;

typedef struct {
    DFWK_DEVICE_HEADER base_device;

    // Captures both the MHU IP configuration and the shared memory configuration
    icc_mhu_channel_t recv_channel;
    icc_mhu_channel_t send_channel;

    // Queue for all immediate dispatched async requests
    DFWK_QUEUE async_req_queue;

    // Async request contexts
    mhu_icc_transport_async_recv_t async_recv_ctx;
    mhu_icc_transport_async_send_t async_send_ctx;

    bool initialized;
} mhu_icc_transport_device_t, *pmhu_icc_transport_device_t;

//
// MHU ICC Transport Interface
//

typedef struct {
    DFWK_INTERFACE_HEADER base_interface;
    mhu_icc_transport_device_t* device;
} mhu_icc_transport_intrf_t, *pmhu_icc_transport_intrf_t;

/*--------- Function Prototypes ----------*/

/**
 * @brief Initializes the MHU ICC Transport Device.
 *
 * @note: Not ISR Safe
 * @note: Not Reentrant Safe
 *
 * @param[in] dev    The device to initialize
 * @param[in] config The device configuration
 *
 * @return FPFW_ICC_TRANSPORT_STATUS_*
 */
fpfw_status_t mhu_icc_transport_device_init(mhu_icc_transport_device_t* dev, DFWK_SCHEDULE* schedule, mhu_icc_transport_device_config_t* config);

/**
 * @brief Initializes the MHU ICC Transport Interface.
 *
 * @note: Not ISR Safe
 * @note: Not Reentrant Safe
 *
 * @param[in] dev         The icc mhu device instance to associate the interface against
 * @param[in] p_interface The icc mhu transport interface to initialize
 *
 * @return FPFW_ICC_TRANSPORT_STATUS_*
 */
fpfw_status_t mhu_icc_transport_interface_init(mhu_icc_transport_device_t* dev, mhu_icc_transport_intrf_t* p_interface);

/**
 * @brief Checks a received request from the icc mhu transport interface against an icc dispatcher entry
 * 
 * @param[in] recv_req The received request
 * @param[in] current_entry The current dispatcher entry
 * @param[in] ctx The context provided when setting up the dispatcher
 * 
 * @return true if the received request matches the dispatcher entry, false otherwise
 */
bool mhu_icc_transport_dispatcher_match_cb(PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req, fpfw_icc_dispatch_table_entry* current_entry, void* ctx);
