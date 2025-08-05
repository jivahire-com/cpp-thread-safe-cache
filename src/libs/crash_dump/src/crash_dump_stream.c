//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_stream.c
 * Crash Dump stream functionality for aggregating crash dump data.
 */

/*------------- Includes -----------------*/
#include "crash_dump_stream.h"

#include "crash_dump_context.h"
#include "crash_dump_memory.h"

#include <bug_check.h> // for BUG_ASSERT
#include <crash_dump.h>
#include <crash_dump_events.h>
#include <ddrss_reserved_regions.h> // for CRASH_DUMP_RESERVATION_BASE, CRASH_DUMP_RESERVATION_END
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_STREAM_LOG_VERBOSE 1
#ifndef _WIN32
    // FPFW Crashdump library has a bug of calculating the size of aggregate chunk when DUMP_ALIGNMENT is 8.
    // This workaround is removing any un-counted bytes in the aggregate chunk size.
    #define CRASH_DUMP_AGGR_CHUNK_ALIGN_WORKAROUND 1
#endif

#define CRASH_DUMP_DDR_HEADER_SIZE (8 * SL_1KB)
#define CRASH_DUMP_DDR_PER_DIE_SIZE \
    ((CRASH_DUMP_RESERVATION_END - CRASH_DUMP_RESERVATION_BASE - CRASH_DUMP_DDR_HEADER_SIZE) / 2)

#define CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR (CRASH_DUMP_RESERVATION_BASE + CRASH_DUMP_DDR_HEADER_SIZE)
#define CRASH_DUMP_DDR_DIE_1_AP_BASE_ADDR (CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR + CRASH_DUMP_DDR_PER_DIE_SIZE)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Map the crash dump region for die 1
 *
 * @param die1_crashdump_map Pointer to the ATU map entry for die 1
 * @return true Mapping successful
 * @return false Mapping failed
 */
static bool map_die1_crash_dump_region(atu_map_entry_t* die1_crashdump_map)
{
    if (die1_crashdump_map == NULL)
    {
        return false;
    }

    die1_crashdump_map->ap_base_address = CRASH_DUMP_DDR_DIE_1_AP_BASE_ADDR;
    die1_crashdump_map->mscp_start_address = 0;
    die1_crashdump_map->mscp_end_address = CRASH_DUMP_DDR_PER_DIE_SIZE - 1;
    die1_crashdump_map->attribute.axprot1 = ATU_BUS_ATTR_SET; // ATU_BUS_ATTR_NS
    die1_crashdump_map->attribute.axnse = ATU_BUS_ATTR_CLR;   // ATU_BUS_ATTR_NS

    int silib_status = atu_map(ATU_ID_MSCP, die1_crashdump_map);
    if (silib_status != SILIBS_SUCCESS)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_STREAM_ATU_ERROR, silib_status);
        return false;
    }
    return true;
}

/**
 * @brief Unmap the crash dump region for die 1
 *
 * @param die1_crashdump_map Pointer to the ATU map entry for die 1
 * @return true Unmapping successful
 * @return false Unmapping failed
 */
static bool unmap_die1_crash_dump_region(atu_map_entry_t* die1_crashdump_map)
{
    int silib_status = atu_unmap(ATU_ID_MSCP, die1_crashdump_map);
    if (silib_status != SILIBS_SUCCESS)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_STREAM_ATU_ERROR, silib_status);
        return false;
    }

    // Reset the map entry to default values to reuse atu map entry later
    die1_crashdump_map->mscp_start_address = 0;
    die1_crashdump_map->mscp_end_address = CRASH_DUMP_DDR_PER_DIE_SIZE - 1;

    return true;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"
#ifdef CRASH_DUMP_AGGR_CHUNK_ALIGN_WORKAROUND
    #define MAX_AGGR_STACK_SIZE 5
typedef struct
{
    uint8_t* chunk;
    uint32_t subchunk_offset;
    uint32_t processed_subchunks;
} ChunkFrame;

