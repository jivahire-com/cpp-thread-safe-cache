//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_mbox_icc_transport.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER
#include <MboxPrimitives.h>  // for PFPFW_MBX_PRIMITIVE_CTX
#include <fpfw_status.h>
#include <fpfw_timer.h> // for fpfw_timer_t

/*-------------- Typedefs ----------------*/

/**
 * @brief List of async requests
 */
typedef enum{
    ICC_MBX_ASYNC_SEND,
    ICC_MBX_ASYNC_RECV,
    ICC_MAX_ASYNC_REQ_TYPE,
} fpfw_mbox_async_req_types_t;

/**
 * @brief struct for ref to the request & queue owning the request
 * Only applicable for async
 */
typedef struct
{
    DFWK_QUEUE queue; //! queue to store mem ref for send from client
    DFWK_ASYNC_REQUEST_DISPATCH dispatch_routine;
    void* request_ref; //! ptr to the current dispatched request, all requests serialized
    void* dispatch_ctx;
} fpfw_mbox_icc_transport_req_t;

/**
 * @brief struct for timer associated items. Only applicable for async.
 * One timer for each serialized queue containing requests.
 */
typedef struct 
{
    fpfw_timer_t* handle; //! timer obj used to enable polling associated with the device
    fpfw_dur_t period; //! polling interval
    fpfw_timer_variant_t type; 
    fpfw_timer_callback cb;
    void* cb_ctx;
} fpfw_mbox_icc_transport_timer_t;

/**
 * @brief combined struct for a req & its associated timer.
 * Only applicable for async. 
 * 2 instances of req ctx for mailbox. One for each send &
 * recv since they monitor different physical hardwares. 
 */
typedef struct 
{
    fpfw_mbox_icc_transport_req_t request; //! request context containing req ref & owning queue
    fpfw_mbox_icc_transport_timer_t timer; //! timer associated with the request.
}fpfw_mbox_icc_transport_async_req_ctx_t;

/**
 * @brief Mailbox device ctx, initialized in `fpfw_mbox_icc_transport_dfwk_device_init`

 */
typedef struct {
    DFWK_DEVICE_HEADER header;
    uint32_t mbx_irq_num; //! irq number for the mailbox
    size_t ref_count; 
    DFWK_QUEUE default_queue; //! default dispatch queue that gets all requests initially
    FPFW_MBX_PRIMITIVE_CTX mbox_prim_ctx; //! ref to initialized mbox prim ctx
    FPFW_MBX_REG_CONFIG mbox_dev_cfg; //! mbox driver config
    fpfw_mbox_icc_transport_async_req_ctx_t async[ICC_MAX_ASYNC_REQ_TYPE]; //! Only applicable for async
} fpfw_mbox_icc_transport_device_t;


/*--------- Function Prototypes ----------*/

/**
 * @brief Bottom half of SW interrupt cb for Mailbox.
 */
void accel_mbox_sw_intr_cb(void* mbox_dev_ctx);
