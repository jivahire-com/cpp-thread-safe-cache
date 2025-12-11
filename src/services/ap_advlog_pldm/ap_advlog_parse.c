//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file advlog_parse.c
 *       API implementations for parsing advanced logger logs from DRAM using
 *       memapi.
 */

/*------------- Includes -----------------*/
#include "ap_advlog_parse.h"

#include "ap_advlog_pldm_events.h"

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <cortex_m7_atomics.h>
#include <ddrss_reserved_regions.h>
#include <fpfw_status.h>
#include <kng_scp_tfa_shared.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <zlib.h>

/*-- Symbolic Constant Macros (defines) --*/
// Use the full ARSM DIE 0 SRAM size for zlib workspace
#define ZLIB_POOL_SIZE AP_ADV_LOGGER_COMPRESS_BUFFER_SIZE
// Alignment for zlib buffers (64 bytes for cache line alignment on Cortex-M7)
#define ZLIB_BUFFER_ALIGNMENT 64
// Use small chunk buffers for efficient memory usage
#define IN_CHUNK_BUFFER_SIZE  0x20000
#define OUT_CHUNK_BUFFER_SIZE 0x100000

/*-- Declarations (Statics and globals) --*/
static advanced_logger_info info;
static atu_map_entry_t ap_advlog_buffer_atu_map_struct;
static atu_map_entry_t ap_advlog_output_buffer_atu_map_struct;
static bool atu_mapped = false;

// Deflate workspace buffer for zlib using atu mapped ddr buffer
static uint8_t* zlib_malloc_buffer = (uint8_t*)MSCP_ATU_AP_WINDOW_ADVLOG_COMPRESS_BUFFER_BASE_ADDR;
static size_t zlib_pool_offset = 0;

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

// Custom allocator for zlib using static pool with proper alignment
// Note: Non-static to allow mocking in unit tests
voidpf zlib_alloc(voidpf opaque, uInt items, uInt size)
{
    FPFW_UNUSED(opaque); // ignore unused variable

    size_t total_size = items * size;

    // Align allocation to cache line boundary
    size_t aligned_offset = ((zlib_pool_offset + (ZLIB_BUFFER_ALIGNMENT - 1)) & ~(ZLIB_BUFFER_ALIGNMENT - 1));
    size_t aligned_size = ((total_size + (ZLIB_BUFFER_ALIGNMENT - 1)) & ~(ZLIB_BUFFER_ALIGNMENT - 1));

    if ((aligned_offset + aligned_size) > ZLIB_POOL_SIZE)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] zlib allocation failed: requested 0x%zx bytes (aligned 0x%zx), "
                            "available 0x%zx bytes (used: 0x%zx)\n",
                            total_size,
                            aligned_size,
                            ZLIB_POOL_SIZE - aligned_offset,
                            aligned_offset);
        return Z_NULL;
    }

    void* ptr = zlib_malloc_buffer + aligned_offset;

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] zlib allocated 0x%zx bytes at offset 0x%zx (ptr=0x%p, aligned)\n",
                       total_size,
                       aligned_offset,
                       ptr);

    // Zero out the allocated memory for safety
    for (size_t i = 0; i < total_size; i++)
    {
        *((uint8_t*)ptr + i) = 0;
    }

    zlib_pool_offset = aligned_offset + aligned_size;

    return ptr;
}

// Custom free for zlib (no-op since we use static buffer)
// Note: Non-static to allow mocking in unit tests
void zlib_free(voidpf opaque, voidpf address)
{
    FPFW_UNUSED(opaque);
    FPFW_UNUSED(address);
    // No-op: pool offset is reset before each compression session
}

static fpfw_status_t map_zlib_error(int zret)
{
    switch (zret)
    {
    case Z_MEM_ERROR:
        return FPFW_STATUS_OUT_OF_MEMORY;
    case Z_BUF_ERROR:
        return FPFW_STATUS_BUFFER_TOO_SMALL;
    case Z_STREAM_ERROR:
        return FPFW_STATUS_INVALID_ARGS;
    default:
        return FPFW_STATUS_UNEXPECTED;
    }
}

