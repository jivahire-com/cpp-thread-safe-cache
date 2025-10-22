//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel_i.h
 * Header containing definitions for the internal SEL APIs.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <icc_mhu.h>
#include <sel.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_MHU_PAYLOAD_SIZE (512)

/*-------------- Typedefs ----------------*/
/**
 * @brief SEL ICC MHU payload structure
 * 
 */
typedef struct
{
    icc_mhu_header_t header;
    sel_event_record_t record;
} icc_mhu_sel_payload_t;

/**
 * @brief SEL ICC MHU context structure for each ICC channel
 * 
 */
typedef struct
{
    fpfw_icc_base_ctx_t* icc_ctx;
    fpfw_icc_base_recv_req_t transfer_recv_req;
    union
    {
        icc_mhu_sel_payload_t payload;
        uint8_t byte_payload[MAX_MHU_PAYLOAD_SIZE];
    };
} icc_mhu_sel_ctx_t;

/**
 * @brief SEL service device structure
 * 
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} sel_svc_device_t, *psel_svc_device_t;

/**
 * @brief SEL service request type
 * 
 */
typedef enum {
    SEL_TRANSFER_ASYNC = 0x5E01,
} SEL_REQUEST_TYPE;

/**
 * @brief SEL service request structure
 * 
 */
typedef struct
{
    DFWK_ASYNC_REQUEST_HEADER Header;

    uint32_t status;    // Status of the request
    bool is_completed;  // Indicates if the request is completed or pendding
} sel_svc_request_t, *psel_svc_request_t;

/**
 * @brief SEL service interface structure
 * 
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    psel_svc_device_t Device;
} sel_svc_interface_t, *psel_svc_interface_t;


/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize SEL queue
 */
void sel_init_queue();

/**
 * @brief Push SEL event record into internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if overflow.
 */
bool sel_push(sel_event_record_t* event_record);

/**
 * @brief Push SEL event record to the head of internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if overflow.
 */
bool sel_push_head(sel_event_record_t* event_record);

/**
 * @brief Pop SEL event record from internal queue
 *
 * @param event_record SEL event record.
 * @return True if succeeded. False if queue is empty.
 */
bool sel_pop(sel_event_record_t* event_record);

/**
 * @brief Register MHU ICC messsage receive callback
 * 
 * @param ctx ICC MHU SEL context
 * @return fpfw_status_t FPFW_ICC_BASE_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t icc_register_mhu_sel_transfer_callback(icc_mhu_sel_ctx_t *ctx);

/**
 * @brief Return SEL ICC context based on type
 * 
 * @param type SEL ICC type
 * @return SEL ICC context
 */
fpfw_icc_base_ctx_t *sel_icc_ctx(sel_icc_type_t type);

/**
 * @brief Send single SEL event record asynchronously
 * 
 * @param event_record SEL event record
 * @param request SEL async request header
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_send_event_async(sel_event_record_t* event_record, PDFWK_ASYNC_REQUEST_HEADER request);

/**
 * @brief Flush SEL event queue
 *
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS sel_flush_queue();

/**
 * @brief Check if PLDM is ready
 *
 * @return True if PLDM is ready, false otherwise
 */
bool sel_is_PLDM_ready();