static uint32_t fix_aggr_chunk_size(uint8_t* chunk)
{
    if (chunk == NULL)
    {
        return 0;
    }

    ChunkFrame stack[MAX_AGGR_STACK_SIZE];
    int top = -1;

    uint32_t total_size = 0;

    // Push the initial chunk
    stack[++top] =
        (ChunkFrame){.chunk = chunk, .subchunk_offset = sizeof(DUMP_AGGREGATE_DESCRIPTOR), .processed_subchunks = 0};

    while (top >= 0)
    {
        ChunkFrame* frame = &stack[top];
        DUMP_AGGREGATE_DESCRIPTOR* aggr_desc = (DUMP_AGGREGATE_DESCRIPTOR*)frame->chunk;

        if (frame->processed_subchunks == 0)
        {
            // First time processing this aggregate
            aggr_desc->ChunkDescriptor.PayloadSize = 0;
        }

        if (frame->processed_subchunks >= aggr_desc->ChunkCount)
        {
            // Done processing this aggregate
            uint32_t chunk_total = aggr_desc->ChunkDescriptor.HeaderSize + aggr_desc->ChunkDescriptor.PayloadSize;

            // Pop chunk frame
            top--;
            if (top >= 0)
            {
                // Add this size to the parent
                DUMP_AGGREGATE_DESCRIPTOR* parent_aggr = (DUMP_AGGREGATE_DESCRIPTOR*)stack[top].chunk;
                parent_aggr->ChunkDescriptor.PayloadSize += chunk_total;
                stack[top].subchunk_offset += chunk_total;
                stack[top].processed_subchunks++;
            }
            else
            {
                total_size = chunk_total;
            }
            continue;
        }

        // Process next subchunk
        DUMP_CHUNK_DESCRIPTOR* sub_chunk_desc = (DUMP_CHUNK_DESCRIPTOR*)(frame->chunk + frame->subchunk_offset);
        if (sub_chunk_desc->Magic == DUMP_CHUNK_MAGIC_FINALIZED)
        {
            if (sub_chunk_desc->Type == DumpChunkType_Aggregate)
            {
                // Push nested aggregate
                BUG_ASSERT(top < MAX_AGGR_STACK_SIZE - 2); // Ensure we have space in the stack
                stack[++top] = (ChunkFrame){.chunk = (uint8_t*)sub_chunk_desc,
                                            .subchunk_offset = sizeof(DUMP_AGGREGATE_DESCRIPTOR),
                                            .processed_subchunks = 0};
            }
            else
            {
                uint32_t sub_chunk_size = sub_chunk_desc->HeaderSize + sub_chunk_desc->PayloadSize;
                aggr_desc->ChunkDescriptor.PayloadSize += sub_chunk_size;
                frame->subchunk_offset += sub_chunk_size;
                frame->processed_subchunks++;
            }
        }
        else if (sub_chunk_desc->Magic == 0)
        {
            // Invalid chunk, skip 4 bytes and retry.
            frame->subchunk_offset += 4;
        }
    }

    return total_size;
}
#endif // CRASH_DUMP_AGGR_CHUNK_ALIGN_WORKAROUND

/**
 * @brief Get the size of a finalized chunk
 *
 * @param chunk Pointer to the chunk descriptor
 * @return uint32_t Size of the chunk, or 0 if invalid
 */
static uint32_t get_chunk_size(uint8_t* chunk)
{
    if (chunk == NULL)
    {
        return 0;
    }

    DUMP_CHUNK_DESCRIPTOR* chunk_desc = (DUMP_CHUNK_DESCRIPTOR*)chunk;
    if (chunk_desc->Magic != DUMP_CHUNK_MAGIC_FINALIZED)
    {
        return 0;
    }

    return chunk_desc->HeaderSize + chunk_desc->PayloadSize;
}

/**
 * @brief Open the crash dump stream and initialize the aggregated headers
 *
 * @param stream Pointer to the crash dump stream
 * @return true Stream opened successfully
 * @return false Stream opening failed
 */
