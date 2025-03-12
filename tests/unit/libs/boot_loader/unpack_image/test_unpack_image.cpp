//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_unpack_image.cpp
 * Unit test code for unpack image in boot loader
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>    // for CmockaWrapperTest, TEST_FUNCTION
#include <cstdint>            // for uint32_t, uintptr_t, uint8_t
#include <string.h>           // for size_t, memcpy, memset, NULL
#include <vcruntime_string.h> // for memcmp

extern "C" {
#include <kingsgate_boot.h>
#include <unpack_image.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ZLIB_FAILED_WINDOW_SIZE (-16)
#define MAX_WBITS               (15)

#define MEM_CMP_SUCCESS (0)

// Unit test sample used compress and uncompressed bytes count
#define ITCM_COMPRESSED_STRING_SIZE        (40)
#define ITCM_UNCOMPRESSED_STRING_SIZE      (10)
#define DTCM_COMPRESSED_STRING_SIZE        (40)
#define DTCM_UNCOMPRESSED_STRING_SIZE      (10)
#define RMSS_DATA_COMPRESSED_STRING_SIZE   (40)
#define RMSS_DATA_UNCOMPRESSED_STRING_SIZE (10)
#define ITCM_UNCOMPRESSED_CRC32            (0x1A58A044)
#define DTCM_UNCOMPRESSED_CRC32            (0xEFA62BF4)
#define RMSS_DATA_UNCOMPRESSED_CRC32       (0xEFA62BF4)

static const char* ITCM_UNCOMPRESSED_STRING = "ITCM TEST\n";
static const char* DTCM_UNCOMPRESSED_STRING = "DTCM TEST\n";
static const char* RMSS_DATA_UNCOMPRESSED_STRING = "DTCM TEST\n";

/*------------- Typedefs -----------------*/
typedef struct _main_image_s
{
    embed_image_header_t image_header;
    uint8_t compressed_itcm[ITCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_dtcm[DTCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE];
} main_image_test_t;

/*------------------- Declarations (Statics and globals) --------------------*/
int ZLIB_WINDOW_SIZE = (16 + MAX_WBITS); // Window value passed to inflateInit2

const uint32_t SCP_ITCM_RAM_SIZE = (512 * 1024);
const uint32_t SCP_DTCM_RAM_SIZE = (512 * 1024);
const uint32_t SCP_RMSS_DATA_SIZE = (192 * 1024);

static uint8_t test_itc_ram[SCP_ITCM_RAM_SIZE];
static uint8_t test_dtc_ram[SCP_DTCM_RAM_SIZE];
static uint8_t test_rmss_data[SCP_RMSS_DATA_SIZE];

static kingsgate_boot_metadata_t test_boot_meta;

static main_image_test_t test_main_data = {
    .image_header = {.embed_image_header_tag = EMBED_HEADER_TAG,
                     .itc_ram = {.compressed_offset = sizeof(embed_image_header_t),
                                 .compressed_size = ITCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = ITCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = ITCM_UNCOMPRESSED_CRC32},

                     .dtc_ram = {.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE,
                                 .compressed_size = DTCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = DTCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = DTCM_UNCOMPRESSED_CRC32},

                     .rmss_data_ram = {.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE + DTCM_COMPRESSED_STRING_SIZE,
                                       .compressed_size = RMSS_DATA_COMPRESSED_STRING_SIZE,
                                       .uncompressed_size = RMSS_DATA_UNCOMPRESSED_STRING_SIZE,
                                       .uncompressed_crc32 = RMSS_DATA_UNCOMPRESSED_CRC32}},

    // Compressed bytes for string "ITCM TEST\n"	, length being higher due to zlib headers
    .compressed_itcm = {0x1f, 0x8b, 0x08, 0x08, 0xea, 0x6a, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0xf3, 0x0c, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0x44, 0xa0, 0x58, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00},

    // Compressed bytes for string "DTCM TEST\n"	, length being higher due to zlib headers
    .compressed_dtcm = {0x1f, 0x8b, 0x08, 0x08, 0x0d, 0x6d, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0x73, 0x09, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0xf4, 0x2b, 0xa6, 0xef, 0x0a, 0x00, 0x00, 0x00, 0x00},

    // Compressed bytes for string "RMSS TEST\n"	, length being higher due to zlib headers
    .compressed_rmss = {0x1f, 0x8b, 0x08, 0x08, 0x0d, 0x6d, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0x73, 0x09, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0xf4, 0x2b, 0xa6, 0xef, 0x0a, 0x00, 0x00, 0x00, 0x00}};

static kingsgate_boot_config_t test_boot_config = {
    .data_src_base = (size_t)&test_main_data,
    .data_src_end = (size_t)&test_main_data.compressed_rmss[RMSS_DATA_COMPRESSED_STRING_SIZE - 1],
    .itc_ram_base = (size_t)&test_itc_ram[0],
    .itc_ram_size = SCP_ITCM_RAM_SIZE,
    .dtc_ram_base = (size_t)&test_dtc_ram[0],
    .dtc_ram_size = SCP_DTCM_RAM_SIZE,
    .rmss_data_base = (size_t)&test_rmss_data[0],
    .rmss_data_size = SCP_RMSS_DATA_SIZE,
    .boot_meta_base = (size_t)&test_boot_meta,
    .cpu_type = MSCP_CPU_SCP}; // This boot config is expected to work correctly when passed as is to unpack_image

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
//
// Mocks
//

//
// Tests
//
TEST_FUNCTION(test_inflate_init2_fail, nullptr, nullptr)
{
    uint8_t* source_arr = &test_main_data.compressed_itcm[0];
    uint32_t source_size = ITCM_COMPRESSED_STRING_SIZE;
    uint8_t* dest_arr = &test_itc_ram[0];
    uint32_t dest_size = ITCM_UNCOMPRESSED_STRING_SIZE;

    // Injecting error zlib window size to make zlib inflate fail
    ZLIB_WINDOW_SIZE = ZLIB_FAILED_WINDOW_SIZE;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));

    // Reverting the value to right
    ZLIB_WINDOW_SIZE = (16 + MAX_WBITS);
}

