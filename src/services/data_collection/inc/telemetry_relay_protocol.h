//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_relay_protocol.h
 * Defines the protocol used by telemetry DCP clients to exchange info between dies and/ or cores.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "data_collection_protocol.h"

#include <fpfw_icc_base.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ENDPOINT_NAME_MAX_LENGTH (20)

#define ROUND_UP_TO_4_BYTE_ALIGN(x) (((x) + 3) & ~3)
#define DCS_TRP_MSG_BLOCK_SIZE ROUND_UP_TO_4_BYTE_ALIGN(sizeof(trp_msg_t))

/*-------------- Typedefs ----------------*/
#define TRP_GEN_MSG_ID(x) (0xD000 | (x))

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/


typedef enum {
    TRP_BROADCAST_NONE                         = 0,
    // for broadcast to work correctly, the primary MCP instance
    // must have routes defined for all secondary MCP instances
    // The secondary MCP does not broadcast further
    TRP_BROADCAST_PRIM_MCP_TO_SEC_MCP_ONLY     = 1,

    // broadcast to secondary MCP and then to SCP on the same die
    // if broadcast from the primary MCP, the message is also sent to the primary SCP
    TRP_BROADCAST_TO_SEC_MCP_THEN_TO_SCP       = 2,
} trp_broadcast_t;

typedef enum {
    TRP_STATUS_RD_DATA_VALID_MORE   = 3,
    TRP_STATUS_RD_DATA_VALID_LAST   = 2,
    TRP_STATUS_RD_DATA_NONE         = 1,
    TRP_STATUS_SUCCESS              = 0,
    TRP_STATUS_E_SIZE               = -1,
    TRP_STATUS_E_PARAM              = -2,
    TRP_STATUS_E_UNK_MSG            = -3,
    TRP_STATUS_E_UNK_CLIENT         = -4,
    TRP_STATUS_E_INCOMPLETE_HANDLER = -5,
    TRP_STATUS_E_DCP_ERROR          = -6,
} trp_status_t;

typedef enum {
    TRP_MSG_ID_DCP_FORWARD                      = TRP_GEN_MSG_ID(0x0),
    TRP_MSG_ID_PACKAGE_NOTIFICATION             = TRP_GEN_MSG_ID(0x1),
    TRP_MSG_ID_READ_PACKAGE                     = TRP_GEN_MSG_ID(0x2), // command only
    TRP_MSG_ID_READ_PACKAGE_RESPONSE            = TRP_GEN_MSG_ID(0x3),
    TRP_MSG_ID_READ_PACKAGE_COMPLETE            = TRP_GEN_MSG_ID(0x4),
    TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION     = TRP_GEN_MSG_ID(0x5),
    TRP_MSG_ID_READ_INTERCORE_BLOCK             = TRP_GEN_MSG_ID(0x6),
    TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE    = TRP_GEN_MSG_ID(0x7),
    TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE    = TRP_GEN_MSG_ID(0x8),

    TRP_MSG_ID_MAX                              = (0x9),
} trp_msg_id_t;

typedef struct {
    fpfw_icc_base_ctx_t*   icc_base_ctx;
    fpfw_icc_base_recv_req_t async_recv_req; // allocation for control to receive messages, no need to initialize
    void* async_recv_buffer;        // buffer for actual icc message
    size_t async_recv_buffer_size;  // size of  async_recv_buffer, must be large enough for max message size
    char name[ENDPOINT_NAME_MAX_LENGTH]; // name of the endpoint
    uint32_t icc_payload_protocol;
} trp_icc_endpoint_t, *p_trp_icc_endpoint_t;

typedef struct __attribute__((packed)) {
    uint8_t source_die_id;              // KNG_DIE_ID
    uint8_t source_cpu_id;              // KNG_CPU_TYPE
    uint8_t dest_die_id;                // KNG_DIE_ID
    uint8_t dest_cpu_id;                // KNG_CPU_TYPE
    uint16_t dcp_client_id;             // dcp_client_id_t
    uint16_t trp_msg_id;                // trp_msg_id_t
    int16_t trp_msg_status;             // trp_status_t
    uint16_t source_seq_num;
    p_trp_icc_endpoint_t incoming_endpt;// needed for broadcast
    uint16_t broadcast_type;            // trp_broadcast_t
    uint16_t payload_size;
} trp_msg_hdr_t, *p_trp_msg_hdr_t;

typedef struct __attribute__((packed)) {
    uint16_t block_id; // dcp client specific
} trp_block_read_req_t;

typedef struct __attribute__((packed)) {
    uint32_t rd_data_ddr_addr_offset; // offset from beginning of telemetry mapped memory
    uint32_t rd_data_size;
} trp_msg_read_pkg_rsp_t;

typedef struct __attribute__((packed)) {
    uint16_t block_id; // dcp client specific
    uint16_t block_version;
    uint32_t rd_data_ddr_addr_offset; // offset from beginning of telemetry mapped memory
    uint32_t rd_data_size;
} trp_msg_read_block_rsp_t;

typedef struct __attribute__((packed)) {
    trp_msg_hdr_t hdr;
    union __attribute__((packed)) {
        dcp_msg_t                       dcp_msg;                        // TRP_MSG_ID_DCP_FORWARD
        trp_msg_read_pkg_rsp_t          package_notification;           // TRP_MSG_ID_PACKAGE_NOTIFICATION
        trp_msg_read_pkg_rsp_t          read_package_response;          // TRP_MSG_ID_READ_PACKAGE_RESPONSE
        trp_msg_read_pkg_rsp_t          read_package_complete;          // TRP_MSG_ID_READ_PACKAGE_COMPLETE
        trp_msg_read_block_rsp_t        intercore_block_notification;   // TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION
        trp_block_read_req_t            read_intercore_block;           // TRP_MSG_ID_READ_INTERCORE_BLOCK
        trp_msg_read_block_rsp_t        read_intercore_block_rsp;       // TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE
        trp_msg_read_block_rsp_t        read_intercore_block_complete;  // TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE
    } payload;
} trp_msg_t, *p_trp_msg_t;

typedef struct {
    uint8_t die_id; // KNG_DIE_ID
    uint8_t cpu_id; // KNG_CPU_TYPE
} trp_dest_t, *p_trp_dest_t;

typedef struct {
    trp_dest_t dest;
    p_trp_icc_endpoint_t icc_endpoint;
} trp_route_t, *p_trp_route_t;

typedef struct
{
    p_trp_route_t routing_table;
    uint16_t number_of_routes;
    p_trp_icc_endpoint_t endpoint_table;
    uint16_t number_of_endpoints;
    uint8_t this_die_id; // KNG_DIE_ID
    uint8_t this_cpu_id; // KNG_CPU_TYPE
} trp_icc_config_t, *p_trp_icc_config_t;