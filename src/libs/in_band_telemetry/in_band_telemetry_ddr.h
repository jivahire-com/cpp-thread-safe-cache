//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file in_band_telemetry_ddr.h
 *  Header for any in-band telemetry defines, macros, etc.. to reflect how the
 *  DDR is split up for in-band telemetry data.
*/

#pragma once

/*----------- Nested includes ------------*/

#include <memory_map/ddrss_reserved_regions.h>
#include <stdint.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * All in-band telemetry data is transferred into DDR (to support reading by the host).
 * All clients share the same range of DDR, and therefore it needs to split it up by
 * client and die.
*/

#define IB_TELEMETRY_DDR_TOTAL_BASE_ADDR (IB_TELEMETRY_RESERVATION_BASE)
#define IB_TELEMETRY_DDR_TOTAL_END_ADDR (IB_TELEMETRY_RESERVATION_END)
#define IB_TELEMETRY_DDR_TOTAL_SIZE (IB_TELEMETRY_RESERVATION_END - IB_TELEMETRY_RESERVATION_BASE)

#define IB_TELEMETRY_DDR_DIE_0_BASE_ADDR (IB_TELEMETRY_DDR_TOTAL_BASE_ADDR)
#define IB_TELEMETRY_DDR_DIE_0_END_ADDR (IB_TELEMETRY_DDR_TOTAL_BASE_ADDR + (IB_TELEMETRY_DDR_TOTAL_SIZE / 2))
#define IB_TELEMETRY_DDR_DIE_0_SIZE (IB_TELEMETRY_DDR_DIE_0_END_ADDR - IB_TELEMETRY_DDR_DIE_0_BASE_ADDR)

#define IB_TELEMETRY_DDR_DIE_1_BASE_ADDR (IB_TELEMETRY_DDR_DIE_0_END_ADDR)
#define IB_TELEMETRY_DDR_DIE_1_END_ADDR (IB_TELEMETRY_DDR_TOTAL_END_ADDR)
#define IB_TELEMETRY_DDR_DIE_1_SIZE (IB_TELEMETRY_DDR_DIE_1_END_ADDR - IB_TELEMETRY_DDR_DIE_1_BASE_ADDR)

static_assert(IB_TELEMETRY_DDR_TOTAL_SIZE == (IB_TELEMETRY_DDR_DIE_0_SIZE + IB_TELEMETRY_DDR_DIE_1_SIZE), "Die DDR sizes do not add up to total DDR size");

/**
 * Event Trace uses 16 MB of DDR per die, and starts at the base of each
 * die's DDR range. It's used for both ASIC buffers and HSP buffers.
*/

#define IB_TELEMETRY_DDR_DIE_TRACE_SIZE (MB * 16)
#define IB_TELEMETRY_DDR_DIE_TRACE_BASE_ADDR(die) ((die) == 0 ? IB_TELEMETRY_DDR_DIE_0_BASE_ADDR : IB_TELEMETRY_DDR_DIE_1_BASE_ADDR)
#define IB_TELEMETRY_DDR_DIE_TRACE_END_ADDR(die) (IB_TELEMETRY_DDR_DIE_TRACE_BASE_ADDR(die) + IB_TELEMETRY_DDR_DIE_TRACE_SIZE)

#define IB_TELEMETRY_DDR_DIE_TRACE_HSP_SIZE (1 * MB)
#define IB_TELEMETRY_DDR_DIE_TRACE_HSP_BASE_ADDR(die) (IB_TELEMETRY_DDR_DIE_TRACE_BASE_ADDR(die))
#define IB_TELEMETRY_DDR_DIE_TRACE_HSP_END_ADDR(die) (IB_TELEMETRY_DDR_DIE_TRACE_HSP_BASE_ADDR(die) + IB_TELEMETRY_DDR_DIE_TRACE_HSP_SIZE)

#define IB_TELEMETRY_DDR_DIE_TRACE_ASIC_SIZE (15 * MB)
#define IB_TELEMETRY_DDR_DIE_TRACE_ASIC_BASE_ADDR(die) (IB_TELEMETRY_DDR_DIE_TRACE_HSP_END_ADDR(die))
#define IB_TELEMETRY_DDR_DIE_TRACE_ASIC_END_ADDR(die) (IB_TELEMETRY_DDR_DIE_TRACE_ASIC_BASE_ADDR(die) + IB_TELEMETRY_DDR_DIE_TRACE_ASIC_SIZE)

static_assert(IB_TELEMETRY_DDR_DIE_TRACE_SIZE == (IB_TELEMETRY_DDR_DIE_TRACE_HSP_SIZE + IB_TELEMETRY_DDR_DIE_TRACE_ASIC_SIZE), "Trace DDR DIE sizes do not add up to total DDR DIE Trace size");

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
