//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_service_i.h
 * Private header for sensor service internals
 */

#pragma once

/*----------- Nested includes ------------*/

#include <icc_mhu.h>
#include <mscp_uefi_shared_ddrss.h>
#include <sensor_fifo_driver_interface.h>

#include "sensor_fifo_service_init.h"

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum {
    SNSR_FIFO_MSG_ID_SYNC_ENABLES   = 0x1,
    DCP_MSG_ID_MAX                  = 0xD,
} snsr_fifo_msg_id_t;

typedef struct __attribute__((packed)) {
    uint16_t msg_id;        // snsr_fifo_msg_id_t
    uint16_t seq_num;
    int16_t  msg_status;
    uint16_t payload_size;
} snsr_fifo_msg_hdr_t;

typedef struct __attribute__((packed)) {
    bool fifo_enabled[DEVICE_FIFO_MAX_ID];
} snsr_fifo_msg_sync_enables_t;

typedef struct __attribute__((packed)) {
    snsr_fifo_msg_hdr_t hdr;
    union __attribute__((packed)) {
        snsr_fifo_msg_sync_enables_t sync_enables;       // SNSR_FIFO_MSG_ID_SYNC_ENABLES
    } payload;
} snsr_fifo_msg_t;

typedef struct
{
    icc_mhu_header_t header;
    snsr_fifo_msg_t sf_msg;

} icc_mhu_snsr_fifo_msg_t;

/*-- Declarations (Statics and globals) --*/

// provides access for unit testing
extern psensor_fifo_driver_interface_t pSensor_fifo_driver_inf;
extern psensor_fifo_svc_config_t snsr_fifo_svc_config;
extern fpfw_icc_base_recv_req_t mcp_icc_async_recv_req;
extern uint8_t mcp_icc_rx_buffer[ICC_MHU_DDR_PAYLOAD_SIZE];
extern FPFW_LOCK snsr_fifo_seq_lock;
extern uint16_t snsr_fifo_seq_num;

/*--------- Function Prototypes ----------*/

/**
 * @brief Fifo poll helper function
 *
 * @param[in] fifo_id - fifo id to read from
 * @param[in] entry_size - size of the entry to read
 * @param[in] dest_data - pointer to the destination buffer
 * @param[out] stride_index - index of the stride if applicable
 */
sensor_ram_poll_status_t fifo_poll_helper(DEVICE_FIFO_ID fifo_id, size_t entry_size, uint8_t* dest_data, uint16_t* stride_index);

/**
 * @brief Notify MCP that fifo enables have changed
 *
 * @param[in] is_enabled - array of bool values for each fifo
 */
fpfw_status_t sensor_fifo_svc_notify_mcp(bool (*is_enabled)[SENSOR_FIFO_MAX_ID]);

/**
 * @brief   Notify the sensor service that an icc message has been received
 *
 * @param[in] context - pointer to the fpfw_icc_base_recv_req_t context
 * @param[in] output_size_bytes - not used
 * @param[in] status - status of the icc message
 */
void sensor_fifo_svc_recv_complete_notify_from_drv_frmwk(void* context, size_t output_size_bytes, fpfw_status_t status);