TEST_FUNCTION(test_decompress_null_src_fail, nullptr, nullptr)
{
    // Error injecting with source compressed being NULL
    uint8_t* source_arr = NULL;
    uint32_t source_size = ITCM_COMPRESSED_STRING_SIZE;
    uint8_t* dest_arr = &test_itc_ram[0];
    uint32_t dest_size = ITCM_UNCOMPRESSED_STRING_SIZE;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));
}

TEST_FUNCTION(test_decompress_null_dest_fail, nullptr, nullptr)
{

    uint8_t* source_arr = &test_main_data.compressed_itcm[0];
    uint32_t source_size = ITCM_COMPRESSED_STRING_SIZE;
    // Error injecting with dest uncompressed being NULL
    uint8_t* dest_arr = NULL;
    uint32_t dest_size = ITCM_UNCOMPRESSED_STRING_SIZE;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));
}

TEST_FUNCTION(test_decompress_src_zero_size_fail, nullptr, nullptr)
{
    uint8_t* source_arr = &test_main_data.compressed_itcm[0];
    // Error injecting forcing compressed buffer size to 0
    uint32_t source_size = 0;
    uint8_t* dest_arr = &test_itc_ram[0];
    uint32_t dest_size = ITCM_UNCOMPRESSED_STRING_SIZE;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));
}

TEST_FUNCTION(test_decompress_dest_zero_size_fail, nullptr, nullptr)
{
    uint8_t* source_arr = &test_main_data.compressed_itcm[0];
    uint32_t source_size = ITCM_COMPRESSED_STRING_SIZE;
    uint8_t* dest_arr = &test_itc_ram[0];
    // Error injecting forcing uncompressed buffer size to 0
    uint32_t dest_size = 0;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));
}

TEST_FUNCTION(test_decompress_invalid_src_val_fail, nullptr, nullptr)
{
    // Error injecting using garbage data for source compressed buffer
    uint8_t source_arr[ITCM_COMPRESSED_STRING_SIZE];
    uint32_t source_size = ITCM_COMPRESSED_STRING_SIZE;
    uint8_t* dest_arr = &test_itc_ram[0];
    uint32_t dest_size = ITCM_UNCOMPRESSED_STRING_SIZE;

    assert_false(decompress((void*)source_arr, source_size, (void*)dest_arr, dest_size));
}

// Confirms the decompress API works correctly with proper input - this only uses the ITCM string for testing
TEST_FUNCTION(test_decompress_success, nullptr, nullptr)
{
    uint8_t dest_arr[ITCM_UNCOMPRESSED_STRING_SIZE];
    memset(dest_arr, 0, ITCM_UNCOMPRESSED_STRING_SIZE);

    assert_true(decompress((void*)test_main_data.compressed_itcm, ITCM_COMPRESSED_STRING_SIZE, (void*)dest_arr, ITCM_UNCOMPRESSED_STRING_SIZE));
    assert_int_equal(memcmp(dest_arr, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE), MEM_CMP_SUCCESS);
}

