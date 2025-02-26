//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_collection_protocol.h
 * Public header matching the Data Collection Protocol
 *
 * See the specification for more details:
 *   https://microsoft.sharepoint.com/:f:/r/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Firmware/Telemetry/Data_Collection_Protocol?csf=1&web=1&e=g94MgL
 *
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

// note: this header implements version 1.0.3 of the DCP protocol
#define DCP_PROTOCOL_VERSION_MAJOR (1)
#define DCP_PROTOCOL_VERSION_MINOR (0)
#define DCP_PROTOCOL_VERSION_PATCH (4)

#define DCP_PLATFORM_COBALT_100 (1)
#define DCP_PLATFORM_COBALT_200 (2)

#define DCP_MAX_ENABLE_DISABLE_EVENTS (64)

/*-------------- Typedefs ----------------*/

// clang-format off


typedef enum {
    DATA_COLLECTION_RD_DATA_VALID_MORE  = 3,
    DATA_COLLECTION_RD_DATA_VALID_LAST  = 2,
    DATA_COLLECTION_RD_DATA_NONE        = 1,
    DCP_STATUS_SUCCESS                  = 0,
    DCP_STATUS_E_SIZE                   = -1,
    DCP_STATUS_E_PARAM                  = -2,
    DCP_STATUS_E_BUSY                   = -3,
    DCP_STATUS_E_UNK_MSG                = -4,
    DCP_STATUS_E_UNK_CLIENT             = -5,
    DCP_STATUS_E_INCOMPLETE_HANDLER     = -6,
    DCP_STATUS_E_DEPRECATED_MSG         = -7,
    DCP_STATUS_E_BUFFER_DISCARD         = -8,
    DATA_COLLECTION_E_UNSUPPORTED_MSG   = -9,
} dcp_status_t, *p_dcp_status_t;

typedef enum {
    DCP_CLIENT_ID_DCS            = 0,
    DCP_CLIENT_ID_PWR_INST_TELEM = 1,
    DCP_CLIENT_ID_EVENT_TRACE    = 2,
    DCP_CLIENT_ID_MAX            = 3,
} dcp_client_id_t, *p_dcp_client_id_t;

typedef enum {
    DCP_MSG_ID_GET_CAPABILITIES     = 0x0,
    DCP_MSG_ID_GET_STATE            = 0x1,
    DCP_MSG_ID_GET_MANIFEST         = 0x2,
    DCP_MSG_ID_EVENTS_ENABLE_DISABLE= 0x3,
    DCP_MSG_ID_EVENTS_DISABLE       = 0x4,  // Deprecated - Keeping for backward compatibility
    DCP_MSG_ID_START_STOP           = 0x5,
    DCP_MSG_ID_STOP                 = 0x6,  // Deprecated - Keeping for backward compatibility
    DCP_MSG_ID_READ_DATA            = 0x7,
    DCP_MSG_ID_READ_DATA_COMPLETE   = 0x8,
    DCP_MSG_ID_UTC_SYNC             = 0x9,  // Deprecated - Keeping for backward compatibility
    DCP_MSG_ID_RESET                = 0xA,
    DCP_MSG_ID_NOTIFICATION         = 0xB,
    DCP_MSG_ID_GET_PLAT_INFO        = 0xC,
    DCP_MSG_ID_MAX                  = 0xD,
} dcp_msg_id_t, *p_dcp_msg_id_t;
// clang-format on



typedef struct __attribute__((packed)) {
    uint16_t client_id;     // dcp_client_id_t
    uint16_t msg_id;        // dcp_msg_id_t
    uint16_t seq_num;
    int16_t  msg_status;    // dcp_status_t
    uint16_t payload_size;
} dcp_msg_hdr_t, *p_dcp_msg_hdr_t;

/* CLIENT_GET_CAPABILITIES */

typedef struct __attribute__((packed)) {
    union {
        struct __attribute__((packed)) {
            uint32_t DCP_MSG_ID_GET_CAPABILITIES     : 1;
            uint32_t DCP_MSG_ID_GET_STATE            : 1;
            uint32_t DCP_MSG_ID_GET_MANIFEST         : 1;
            uint32_t DCP_MSG_ID_EVENTS_ENABLE_DISABLE: 1;
            uint32_t DCP_MSG_ID_EVENTS_DISABLE       : 1;
            uint32_t DCP_MSG_ID_START_STOP           : 1;
            uint32_t DCP_MSG_ID_STOP                 : 1;
            uint32_t DCP_MSG_ID_READ_DATA            : 1;
            uint32_t DCP_MSG_ID_READ_DATA_COMPLETE   : 1;
            uint32_t DCP_MSG_ID_UTC_SYNC             : 1;
            uint32_t DCP_MSG_ID_RESET                : 1;
            uint32_t DCP_MSG_ID_NOTIFICATION         : 1;
            uint32_t DCP_MSG_ID_GET_PLAT_INFO        : 1;
            uint32_t reserved                        : 19; // Remaining bits to make up 32 bits
        };
        uint32_t as_uint32;
    } caps;
} dcp_msg_get_caps_t, *p_dcp_msg_get_caps_t;

