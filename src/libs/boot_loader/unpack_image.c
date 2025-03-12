//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file unpack_image.c
 * This file provides unpack_image API which will decompress the SCP FW
 * into ITCM and DTCM separately.
 */

#include <stdbool.h>      // for false, bool, true
#include <stddef.h>       // for NULL, size_t
#include <stdint.h>       // for uint32_t, uint8_t
#include <unpack_image.h> // for embed_image_header_t, EMBED_HEADER_TAG
#ifndef UNIT_TEST
    #include <zconf.h> // for MAX_WBITS
#endif
#include <zlib.h> // for inflate, Z_FINISH, Z_OK, Z_STREAM_END, inf...

/*-- Symbolic Constant Macros (defines) --*/
#ifndef UNIT_TEST
    #define ZLIB_WINDOW_SIZE (16 + MAX_WBITS)
#else
extern int ZLIB_WINDOW_SIZE;
#endif
/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

bool decompress(void* source, uint32_t source_size, void* destination, uint32_t destination_size)
{
    z_stream stream = {0};

    if ((source == NULL) || (source_size == 0) || (destination == NULL) || (destination_size == 0))
    {
        return false;
    }

    stream.next_in = source;
    stream.avail_in = source_size;

    stream.next_out = destination;
    stream.avail_out = destination_size;

    stream.zalloc = NULL;
    stream.zfree = NULL;

    // 16 + MAX_WBITS is used to unpack data with a gzip header
    int result = inflateInit2(&stream, ZLIB_WINDOW_SIZE); // Can replace with 32 if needed
    if (result != Z_OK)
    {
        return false;
    }

    result = inflate(&stream, Z_FINISH);

    if (result != Z_STREAM_END)
    {
        return false;
    }

    return true;
}

bool unpack_image(size_t source_buffer,
                  uint32_t source_buffer_size,
                  size_t itc_ram_base,
                  uint32_t itc_ram_size,
                  size_t dtc_ram_base,
                  uint32_t dtc_ram_size,
                  size_t rmss_data_base,
                  uint32_t rmss_data_size)
{
    embed_image_header_t* header = NULL;
    void* itc_ram_compressed_source = NULL;
    void* dtc_ram_compressed_source = NULL;
    void* rmss_data_compressed_source = NULL;

    // Ensure the source buffer has enough size to decode the header
    if (source_buffer_size < sizeof(embed_image_header_t))
    {
        return false;
    }

    header = (embed_image_header_t*)((size_t)source_buffer);

    // Ensure the header is valid by its tag
    //   (if no embedded firmware is included we expect this section to be initialized as zeros)
    if (header == NULL || (header->embed_image_header_tag != EMBED_HEADER_TAG))
    {
        return false;
    }

    // Validate the embedded image IRAM size will fit in the destination ITCRAM
    if (header->itc_ram.uncompressed_size > itc_ram_size)
    {
        return false;
    }

    // Validate the embedded image DRAM size will fit in the destination DTCRAM
    if (header->dtc_ram.uncompressed_size > dtc_ram_size)
    {
        return false;
    }

    // Validate the embedded image RMSS data size will fit in the destination RMSS data region
    if (header->rmss_data_ram.uncompressed_size > rmss_data_size)
    {
        return false;
    }

    // Validate the uncompressed IRAM image
    if (header->itc_ram.compressed_offset + header->itc_ram.compressed_size > source_buffer_size)
    {
        return false;
    }

    // Validate the uncompressed DRAM image
    if (header->dtc_ram.compressed_offset + header->dtc_ram.compressed_size > source_buffer_size)
    {
        return false;
    }

    // Validate the uncompressed RMSS data image
    if (header->rmss_data_ram.compressed_offset + header->rmss_data_ram.compressed_size > source_buffer_size)
    {
        return false;
    }

    // Get pointers to the compressed sources
    itc_ram_compressed_source = (uint8_t*)header + header->itc_ram.compressed_offset;

    // Decompress the IRAM image
    if (!decompress(itc_ram_compressed_source, header->itc_ram.compressed_size, (void*)itc_ram_base, itc_ram_size))
    {
        return false;
    }

    dtc_ram_compressed_source = (uint8_t*)header + header->dtc_ram.compressed_offset;
    // Decompress the DRAM image
    if (!decompress(dtc_ram_compressed_source, header->dtc_ram.compressed_size, (void*)dtc_ram_base, dtc_ram_size))
    {
        return false;
    }

    rmss_data_compressed_source = (uint8_t*)header + header->rmss_data_ram.compressed_offset;
    // Decompress the RMSS data image
    if (!decompress(rmss_data_compressed_source, header->rmss_data_ram.compressed_size, (void*)rmss_data_base, rmss_data_size))
    {
        return false;
    }
    return true;
}