TEST_FUNCTION(test_unpack_image_size_failure, nullptr, nullptr)
{
    uint32_t source_buffer_size = 0;

    // Error injection with source buffer size to 0
    assert_false(unpack_image((uintptr_t)&test_main_data.compressed_itcm,
                              source_buffer_size,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
}

TEST_FUNCTION(test_unpack_image_null_source, nullptr, nullptr)
{
    // Error inject with no source buffer
    uint8_t* source_buffer = NULL;

    assert_false(unpack_image((size_t)source_buffer,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
}

TEST_FUNCTION(test_unpack_image_invalid_header_tag, nullptr, nullptr)
{
    // Error inject with invalid header value
    test_main_data.image_header.embed_image_header_tag = 0x0;
    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));

    // Reverting the header to correct one
    test_main_data.image_header.embed_image_header_tag = EMBED_HEADER_TAG;
}

TEST_FUNCTION(test_unpack_image_invalid_uncompressed_itcm_size, nullptr, nullptr)
{
    // Error injecting through header meta data for uncompressed data to be higher than ITCM size
    test_main_data.image_header.itc_ram.uncompressed_size = (test_boot_config.itc_ram_size + 0x4);

    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct vaue
    test_main_data.image_header.itc_ram.uncompressed_size = ITCM_UNCOMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_invalid_uncompressed_dtcm_size, nullptr, nullptr)
{
    // Error injecting through header meta data for uncompressed data to be higher than DTCM size
    test_main_data.image_header.dtc_ram.uncompressed_size = (test_boot_config.dtc_ram_size + 0x4);

    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.dtc_ram.uncompressed_size = DTCM_UNCOMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_invalid_uncompressed_rmss_size, nullptr, nullptr)
{
    // Error injecting through header meta data for uncompressed RMSS data size being invalid
    test_main_data.image_header.rmss_data_ram.uncompressed_size = (test_boot_config.rmss_data_size + 0x4);

    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.rmss_data_ram.uncompressed_size = RMSS_DATA_UNCOMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_invalid_compressed_itcm_size, nullptr, nullptr)
{
    // Error injecting through header meta data for compressed ITCM buffer size than actual data
    test_main_data.image_header.itc_ram.compressed_size =
        ((test_boot_config.data_src_end - test_boot_config.data_src_base) + 0x4);

    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.itc_ram.compressed_size = ITCM_COMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_invalid_compressed_dtcm_size, nullptr, nullptr)
{
    // Error injecting through header meta data for compressed DTCM buffer size than actual data
    test_main_data.image_header.dtc_ram.compressed_size =
        ((test_boot_config.data_src_end - test_boot_config.data_src_base) + 0x4);
    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.dtc_ram.compressed_size = DTCM_COMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_invalid_compressed_rmss_size, nullptr, nullptr)
{
    // Error injecting through header meta data for compressed RMSS buffer size than actual data
    test_main_data.image_header.rmss_data_ram.compressed_size =
        ((test_boot_config.data_src_end - test_boot_config.data_src_base) + 0x4);

    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.rmss_data_ram.compressed_size = RMSS_DATA_COMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_itcm_decompress_fail, nullptr, nullptr)
{
    // Error injecting through header meta data for ITCM compressed offset is wrong
    test_main_data.image_header.itc_ram.compressed_offset = 0x0;
    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.itc_ram.compressed_offset = sizeof(embed_image_header_t);
}

TEST_FUNCTION(test_unpack_image_dtcm_decompress_fail, nullptr, nullptr)
{
    // Error injecting through header meta data for DTCM compressed offset is wrong
    test_main_data.image_header.dtc_ram.compressed_offset = 0x0;
    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.dtc_ram.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_rmss_decompress_fail, nullptr, nullptr)
{
    // Error injecting through header meta data for RMSS compressed offset is wrong
    test_main_data.image_header.rmss_data_ram.compressed_offset = 0x0;
    assert_false(unpack_image(test_boot_config.data_src_base,
                              (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                              test_boot_config.itc_ram_base,
                              test_boot_config.itc_ram_size,
                              test_boot_config.dtc_ram_base,
                              test_boot_config.dtc_ram_size,
                              test_boot_config.rmss_data_base,
                              test_boot_config.rmss_data_size));
    // Reverting to correct value
    test_main_data.image_header.rmss_data_ram.compressed_offset =
        sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE + DTCM_COMPRESSED_STRING_SIZE;
}

TEST_FUNCTION(test_unpack_image_decompress_success, nullptr, nullptr)
{
    memset((void*)test_boot_config.itc_ram_base, 0x0, ITCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.dtc_ram_base, 0x0, DTCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.rmss_data_base, 0x0, RMSS_DATA_UNCOMPRESSED_STRING_SIZE);

    assert_true(unpack_image(test_boot_config.data_src_base,
                             (test_boot_config.data_src_end - test_boot_config.data_src_base) + 1,
                             test_boot_config.itc_ram_base,
                             test_boot_config.itc_ram_size,
                             test_boot_config.dtc_ram_base,
                             test_boot_config.dtc_ram_size,
                             test_boot_config.rmss_data_base,
                             test_boot_config.rmss_data_size));

    assert_int_equal(memcmp((void*)test_boot_config.itc_ram_base, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.dtc_ram_base, DTCM_UNCOMPRESSED_STRING, DTCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.rmss_data_base, RMSS_DATA_UNCOMPRESSED_STRING, RMSS_DATA_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
}
}