bool populate_advanced_logger_info()
{
    int status = SILIBS_SUCCESS;
    volatile void* dest = &info;

    if (atu_mapped == false)
    {
        atu_entry_attr_t ap_advlog_buffer_atu_ns_attr = {ATU_BUS_ATTR_NS};

        ap_advlog_buffer_atu_map_struct.ap_base_address = AP_ADV_LOGGER_BUFFER_BASE;
        ap_advlog_buffer_atu_map_struct.mscp_start_address = 0;
        ap_advlog_buffer_atu_map_struct.mscp_end_address = AP_ADV_LOGGER_BUFFER_SIZE - 1;
        ap_advlog_buffer_atu_map_struct.attribute.as_uint32 = ap_advlog_buffer_atu_ns_attr.as_uint32;
        status = atu_map(ATU_ID_MSCP, &ap_advlog_buffer_atu_map_struct);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Advanced Logger buffer mapped");

        ap_advlog_output_buffer_atu_map_struct.ap_base_address = AP_ADV_LOGGER_OUTPUT_BUFFER_BASE;
        ap_advlog_output_buffer_atu_map_struct.mscp_start_address = 0;
        ap_advlog_output_buffer_atu_map_struct.mscp_end_address = AP_ADV_LOGGER_OUTPUT_BUFFER_SIZE - 1;
        ap_advlog_output_buffer_atu_map_struct.attribute.as_uint32 = ap_advlog_buffer_atu_ns_attr.as_uint32;
        status = atu_map(ATU_ID_MSCP, &ap_advlog_output_buffer_atu_map_struct);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Advanced Logger Output buffer mapped");

        atu_mapped = true;
    }

    for (size_t idx = 0; idx < sizeof(advanced_logger_info); idx++)
    {
        uint8_t byte = MMIO_READ8(ap_advlog_buffer_atu_map_struct.mscp_start_address + idx);
        *((uint8_t*)dest + idx) = byte;
    }

    if (info.signature != ADVANCED_LOGGER_SIGNATURE)
    {
        AP_ADVLOG_PLDM_ET_ERROR(AP_ADVLOG_PLDM_ET_TYPE_SIGNATURE_MISMATCH);
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Logger signature mismatch! Expected signature 0x%lx, got "
                            "0x%lx - payload will not be sent!\n",
                            ADVANCED_LOGGER_SIGNATURE,
                            info.signature);
        return false;
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Buffer = 0x%lx\n", info.log_buffer);
    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Current = 0x%lx\n", info.log_current);

    return true;
}

uint64_t get_advanced_logger_base()
{
    return (ap_advlog_buffer_atu_map_struct.mscp_start_address);
}

uint64_t get_advanced_logger_compressed_base()
{
    return (ap_advlog_output_buffer_atu_map_struct.mscp_start_address);
}

uint64_t get_advanced_logger_size()
{
    uint64_t current = info.log_current;
    uint64_t base = info.log_buffer;
    uint64_t log_size = 0x00;

    if (current >= base)
    {
        if ((current - base) > AP_ADV_LOGGER_BUFFER_SIZE)
        {
            AP_ADVLOG_PLDM_ET_ERROR_PARAM(AP_ADVLOG_PLDM_ET_TYPE_SIZE_EXCEEDED, (uint32_t)((current - base) & UINT32_MAX));
            FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Log size exceeds maximum expected size! Size = 0x%llx\n",
                                (current - base));

            log_size = AP_ADV_LOGGER_BUFFER_SIZE;
        }
        else
        {
            log_size = sizeof(advanced_logger_info) + (current - base);
        }
    }
    else
    {
        AP_ADVLOG_PLDM_ET_ERROR_PARAM(AP_ADVLOG_PLDM_ET_TYPE_CORRUPTED, (uint32_t)(current & UINT32_MAX));
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Log corrupted! Current (0x%llx) < Base (0x%llx)!\n", current, base);
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Log Size = 0x%llx\n", log_size);
    return log_size;
}

