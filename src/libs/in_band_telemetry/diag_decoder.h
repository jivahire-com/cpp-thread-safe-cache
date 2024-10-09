//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file diag_decoder.h
 * Top level headers for in band telemetry decoding.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * To properly decode in-band telemetry data, the decoder needs to validate
 * that the data matches it's configuration. The header at the front of every
 * in-band telemetry data buffer is used to determine version per strategy.
 *
 * See: https://azurecsi.visualstudio.com/DuvallFw/_git/1pfw.hostlibs?path=/modules/DiagnosticDecoder/inc/DiagnosticDecoderTypes.h
 *
*/

typedef enum {
    PP_TELEMETRY_ENCODER_VERSION = 1,
} pp_telemetry_encoder_version_t;

typedef enum {
    TRACE_ENCODER_VERSION_CORE_PAYLOAD = 1,
    TRACE_ENCODER_VERSION_DEVICE_PAYLOAD = 2,
} trace_encoder_version_t;

typedef enum {
    HSP_ENCODER_VERSION = 1,
} hsp_encoder_version_t;

typedef enum {
    PACKED_MANIFEST_ENCODER_VERSION = 1,
} packed_manifest_encoder_version_t;

typedef enum
{
    DIAGNOSTIC_DECODER_STRATEGY_ID_TELEMETRY = 1,
    DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_DEVICE = 2,
    DIAGNOSTIC_DECODER_STRATEGY_ID_TRACE_CORE = 3,
    DIAGNOSTIC_DECODER_STRATEGY_ID_HSP_TRACE = 4,
    DIAGNOSTIC_DECODER_STRATEGY_ID_RESERVED = 5,
    DIAGNOSTIC_DECODER_STRATEGY_ID_INVALID,
} diag_decoder_strategy_id_t;

typedef struct PAYLOAD_HEADER
{
    uint32_t encoder_version;
    uint32_t strategy_id;
    uint8_t payload[0];
} diag_decoder_payload_header_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
