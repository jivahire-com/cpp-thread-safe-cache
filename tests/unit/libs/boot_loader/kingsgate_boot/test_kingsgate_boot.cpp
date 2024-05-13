//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_kingsgate_boot.cpp
 * Unit test code for unpack image in boot loader
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>    // for CmockaWrapperTest, TEST_FUNCTION, asse...
#include <cstdint>            // for uint32_t, uint8_t
#include <stddef.h>           // for size_t, NULL
#include <string.h>           // for memset
#include <vcruntime_string.h> // for memcmp

extern "C" {
#include <kingsgate_boot.h> // for load_image, MSCP_CPU_SCP, MSCP_CPU_MCP
#include <unpack_image.h>   // for embed_image_header_t, EMBED_HEADER_TAG

/*-- Symbolic Constant Macros (defines) --*/

#define MEM_CMP_SUCCESS (0)

#define ITCM_COMPRESSED_STRING_SIZE   (40)
#define ITCM_UNCOMPRESSED_STRING_SIZE (10)

#define DTCM_COMPRESSED_STRING_SIZE   (40)
#define DTCM_UNCOMPRESSED_STRING_SIZE (10)

static const char* ITCM_UNCOMPRESSED_STRING = "ITCM TEST\n";
static const char* DTCM_UNCOMPRESSED_STRING = "DTCM TEST\n";

/*------------- Typedefs -----------------*/
typedef struct main_image_test_s
{
    embed_image_header_t image_header;
    // uint8_t compressed_source[ITCM_COMPRESSED_STRING_SIZE + DTCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_itcm[ITCM_COMPRESSED_STRING_SIZE];
    uint8_t compressed_dtcm[DTCM_COMPRESSED_STRING_SIZE];
} main_image_test_t;

/*------------------- Declarations (Statics and globals) --------------------*/
const uint32_t SCP_ITCM_RAM_SIZE = (512 * 1024);
const uint32_t SCP_DTCM_RAM_SIZE = (512 * 1024);

const uint32_t MCP_ITCM_RAM_SIZE = (512 * 1024);
const uint32_t MCP_DTCM_RAM_SIZE = (512 * 1024);

static uint8_t test_itc_ram[SCP_ITCM_RAM_SIZE];
static uint8_t test_dtc_ram[SCP_DTCM_RAM_SIZE];

static kingsgate_boot_metadata_t test_metadata;

static main_image_test_t test_main_image_data = {
    .image_header = {.embed_image_header_tag = EMBED_HEADER_TAG,
                     .itc_ram = {.compressed_offset = sizeof(embed_image_header_t),
                                 .compressed_size = ITCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = ITCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = 0x1A58A044},
                     .dtc_ram = {.compressed_offset = sizeof(embed_image_header_t) + ITCM_COMPRESSED_STRING_SIZE,
                                 .compressed_size = DTCM_COMPRESSED_STRING_SIZE,
                                 .uncompressed_size = DTCM_UNCOMPRESSED_STRING_SIZE,
                                 .uncompressed_crc32 = 0xEFA62BF4}},
    .compressed_itcm = {0x1f, 0x8b, 0x08, 0x08, 0xea, 0x6a, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0xf3, 0x0c, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0x44, 0xa0, 0x58, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x00},
    .compressed_dtcm = {0x1f, 0x8b, 0x08, 0x08, 0x0d, 0x6d, 0x38, 0x66, 0x00, 0x03, 0x68, 0x6f, 0x70, 0x65,
                        0x2e, 0x74, 0x78, 0x74, 0x00, 0x73, 0x09, 0x71, 0xf6, 0x55, 0x08, 0x71, 0x0d, 0x0e,
                        0xe1, 0x02, 0x00, 0xf4, 0x2b, 0xa6, 0xef, 0x0a, 0x00, 0x00, 0x00, 0x00}};

static kingsgate_boot_config_t test_boot_config = {
    .data_src_base = (size_t)&test_main_image_data,
    .data_src_end = (size_t)&test_main_image_data.compressed_dtcm[(DTCM_COMPRESSED_STRING_SIZE - 1)],
    .itc_ram_base = (size_t)&test_itc_ram[0],
    .itc_ram_size = SCP_ITCM_RAM_SIZE,
    .dtc_ram_base = (size_t)&test_dtc_ram[0],
    .dtc_ram_size = SCP_DTCM_RAM_SIZE,
    .boot_meta_base = (size_t)&test_metadata, // This boot config is expected to work correctly when passed as is to unpack_image
    .cpu_type = MSCP_CPU_SCP};
/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
//
// Mocks
//

//
// Tests
//
TEST_FUNCTION(test_null_boot_config, nullptr, nullptr)
{
    kingsgate_boot_config_t* config = NULL;
    assert_null(load_image(config));
}

TEST_FUNCTION(test_boot_config_invalid_cpu, nullptr, nullptr)
{
    // Error injecting invalid cpu type for MSCP
    test_boot_config.cpu_type = MSCP_CPU_INVALID;
    assert_null(load_image(&test_boot_config));
}

TEST_FUNCTION(test_scp_boot_config_src_base_failure, nullptr, nullptr)
{
    // Error injecting invalid main image section start address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_scp_boot_config_src_end_size_failure, nullptr, nullptr)
{
    // Error injecting invalid main image end section address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_end = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_end = (size_t)&test_main_image_data.compressed_dtcm[(DTCM_COMPRESSED_STRING_SIZE - 1)];
}

TEST_FUNCTION(test_scp_boot_config_image_size_zero_failure, nullptr, nullptr)
{
    // Error injecting incorrect main image size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.data_src_base = test_boot_config.data_src_end;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_scp_boot_config_itc_ram_base_failure, nullptr, nullptr)
{
    // Error injecting incorrect ITCM ram base
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.itc_ram_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_base = (size_t)&test_itc_ram[0];
}

TEST_FUNCTION(test_scp_boot_config_itc_ram_size_failure, nullptr, nullptr)
{
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.itc_ram_size = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_size = SCP_ITCM_RAM_SIZE;
}

TEST_FUNCTION(test_scp_boot_config_dtc_ram_base_failure, nullptr, nullptr)
{
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.dtc_ram_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_base = (size_t)&test_dtc_ram[0];
}

TEST_FUNCTION(test_scp_boot_config_dtc_ram_size_failure, nullptr, nullptr)
{
    // Error injecting incorrect DTCM ram size
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.dtc_ram_size = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_size = SCP_DTCM_RAM_SIZE;
}

TEST_FUNCTION(test_scp_boot_config_meta_base_null_failure, nullptr, nullptr)
{
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_boot_config.boot_meta_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_mcp_boot_config_src_base_failure, nullptr, nullptr)
{
    // Error injecting invalid main image section start address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_mcp_boot_config_src_end_size_failure, nullptr, nullptr)
{
    // Error injecting invalid main image end section address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_end = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_end = (size_t)&test_main_image_data.compressed_dtcm[(DTCM_COMPRESSED_STRING_SIZE - 1)];
}

TEST_FUNCTION(test_mcp_boot_config_image_size_zero_failure, nullptr, nullptr)
{
    // Error injecting incorrect main image size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.data_src_base = test_boot_config.data_src_end;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.data_src_base = (size_t)&test_main_image_data;
}

TEST_FUNCTION(test_mcp_boot_config_itc_ram_base_failure, nullptr, nullptr)
{
    // Error injecting incorrect ITCM ram base
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.itc_ram_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_base = (size_t)&test_itc_ram[0];
}

TEST_FUNCTION(test_mcp_boot_config_itc_ram_size_failure, nullptr, nullptr)
{
    // Error injecting incorrect ITCM ram size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.itc_ram_size = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.itc_ram_size = MCP_ITCM_RAM_SIZE;
}

TEST_FUNCTION(test_mcp_boot_config_dtc_ram_base_failure, nullptr, nullptr)
{
    // Error injecting incorrect DTCM ram base
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.dtc_ram_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_base = (size_t)&test_dtc_ram[0];
}

TEST_FUNCTION(test_mcp_boot_config_dtc_ram_size_failure, nullptr, nullptr)
{
    // Error injecting incorrect DTCM ram size
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.dtc_ram_size = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.dtc_ram_size = MCP_DTCM_RAM_SIZE;
}

TEST_FUNCTION(test_mcp_boot_config_meta_base_null_failure, nullptr, nullptr)
{
    // Error injecting incorrect boot meta data base address
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_boot_config.boot_meta_base = 0x0;

    assert_null(load_image(&test_boot_config));
    // Reverting to correct value
    test_boot_config.boot_meta_base = (size_t)&test_metadata;
}

TEST_FUNCTION(test_scp_cold_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_metadata.reset_reason &= ~(BITMASK_WARM_BOOT);
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_scp_warm_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail
    test_metadata.reset_reason |= BITMASK_WARM_BOOT;

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_mcp_cold_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_metadata.reset_reason &= ~(BITMASK_WARM_BOOT);
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_mcp_warm_boot_unpack_image_fail, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    test_main_image_data.compressed_itcm[0] = 0x0; // Corrupt compressed ITCM header to cause decompress() API to fail
    test_metadata.reset_reason |= BITMASK_WARM_BOOT;

    assert_null(load_image(&test_boot_config));

    test_main_image_data.compressed_itcm[0] = 0x1f; // Restore compressed data header
}

TEST_FUNCTION(test_scp_boot_success, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_SCP;
    memset((void*)test_boot_config.itc_ram_base, 0x0, ITCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.dtc_ram_base, 0x0, DTCM_UNCOMPRESSED_STRING_SIZE);

    assert_non_null(load_image(&test_boot_config));
    assert_int_equal(memcmp((void*)test_boot_config.itc_ram_base, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.dtc_ram_base, DTCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
}

TEST_FUNCTION(test_mcp_boot_success, nullptr, nullptr)
{
    test_boot_config.cpu_type = MSCP_CPU_MCP;
    memset((void*)test_boot_config.itc_ram_base, 0x0, ITCM_UNCOMPRESSED_STRING_SIZE);
    memset((void*)test_boot_config.dtc_ram_base, 0x0, DTCM_UNCOMPRESSED_STRING_SIZE);

    assert_non_null(load_image(&test_boot_config));
    assert_int_equal(memcmp((void*)test_boot_config.itc_ram_base, ITCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
    assert_int_equal(memcmp((void*)test_boot_config.dtc_ram_base, DTCM_UNCOMPRESSED_STRING, ITCM_UNCOMPRESSED_STRING_SIZE),
                     MEM_CMP_SUCCESS);
}

} // extern c end