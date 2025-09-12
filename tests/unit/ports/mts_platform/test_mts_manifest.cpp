//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mts_manifest.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:data_collection

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <build_data.h>
#include <diag_decoder.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_...
#include <in_band_telemetry_ddr.h>
#include <message_transfer_service.h>
#include <mts_platform_specialization.h>
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

TEST_FUNCTION(test_mts_platform_get_manifest_info, test_setup, nullptr)
{
    dcp_msg_get_manifest_t manifest_info;

    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    set_header->sentinel = DIAG_METADATA_SENTINEL;
    set_header->manifest_set_size = 0xAABB;

    fpfw_status_t status = mts_platform_get_manifest_info(&manifest_info);

    assert_true(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(manifest_info.physical_start_addr, IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR);
    assert_int_equal(manifest_info.start_addr_offset, IB_TLM_DDR_GET_FULL_MANIFEST_OFFSET);
    assert_int_equal(manifest_info.total_size, sizeof(diag_manifest_set_v2_header_t) + set_header->manifest_set_size);

    manifest_info.physical_start_addr = 0;
    manifest_info.start_addr_offset = 0;
    manifest_info.total_size = 0;

    set_header->sentinel = 0x18;
    set_header->manifest_set_size = 0xAABB;

    status = mts_platform_get_manifest_info(&manifest_info);

    assert_false(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(manifest_info.physical_start_addr, 0);
    assert_int_equal(manifest_info.start_addr_offset, 0);
    assert_int_equal(manifest_info.total_size, 0);
}

TEST_FUNCTION(test_mts_build_diag_decoder_full_manifest, test_setup, nullptr)
{
    /* Initialize the full Manifest - set to Zeroes */
    diag_manifest_set_v2_header_t* full_manifest_hdr = (diag_manifest_set_v2_header_t*)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    memset(full_manifest_hdr, 0, sizeof(diag_manifest_set_v2_header_t));

    /* Set Up Staging Buffers */
    uintptr_t dest_write_ptr = IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR;
    diag_manifest_set_v2_header_t* mscp_staging_manifest_hdr = (diag_manifest_set_v2_header_t*)dest_write_ptr;
    memset(mscp_staging_manifest_hdr, 0, sizeof(diag_manifest_set_v2_header_t));

    mscp_staging_manifest_hdr->sentinel = DIAG_METADATA_SENTINEL;
    mscp_staging_manifest_hdr->manifest_set_size = 0xAABB;
    mscp_staging_manifest_hdr->manifest_count = 2;
    mscp_staging_manifest_hdr->crc32 = 0xDEADBEEF;
    mscp_staging_manifest_hdr->manifest_set_header_version = DIAG_MANIFEST_SET_HEADER_VERSION_V2;

    dest_write_ptr = IB_TLM_DDR_ATU_AP_SDM_CDED_STAGING_MANIFEST_BASE_ADDR;
    diag_manifest_set_v2_header_t* sdm_cded_staging_manifest_hdr = (diag_manifest_set_v2_header_t*)dest_write_ptr;
    memset(sdm_cded_staging_manifest_hdr, 0, sizeof(diag_manifest_set_v2_header_t));

    sdm_cded_staging_manifest_hdr->sentinel = DIAG_METADATA_SENTINEL;
    sdm_cded_staging_manifest_hdr->manifest_set_size = 0xAABB;
    sdm_cded_staging_manifest_hdr->manifest_count = 2;
    sdm_cded_staging_manifest_hdr->crc32 = 0xDEADBEEF;
    sdm_cded_staging_manifest_hdr->manifest_set_header_version = DIAG_MANIFEST_SET_HEADER_VERSION_V2;

    expect_function_calls(__wrap_FpFwAssertWithArgs, 44);
    mts_build_diag_decoder_full_manifest();

    /* Check that all fields for the final manifest are updated and not zero */
    assert_int_not_equal(full_manifest_hdr->manifest_set_size, 0);
    assert_int_not_equal(full_manifest_hdr->manifest_count, 0);
    assert_int_not_equal(full_manifest_hdr->crc32, 0);
}