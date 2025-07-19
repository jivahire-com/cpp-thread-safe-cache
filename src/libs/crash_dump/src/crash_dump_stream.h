//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_stream.h
 * Crash Dump stream functionality for aggregating crash dump data.
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <atu_lib.h>            // for atu_map_entry_t
#include <modules/CdFormat.h>   // for DUMP_HEADER, DUMP_METADATA_HEADER, DUMP_PAYLOAD_HEADER
#include <shared_crashdump_def.h> // for CRASH_DUMP_CORE_NUM
#include <stdbool.h>            // for bool
#include <stdint.h>             // for uint32_t, uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef enum
{
    CHUNK_HEADER,
    CHUNK_METADATA_HEADER,
    CHUNK_METADATA_MCP0,
    CHUNK_METADATA_SCP0,
    CHUNK_METADATA_HSP0,
    CHUNK_METADATA_SDM0,
    CHUNK_METADATA_CDED0,
    CHUNK_METADATA_KMP0,
    CHUNK_METADATA_MCP1,
    CHUNK_METADATA_SCP1,
    CHUNK_METADATA_HSP1,
    CHUNK_METADATA_SDM1,
    CHUNK_METADATA_CDED1,
    CHUNK_METADATA_KMP1,
    CHUNK_PAYLOAD_HEADER,
    CHUNK_PAYLOAD_MCP0,
    CHUNK_PAYLOAD_SCP0,
    CHUNK_PAYLOAD_HSP0,
    CHUNK_PAYLOAD_SDM0,
    CHUNK_PAYLOAD_CDED0,
    CHUNK_PAYLOAD_KMP0,
    CHUNK_PAYLOAD_MCP1,
    CHUNK_PAYLOAD_SCP1,
    CHUNK_PAYLOAD_HSP1,
    CHUNK_PAYLOAD_SDM1,
    CHUNK_PAYLOAD_CDED1,
    CHUNK_PAYLOAD_KMP1,
    CHUNK_COUNT
} chunk_index;

typedef struct
{
    uint8_t* start;
    uint32_t size;
} crash_dump_chunk_t;

typedef struct
{
    // Aggregated headers
    DUMP_HEADER header_aggregate;
    DUMP_METADATA_HEADER metadata_header_aggregate;
    DUMP_PAYLOAD_HEADER payload_header_aggregate;

    // Metadata and payload chunks for each core
    crash_dump_chunk_t chunks[CHUNK_COUNT];

    // Stream state
    chunk_index current_chunk_index;
    uint32_t current_chunk_offset;

    // Raw crash dump addresses for each core
    atu_map_entry_t die1_map_entry;
} crash_dump_stream_t;

/*-------- Function Prototypes -----------*/
/**
 * @brief Initialize the crash dump stream
 *
 * @param stream Pointer to the crash dump stream
 * @return true Initialization successful
 * @return false Initialization failed
 */
bool crash_dump_stream_open(crash_dump_stream_t* stream);

/**
 * @brief Close the crash dump stream and optionally invalidate dumps
 *
 * @param stream Pointer to the crash dump stream
 * @param invalidate_dumps If true, invalidates the crash dump headers
 */
void crash_dump_stream_close(crash_dump_stream_t* stream, bool invalidate_dumps);

/**
 * @brief Reset the crash dump stream to the initial state
 *
 * @param stream Pointer to the crash dump stream
 */
void crash_dump_stream_reset(crash_dump_stream_t* stream);

/**
 * @brief Read data from the crash dump stream into a buffer
 *
 * @param stream Pointer to the crash dump stream
 * @param buffer Buffer to read data into
 * @param size Number of bytes to read
 * @return Number of bytes read
 */
uint32_t crash_dump_stream_read(crash_dump_stream_t* stream, uint8_t* buffer, const uint32_t size);