fpfw_status_t adv_logger_compress(size_t* dst_size)
{
    z_stream strm;
    int err;

    *dst_size = 0;

    // Reset zlib pool for this compression session
    zlib_pool_offset = 0;

    memset(&strm, 0, sizeof(strm));
    strm.zalloc = zlib_alloc;
    strm.zfree = zlib_free;
    strm.opaque = Z_NULL;

    size_t src_len = get_advanced_logger_size();
    uint64_t src_base = get_advanced_logger_base();
    uint64_t dst_base = get_advanced_logger_compressed_base();

    size_t src_offset = 0;
    size_t dst_offset = 0;

    FPFW_DBGPRINT_INFO(
        "[AP_ADVLOG_PLDM] Starting compression: src_base=0x%llx, src_len=0x%zx, dst_base=0x%llx\n",
        src_base,
        src_len,
        dst_base);

    // Initialize deflate with gzip wrapper
    err = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 1, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] deflateInit failed with error code 0x%x\n", err);
        return map_zlib_error(err);
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] deflateInit succeeded\n");

    // Allocate aligned buffers for full input and output from zlib pool
    Bytef* input_buffer = (Bytef*)zlib_alloc(NULL, IN_CHUNK_BUFFER_SIZE, 1);
    if (input_buffer == NULL)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to allocate input buffer\n");
        deflateEnd(&strm);
        return FPFW_STATUS_OUT_OF_MEMORY;
    }

    Bytef* output_buffer = (Bytef*)zlib_alloc(NULL, OUT_CHUNK_BUFFER_SIZE, 1);
    if (output_buffer == NULL)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to allocate output buffer\n");
        deflateEnd(&strm);
        return FPFW_STATUS_OUT_OF_MEMORY;
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Buffer addresses: input=0x%p, output=0x%p (aligned to 0x%x)\n",
                       input_buffer,
                       output_buffer,
                       ZLIB_BUFFER_ALIGNMENT);

    // Setup stream for compression
    strm.next_in = input_buffer;
    strm.avail_in = 0;
    strm.next_out = output_buffer;
    strm.avail_out = OUT_CHUNK_BUFFER_SIZE;

    do
    {
        if (strm.avail_out == 0)
        {
            size_t have = OUT_CHUNK_BUFFER_SIZE - strm.avail_out;

            // Write compressed output to MMIO
            for (size_t i = 0; i < have; i++)
            {
                MMIO_WRITE8(dst_base + dst_offset + i, output_buffer[i]);
            }

            dst_offset += have;
            strm.next_out = output_buffer;
            strm.avail_out = OUT_CHUNK_BUFFER_SIZE;
        }
        if (strm.avail_in == 0)
        {
            strm.next_in = input_buffer;
            strm.avail_in = src_len > IN_CHUNK_BUFFER_SIZE ? IN_CHUNK_BUFFER_SIZE : (uInt)src_len;
            src_len -= strm.avail_in;

            // Read input from MMIO into aligned buffer
            for (size_t i = 0; i < strm.avail_in; i++)
            {
                input_buffer[i] = MMIO_READ8(src_base + src_offset + i);
            }

            src_offset += strm.avail_in;
        }
        err = deflate(&strm, src_len ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);

    if (err != Z_STREAM_END)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] deflate failed with error 0x%x, avail_out=0x%x\n", err, strm.avail_out);
        deflateEnd(&strm);
        return map_zlib_error(err);
    }

    // Write remaining compressed output to MMIO
    size_t have = OUT_CHUNK_BUFFER_SIZE - strm.avail_out;

    for (size_t i = 0; i < have; i++)
    {
        MMIO_WRITE8(dst_base + dst_offset + i, output_buffer[i]);
    }

    // Update total compressed size
    *dst_size = strm.total_out;

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Compression ended with error code 0x%x, dst_size=0x%zx\n", err, *dst_size);

    deflateEnd(&strm);

    if (err == Z_STREAM_END)
    {
        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Compression successful, compressed size: 0x%zx bytes\n", *dst_size);
        return FPFW_STATUS_SUCCESS;
    }

    FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Compression failed with error code 0x%x\n", err);
    return map_zlib_error(err);
}