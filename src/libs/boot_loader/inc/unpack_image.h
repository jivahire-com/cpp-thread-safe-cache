//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file unpack_image.h
 * Header file containing structure definitions and function prototypes used for 
 * decompressing the firmware image
 */
#pragma once

/*----------- Nested includes ------------*/

#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

#define EMBED_HEADER_TAG (0xEBDD4EAD)

/*-------------- Typedefs ----------------*/

typedef struct embed_image_section_s {
    uint32_t compressed_offset;   // Offset from beginning of the embed_image_header to the first byte of the compressed image
    uint32_t compressed_size;     // Compressed size of the image
    uint32_t uncompressed_size;   // Uncompressed size of the image
    uint32_t uncompressed_crc32;  // CRC32 checksum of the uncompressed image
} embed_image_section_t;

typedef struct embed_image_header_s {
    uint32_t embed_image_header_tag;  // 0xEBDD4EAD
    embed_image_section_t itc_ram;  // Location and size of the iram image
    embed_image_section_t dtc_ram;  // Location and size of the dram image
    embed_image_section_t rmss_data_ram;  // Location and size of the rmss data image
} embed_image_header_t;

/*--------- Function Prototypes ----------*/

/**
 *  @brief This function is used unpack the instruction and data sections of the 
 *         compressed firmware image. It takes source and destination as arguments
 *         and invokes zlib decompress APIs.
 *
 *  @note This function is not thread safe.
 *  @note This function is not ISR safe.
 * 
 *  @param[in] source_buffer Base address containing the image header and compress image
 *  @param[in] source_buffer_size Total firmware image size
 *  @param[in] itcram_base Base address of ITCM
 *  @param[in] itcram_size Size of ITCM
 *  @param[in] dtcram_base Base address of DTCM
 *  @param[in] dtcram_size Size of DTCM
 *  @param[in] rmss_data_base Base address of RMSS data region for rodata storage
 *  @param[in] rmss_data_size Size of RMSS data region
 * 
 *  @return
 *      Returns true on success, false on failure
 */

bool unpack_image(size_t    source_buffer,
                  uint32_t  source_buffer_size,
                  size_t    itcram_base,
                  uint32_t  itcram_size,
                  size_t    dtcram_base,
                  uint32_t  dtcram_size,
                  size_t rmss_data_base,
                  uint32_t rmss_data_size);


/**
 *  @brief Function that decompresses given section into memory
 *
 *
 *  @param[in] source               Compressed source buffer
 *  @param[in] source_size          Compressed source size
 *  @param[in] destination          Destination buffer for uncompressed data
 *  @param[in] destination_size     Destination buffer max size
 *
 *  @return
 *      Returns true if decompress succeeded or false if it failed
 */
bool decompress(void* source, uint32_t source_size, void* destination, uint32_t destination_size);