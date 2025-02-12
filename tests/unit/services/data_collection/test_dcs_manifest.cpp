//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcs_manifest.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <build_data.h>
#include <data_collection_service.h>
#include <dcs_manifest_i.h>
#include <diag_decoder.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <in_band_telemetry_ddr.h>
#include <stdint.h> // for uint8_t
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    return 0;
}

TEST_FUNCTION(test_dcs_get_manifest_info, test_setup, nullptr)
{
    uint64_t manifest_offset = 0;
    uint64_t manifest_size = 0;

    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    set_header->sentinel = DIAG_METADATA_SENTINEL;
    set_header->manifest_set_size = 0xAABB;

    fpfw_status_t status = dcs_get_manifest_info(&manifest_offset, &manifest_size);

    assert_true(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(manifest_offset, IB_TLM_DDR_GET_FULL_MANIFEST_OFFSET);
    assert_int_equal(manifest_size, sizeof(diag_manifest_set_v2_header_t) + set_header->manifest_set_size);

    manifest_offset = 0;
    manifest_size = 0;

    set_header->sentinel = 0x18;
    set_header->manifest_set_size = 0xAABB;

    status = dcs_get_manifest_info(&manifest_offset, &manifest_size);

    assert_false(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(manifest_offset, 0);
    assert_int_equal(manifest_size, 0);
}

TEST_FUNCTION(test_dcs_create_manifest_from_elf, test_setup, nullptr)
{
    uint8_t dest_buffer[1024] = {0};

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_create_manifest_from_elf((uintptr_t)dest_buffer, sizeof(dest_buffer));

    p_diag_manifest_header_t manifest_hdr = (p_diag_manifest_header_t)dest_buffer;
    assert_int_equal(manifest_hdr->manifest_parser_type, DIAG_MANIFEST_PARSER_PACKED);
}

TEST_FUNCTION(test_dcs_build_diag_decoder_full_manifest, test_setup, nullptr)
{
    // output destination buffer
    uintptr_t dest_write_ptr = IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)dest_write_ptr;
    memset(set_header, 0, sizeof(diag_manifest_set_v2_header_t));

    diag_manifest_header_t* scp_staging_manifest_hdr =
        (diag_manifest_header_t*)IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR;
    diag_packed_manifest_header_t* scp_staging_packed_manifest_hdr =
        (diag_packed_manifest_header_t*)scp_staging_manifest_hdr->payload;

    scp_staging_manifest_hdr->manifest_parser_type = DIAG_MANIFEST_PARSER_PACKED;
    scp_staging_manifest_hdr->manifest_size = 0;
    memset(&scp_staging_packed_manifest_hdr->manifest_id, 0xAA, sizeof(scp_staging_packed_manifest_hdr->manifest_id));

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    dcs_build_diag_decoder_full_manifest();
}

TEST_FUNCTION(test_dcs_build_diag_decoder_full_manifest_validate, test_setup, nullptr)
{
    // output destination buffer
    uintptr_t dest_write_ptr = IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)dest_write_ptr;
    memset(set_header, 0, sizeof(diag_manifest_set_v2_header_t));

    diag_manifest_header_t* scp_staging_manifest_hdr =
        (diag_manifest_header_t*)IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR;

    // validate wrong parser type
    scp_staging_manifest_hdr->manifest_parser_type = 0x48;
    dcs_build_diag_decoder_full_manifest();

    assert_int_equal(set_header->manifest_set_size, 0);
    assert_int_equal(set_header->manifest_count, 0);
}