/* CLIENT_GET_STATE */

typedef enum {
    DCP_CLIENT_STATE_STOPPED = 0,
    DCP_CLIENT_STATE_RUNNING = 1,
} dcp_client_state_t;

typedef struct {
    uint32_t state; // dcp_client_state_t
} dcp_msg_get_client_state_t, *p_dcp_msg_get_client_state_t;

/* CLIENT_GET_MANIFEST */

typedef struct __attribute__((packed)) {
    uint64_t physical_start_addr;
    uint64_t start_addr_offset;
    uint64_t total_size;
} dcp_msg_get_manifest_t, *p_dcp_msg_get_manifest_t;

/* CLIENT_EVENTS_ENABLE_DISABLE */

typedef enum {
    DCP_EVENTS_ENABLE_STATE_DISABLE = 0,
    DCP_EVENTS_ENABLE_STATE_ENABLE  = 1,
} dcp_events_enable_state_t, *p_dcp_events_enable_state_t;

typedef struct __attribute__((packed)) {
    uint16_t number_of_events;
    struct __attribute__((packed)) {
        uint16_t provider_id;
        uint16_t event_id;
        uint16_t state; // dcp_events_enable_state_t
    } events[DCP_MAX_ENABLE_DISABLE_EVENTS];       // max events per message
} dcp_msg_events_enable_disable_t, *p_dcp_msg_events_enable_disable_t;

/* CLIENT_START_STOP */

typedef enum {
    DCP_START_STOP_STATE_STOP = 0,
    DCP_START_STOP_STATE_START = 1,
} dcp_start_stop_state_t, *p_dcp_start_stop_state_t;

typedef struct {
    uint32_t state; // dcp_start_stop_state_t
} dcp_msg_start_stop_t, *p_dcp_msg_start_stop_t;

/* CLIENT_READ_DATA */

typedef struct __attribute__((packed)) {
    uint64_t physical_start_addr;
    uint64_t physical_buffer_size;
    uint64_t rd_data_addr_offset;
    uint64_t rd_data_size;
    uint32_t crc;
} dcp_msg_read_data_t, *p_dcp_msg_read_data_t;

/* CLIENT_READ_DATA_COMPLETE */

typedef struct __attribute__((packed)) {
    uint32_t reserved1;
    uint64_t rd_data_addr_offset;
    uint64_t rd_data_size;
    uint32_t reserved2;
} dcp_msg_read_data_complete_t, *p_dcp_msg_read_data_complete_t;

/* CLIENT_RESET */

// No payload other than the header

/* CLIENT_NOTIFICATION */

typedef enum {
    DCP_NOTIFICATION_TYPE_DATA_AVAILABLE = 1,
    DCP_NOTIFICATION_TYPE_FW_RESET       = 2,
} dcp_notification_type_t, *p_dcp_notification_type_t;

typedef struct {
    uint32_t type; //dcp_notification_type_t
} dcp_msg_notification_t, *p_dcp_msg_notification_t;

/* CLIENT_GET_PLATFORM_INFORMATION */
typedef struct __attribute__((packed)) {
    uint32_t ifwi_ver_major;
    uint32_t ifwi_ver_minor;
    uint32_t ifwi_ver_patch;
    uint32_t ifwi_ver_rev;
} dcp_msg_ifwi_version_t, *p_dcp_msg_ifwi_version_t;

typedef struct __attribute__((packed)) {
    uint32_t dcp_ver_major;
    uint32_t dcp_ver_minor;
    uint32_t dcp_ver_patch;
    dcp_msg_ifwi_version_t ifwi_ver;
    uint64_t plat_id;
} dcp_msg_get_plat_info_t, *p_dcp_msg_get_plat_info_t;

/* Messages */

typedef struct __attribute__((packed)) {
    dcp_msg_hdr_t hdr;
    union __attribute__((packed)) {
        dcp_msg_get_caps_t get_caps;                            // DCP_MSG_ID_GET_CAPABILITIES
        dcp_msg_get_client_state_t get_state;                   // DCP_MSG_ID_GET_STATE
        dcp_msg_get_manifest_t get_manifest;                    // DCP_MSG_ID_GET_MANIFEST
        dcp_msg_events_enable_disable_t events_enable_disable;  // DCP_MSG_ID_EVENTS_ENABLE_DISABLE
        dcp_msg_start_stop_t start_stop;                        // DCP_MSG_ID_START_STOP
        dcp_msg_read_data_t read_data;                          // DCP_MSG_ID_READ_DATA
        dcp_msg_read_data_complete_t read_data_complete;        // DCP_MSG_ID_READ_DATA_COMPLETE
        dcp_msg_notification_t notification;                    // DCP_MSG_ID_NOTIFICATION
        dcp_msg_get_plat_info_t get_plat_info;                  // DCP_MSG_ID_GET_PLAT_INFO
    } payload;
} dcp_msg_t, *p_dcp_msg_t;