bool crash_dump_stream_open(crash_dump_stream_t* stream)
{
    bool ret = false;
    uint64_t total_file_size = 0;
    uint32_t total_metadata_chunk_count = 0;
    uint32_t total_payload_chunk_count = 0;

    if (stream == NULL)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_STREAM_OPEN_ERROR, KNG_E_INVALIDARG);
        return false;
    }

    // Reset the stream state
    memset(stream, 0, sizeof(crash_dump_stream_t));

    // Map the die1 crash dump region
    if (!map_die1_crash_dump_region(&stream->die1_map_entry))
    {
        CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_STREAM_OPEN_ERROR);
        return false;
    }

    // Iterate through each die and core to aggregate crash dump data
    for (KNG_DIE_ID die = DIE_0; die <= DIE_1; die++)
    {
        chunk_index metadata_chunk_index = (die == DIE_0) ? CHUNK_METADATA_MCP0 : CHUNK_METADATA_MCP1;
        chunk_index payload_chunk_index = (die == DIE_0) ? CHUNK_PAYLOAD_MCP0 : CHUNK_PAYLOAD_MCP1;

        for (crash_dump_core_t core = 0; core < CRASH_DUMP_CORE_NUM; core++, metadata_chunk_index++, payload_chunk_index++)
        {
            // Get the crash dump addresses for each core
            uint8_t* crash_dump_address = get_crash_dump_region_address(&stream->die1_map_entry, die, core);
            DUMP_HEADER* header = (DUMP_HEADER*)crash_dump_address;

            if (header->Magic == DUMP_HEADER_MAGIC_COMPLETE)
            {
                // Crash Dump Header is finalized - valid dump
                DUMP_METADATA_HEADER* metadata_header =
                    (DUMP_METADATA_HEADER*)(crash_dump_address + header->MetadataExtent.Offset);
                DUMP_PAYLOAD_HEADER* payload_header =
                    (DUMP_PAYLOAD_HEADER*)(crash_dump_address + header->PayloadExtent.Offset);

                if (total_file_size == 0) // Find first completed crash dump header
                {
                    // Add the size of common headers
                    total_file_size += sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER) + sizeof(DUMP_PAYLOAD_HEADER);

                    // Copy first crash dump header to build aggregated header and set file header stream points
                    stream->header_aggregate = *header;
                    stream->header_aggregate.MetadataExtent.Size = sizeof(DUMP_METADATA_HEADER);
                    stream->header_aggregate.PayloadExtent.Size = sizeof(DUMP_PAYLOAD_HEADER);
                    stream->chunks[CHUNK_HEADER].start = (uint8_t*)&stream->header_aggregate;
                    stream->chunks[CHUNK_HEADER].size = sizeof(DUMP_HEADER);

                    // Copy first metadata header to build aggregated header and set metadata header stream points
                    stream->metadata_header_aggregate = *metadata_header;
                    stream->metadata_header_aggregate.Root.ChunkDescriptor.PayloadSize = 0; // Reset payload size for aggregate
                    stream->metadata_header_aggregate.Root.ChunkCount = 0; // Reset chunk count for aggregate
                    stream->chunks[CHUNK_METADATA_HEADER].start = (uint8_t*)&stream->metadata_header_aggregate;
                    stream->chunks[CHUNK_METADATA_HEADER].size = sizeof(DUMP_METADATA_HEADER);

                    // Copy first payload header to build aggregated header and set payload header stream points
                    stream->payload_header_aggregate = *payload_header;
                    stream->payload_header_aggregate.Root.ChunkDescriptor.PayloadSize = 0; // Reset payload size for aggregate
                    stream->payload_header_aggregate.Root.ChunkCount = 0; // Reset chunk count for aggregate
                    stream->chunks[CHUNK_PAYLOAD_HEADER].start = (uint8_t*)&stream->payload_header_aggregate;
                    stream->chunks[CHUNK_PAYLOAD_HEADER].size = sizeof(DUMP_PAYLOAD_HEADER);
                }

                // Calculate metadata chunk count and payload size
                stream->chunks[metadata_chunk_index].start = (uint8_t*)(metadata_header) + sizeof(DUMP_METADATA_HEADER);
                stream->chunks[metadata_chunk_index].size = 0;
                uint32_t chunk_offset = 0;

#ifdef CRASH_DUMP_AGGR_CHUNK_ALIGN_WORKAROUND
                // Workaround for aggregate chunk size alignment
                fix_aggr_chunk_size((uint8_t*)&metadata_header->Root);
#endif

                for (uint32_t i = 0; i < metadata_header->Root.ChunkCount; i++)
                {
                    uint32_t chunk_size = get_chunk_size(stream->chunks[metadata_chunk_index].start + chunk_offset);
                    stream->chunks[metadata_chunk_index].size += chunk_size;

                    stream->metadata_header_aggregate.Root.ChunkDescriptor.PayloadSize += chunk_size;

                    chunk_offset += chunk_size;
                    total_file_size += chunk_size;
                }
                total_metadata_chunk_count += metadata_header->Root.ChunkCount;

                // Calculate payload size
                stream->chunks[payload_chunk_index].start = (uint8_t*)(payload_header) + sizeof(DUMP_PAYLOAD_HEADER);
                stream->chunks[payload_chunk_index].size = 0;
                chunk_offset = 0;

#ifdef CRASH_DUMP_AGGR_CHUNK_ALIGN_WORKAROUND
                // Workaround for aggregate chunk size alignment
                fix_aggr_chunk_size((uint8_t*)&payload_header->Root);
#endif

                for (uint32_t i = 0; i < payload_header->Root.ChunkCount; i++)
                {
                    uint32_t chunk_size = get_chunk_size(stream->chunks[payload_chunk_index].start + chunk_offset);
                    stream->chunks[payload_chunk_index].size += chunk_size;

                    stream->payload_header_aggregate.Root.ChunkDescriptor.PayloadSize += chunk_size;

                    chunk_offset += chunk_size;
                    total_file_size += chunk_size;
                }
                total_payload_chunk_count += payload_header->Root.ChunkCount;

                ret = true; // Found a valid crash dump header
            }
        }
    }

    // Update the aggregated headers with the total file size and metadata/payload extents
    stream->header_aggregate.FileSize = total_file_size;
    stream->header_aggregate.MetadataExtent.Offset =
        (uint8_t*)&stream->metadata_header_aggregate - (uint8_t*)&stream->header_aggregate;
    stream->header_aggregate.MetadataExtent.Size += stream->metadata_header_aggregate.Root.ChunkDescriptor.PayloadSize;
    stream->header_aggregate.PayloadExtent.Offset =
        (uint8_t*)&stream->payload_header_aggregate - (uint8_t*)&stream->header_aggregate;
    stream->header_aggregate.PayloadExtent.Size += stream->payload_header_aggregate.Root.ChunkDescriptor.PayloadSize;

    stream->metadata_header_aggregate.Root.ChunkCount = total_metadata_chunk_count;
    stream->payload_header_aggregate.Root.ChunkCount = total_payload_chunk_count;

    if (ret)
    {
        CRASH_DUMP_ET_INFO(CRASH_DUMP_ET_TYPE_STREAM_OPEN);
    }
    else
    {
        CRASH_DUMP_ET_WARNING(CRASH_DUMP_ET_TYPE_STREAM_OPEN_EMPTY);
    }

    return ret;
}
#pragma GCC diagnostic pop

