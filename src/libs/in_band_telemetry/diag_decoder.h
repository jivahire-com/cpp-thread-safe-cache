//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file diag_decoder.h
 * Top level headers for in band telemetry decoding.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <IFpFwEventTracingBuffers.h>
#include <build_data.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

#define DIAG_METADATA_SENTINEL (0xE001CD6BU)

/*-------------- Typedefs ----------------*/
/**
 * To properly decode in-band telemetry data, the decoder needs to validate
 * that the data matches it's configuration. The header at the front of every
 * in-band telemetry data buffer is used to determine version per payload type.
 *
 *  1. The diagtool creates a binary manifest for one or more cores.
 *       https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.hostlibs?path=/python_tools/diagtool
 *
 *  2. The datwizard tool adds the binary manifest to the firmware flash image.
 *      https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.datwizard?path=%2F&version=GBpreview&_a=contents
 *
 *  3. At startup, the SCP sends HSP commands to copy the binary manifests from flash to staging DDR.
 *
 *  4. Then the MCP at startup combines the binary manifests into a single combined manifest set, which the host can then read later via a DCP command.
 *
 *  5. Each manifest is of the type packed manifest and includes decode information for both event trace and power telemetry.
 *
 *  6. Therefore the combined manifest set will have this structure:
 *
 *    |--------------------------------------------------------------------------------------|
 *    | diag_manifest_set_v2_header_t   | manifest_set_header_version = 2                    |
 *    |                                 | manifest_count = 3                                 |
 *    |                                 | sentinel = DIAG_METADATA_SENTINEL                  |
 *    |                                 | manifest_set_size = size of data after header      |
 *    |                                 | crc32 = crc of following bytes                     |
 *    | diag_manifest_header_t          | manifest_parser_type = DIAG_MANIFEST_PARSER_PACKED | // payload[0] of diag_manifest_set_v2_header_t
 *    |                                 | manifest_size                                      |
 *    | diag_packed_manifest_header_t   | build_guid                                         | // payload[0] of diag_manifest_header_t
 *    |                                 | provider_metadata_size                             |
 *    |                                 | event_metadata_size                                |
 *    |                                 | provider_metadata                                  | // payload[0] of diag_packed_manifest_header_t
 *    |                                 | event_metadata                                     |
 *
 *                                     ...
 *    diag_manifest_header_t and diag_packed_manifest_header_t are repeated for each manifest
 *
 *    |                                 | event_metadata                                     |
 *    |--------------------------------------------------------------------------------------|
*/

typedef enum
{
    DIAG_MANIFEST_PARSER_PACKED = 1,
    DIAG_MANIFEST_PARSER_HEALTH_COLLECTOR = 2,
} diag_manifest_parser_type_t;

typedef enum {
    DIAG_MANIFEST_SET_HEADER_VERSION_V1 = 1,
    DIAG_MANIFEST_SET_HEADER_VERSION_V2 = 2
} diag_manifest_set_header_version_t;

// header for DIAG_MANIFEST_SET_HEADER_VERSION_V1
typedef struct {
   uint32_t manifest_set_header_version; // diag_manifest_set_header_version_t
   uint32_t manifest_count;
   uint8_t payload[0];
} diag_manifest_set_v1_header_t, *p_diag_manifest_set_v1_header_t;

// header for DIAG_MANIFEST_SET_HEADER_VERSION_V2
typedef struct {
    uint32_t manifest_set_header_version; // diag_manifest_set_header_version_t
    uint32_t manifest_count;
    uint32_t sentinel; // provides quick check to see if header looks valid
    uint32_t manifest_set_size; // everything after this header
    uint32_t crc32;
    uint8_t payload[0];
} diag_manifest_set_v2_header_t, *p_diag_manifest_set_v2_header_t;

typedef struct {
   uint32_t manifest_parser_type; // diag_manifest_parser_type_t
   uint32_t manifest_size;
   uint8_t payload[0];
} diag_manifest_header_t, *p_diag_manifest_header_t;

typedef struct
{
    FPFW_ET_MANIFEST_ID manifest_id;
    uint32_t provider_metadata_size;
    uint32_t event_metadata_size;
    uint8_t payload[0];
} diag_packed_manifest_header_t, *p_diag_packed_manifest_header_t;

typedef enum
{
    DIAG_PAYLOAD_PARSER_TELEMETRY = 1,
    DIAG_PAYLOAD_PARSER_TRACE_DEVICE = 2,
    DIAG_PAYLOAD_PARSER_TRACE_CORE = 3,
    DIAG_PAYLOAD_PARSER_HSP_TRACE = 4,
    DIAG_PAYLOAD_PARSER_HEALTH_COLLECTOR = 5,
    DIAG_PAYLOAD_PARSER_INVALID,
} diag_payload_parser_type_t;

// DIAG_PAYLOAD_PARSER_TELEMETRY
typedef enum {
    DIAG_PWR_TELEMETRY_PAYLOAD_PARSER_V1 = 1,
    DIAG_PWR_TELEMETRY_PAYLOAD_PARSER_V2 = 2,
} diag_pwr_telemetry_payload_parser_version_t;

typedef enum {
    DIAG_TRACE_PAYLOAD_PARSER_VERSION_CORE_PAYLOAD = 1,   // DIAG_PAYLOAD_PARSER_TRACE_CORE
    DIAG_TRACE_PAYLOAD_PARSER_VERSION_DEVICE_PAYLOAD = 2, // DIAG_PAYLOAD_PARSER_TRACE_DEVICE
} diag_trace_payload_parser_version_t;

// DIAG_PAYLOAD_PARSER_HSP_TRACE
typedef enum {
    DIAG_HSP_PAYLOAD_PARSER_VERSION = 1,
} diag_hsp_payload_parser_version_t;

typedef struct
{
    uint32_t payload_parser_version;  // specific to diag_payload_parser_type_t
    uint32_t payload_parser_type;     // diag_payload_parser_type_t
    uint8_t payload[0];
} diag_decoder_payload_header_t, *p_diag_decoder_payload_header_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