/**
 * @brief Close the crash dump stream and invalidate dumps if requested
 *
 * @param stream Pointer to the crash dump stream
 * @param invalidate_dumps If true, invalidate the crash dump headers
 */
void crash_dump_stream_close(crash_dump_stream_t* stream, bool invalidate_dumps)
{
    if (stream == NULL)
    {
        return;
    }

    if (invalidate_dumps)
    {
        // Invalidate the crash dump header magic to indicate no valid dump
        for (KNG_DIE_ID die = DIE_0; die <= DIE_1; die++)
        {
            for (crash_dump_core_t core = 0; core < CRASH_DUMP_CORE_NUM; core++)
            {
                // Get the crash dump addresses for each core
                DUMP_HEADER* header = (DUMP_HEADER*)get_crash_dump_region_address(&stream->die1_map_entry, die, core);
                if (header != NULL)
                {
                    header->Magic = 0; // Invalidate the magic number
                }
            }
        }
    }

    // Unmap the die1 crash dump region
    unmap_die1_crash_dump_region(&stream->die1_map_entry);
}

/**
 * @brief Reset the crash dump stream to the initial state
 *
 * @param stream Pointer to the crash dump stream
 */
void crash_dump_stream_reset(crash_dump_stream_t* stream)
{
    if (stream == NULL)
    {
        return;
    }

    stream->current_chunk_index = CHUNK_HEADER;
    stream->current_chunk_offset = 0;
}

/**
 * @brief Read data from the crash dump stream into a buffer
 *
 * @param stream Pointer to the crash dump stream
 * @param buffer Pointer to the buffer to read data into
 * @param size Number of bytes to read
 * @return uint32_t Number of bytes read, or 0 if no more data
 */
uint32_t crash_dump_stream_read(crash_dump_stream_t* stream, uint8_t* buffer, const uint32_t size)
{
    uint32_t bytes_read = 0;

    if (stream == NULL || buffer == NULL || size == 0)
    {
        return bytes_read;
    }

    while (stream->current_chunk_index < CHUNK_COUNT)
    {
        crash_dump_chunk_t* chunk = &stream->chunks[stream->current_chunk_index];

        if (chunk->size > stream->current_chunk_offset)
        {
            uint32_t remaining_size = chunk->size - stream->current_chunk_offset; // remaining size of current chunk
            uint32_t bytes_to_read = (remaining_size < (size - bytes_read) ? remaining_size : (size - bytes_read));

            if (bytes_to_read > 0)
            {
                // memcpy(buffer + bytes_read,
                //        (uint8_t*)(chunk->start + stream->current_chunk_offset),
                //        bytes_to_read);
                // buffer or size can be un-aligned, so we copy byte by byte
                for (uint32_t i = 0; i < bytes_to_read; i++)
                {
                    buffer[bytes_read + i] = *((uint8_t*)(chunk->start + stream->current_chunk_offset + i));
                }
                bytes_read += bytes_to_read;
                stream->current_chunk_offset += bytes_to_read;

                if (stream->current_chunk_offset >= chunk->size)
                {
                    stream->current_chunk_index++;    // Move to the next chunk
                    stream->current_chunk_offset = 0; // Reset offset for the next chunk
                }
            }

            if (bytes_read >= size)
            {
                break; // Read enough data
            }
        }
        else
        {
            stream->current_chunk_index++;    // Move to the next chunk
            stream->current_chunk_offset = 0; // Reset offset for the next chunk
        }
    }

    return bytes_read;
}