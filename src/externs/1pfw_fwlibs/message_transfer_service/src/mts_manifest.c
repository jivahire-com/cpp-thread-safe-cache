
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_manifest.c
 * Handle metadata preparation for manifest to be read by host
 */

/*-------------------------------- Includes ---------------------------------*/

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <assert.h>
#include <build_data.h> // for BUILD_ELF_SECTION_BINARY_METADATA
#include <diag_decoder.h>
#include <in_band_telemetry_ddr.h>
#include <message_transfer_service.h>
#include <mts_events_i.h>
#include <mts_platform_definitions.h>
#include <stdio.h> // for NULL, printf

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static_assert(sizeof(((diag_packed_manifest_header_t*)0)->manifest_id) <= sizeof(g_note_gnu_build_id.BuildId),
              "Source ID is too small");

static void validate_individual_manifest(p_diag_manifest_header_t p_diag_manifest_start, uint32_t manifest_count)
{
    p_diag_manifest_header_t p_diag_manifest = (p_diag_manifest_header_t)p_diag_manifest_start;

    for (unsigned int count = 1; count <= manifest_count; count++)
    {
        FPFW_RUNTIME_ASSERT_EXT(p_diag_manifest->manifest_parser_type == DIAG_MANIFEST_PARSER_PACKED, 0, 0, 0, 0);
        FPFW_RUNTIME_ASSERT_EXT(p_diag_manifest->manifest_size != 0, 0, 0, 0, 0);

        p_diag_packed_manifest_header_t p_diag_packed_manifest =
            (p_diag_packed_manifest_header_t)((uintptr_t)p_diag_manifest + sizeof(diag_manifest_header_t));

        FPFW_RUNTIME_ASSERT_EXT(p_diag_packed_manifest->provider_metadata_size != 0, 0, 0, 0, 0);
        FPFW_RUNTIME_ASSERT_EXT(p_diag_packed_manifest->event_metadata_size != 0, 0, 0, 0, 0);

        p_diag_manifest =
            (p_diag_manifest_header_t)((uintptr_t)p_diag_packed_manifest + sizeof(diag_packed_manifest_header_t) +
                                       p_diag_packed_manifest->provider_metadata_size +
                                       p_diag_packed_manifest->event_metadata_size);
    }
}

static void validate_packed_manifest(p_diag_manifest_set_v2_header_t p_packed_manifest_hdr)
{
    FPFW_RUNTIME_ASSERT_EXT(p_packed_manifest_hdr->sentinel == DIAG_METADATA_SENTINEL, 0, 0, 0, 0);
    FPFW_RUNTIME_ASSERT_EXT(p_packed_manifest_hdr->manifest_set_header_version == DIAG_MANIFEST_SET_HEADER_VERSION_V2, 0, 0, 0, 0);
    FPFW_RUNTIME_ASSERT_EXT(p_packed_manifest_hdr->manifest_set_size != 0, 0, 0, 0, 0);
    FPFW_RUNTIME_ASSERT_EXT(p_packed_manifest_hdr->manifest_count != 0, 0, 0, 0, 0);

    /* Validate the individual Manifests in the Full Manifest */
    validate_individual_manifest((p_diag_manifest_header_t)p_packed_manifest_hdr->payload,
                                 p_packed_manifest_hdr->manifest_count);
}

static void append_staging_manifest(uintptr_t dest_write_ptr,
                                    p_diag_manifest_set_v2_header_t p_full_manifest_hdr,
                                    uint32_t staging_manifest_hdr_addr)
{
    /* Extract Staging Manifest Metadata and Validate the Header + Individual Manifests */
    p_diag_manifest_set_v2_header_t p_staging_manifest_hdr = (p_diag_manifest_set_v2_header_t)staging_manifest_hdr_addr;
    validate_packed_manifest(p_staging_manifest_hdr);

    /* Update the Manifest */
    p_full_manifest_hdr->manifest_count += p_staging_manifest_hdr->manifest_count;

    /* Before copying the Staging Manifest - discard its header */
    uintptr_t src_read_ptr = (uintptr_t)(p_staging_manifest_hdr->payload);

    memcpy((void*)dest_write_ptr, (void*)src_read_ptr, p_staging_manifest_hdr->manifest_set_size);

    /* Update the value of p_full_manifest_hdr->manifest_set_size based on Staging manifest size */
    p_full_manifest_hdr->manifest_set_size += p_staging_manifest_hdr->manifest_set_size;
}

/*----------------------------- Global Functions ----------------------------*/

void mts_build_diag_decoder_full_manifest(void)
{
    uintptr_t dest_write_ptr = IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;
    /* Write Manifest Header */
    p_diag_manifest_set_v2_header_t p_full_manifest_hdr = (p_diag_manifest_set_v2_header_t)dest_write_ptr;
    p_full_manifest_hdr->manifest_set_header_version = DIAG_MANIFEST_SET_HEADER_VERSION_V2;
    p_full_manifest_hdr->manifest_count = 0;
    p_full_manifest_hdr->sentinel = DIAG_METADATA_SENTINEL;
    p_full_manifest_hdr->manifest_set_size = 0;

    /* Start writing manifests after the header */
    dest_write_ptr += sizeof(diag_manifest_set_v2_header_t);

    append_staging_manifest(dest_write_ptr + p_full_manifest_hdr->manifest_set_size,
                            p_full_manifest_hdr,
                            IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR);
    append_staging_manifest(dest_write_ptr + p_full_manifest_hdr->manifest_set_size,
                            p_full_manifest_hdr,
                            IB_TLM_DDR_ATU_AP_SDM_CDED_STAGING_MANIFEST_BASE_ADDR);

    /* Update the CRC32 now that the full manifest is copied */
    p_full_manifest_hdr->crc32 =
        fpfw_crc32(0, (void*)p_full_manifest_hdr->payload, p_full_manifest_hdr->manifest_set_size);

    /* Validate the final packed manifest */
    validate_packed_manifest(p_full_manifest_hdr);

    printf("------------------------------------------------\n");
    printf("MTS: Full Manifest Count: %d\n", p_full_manifest_hdr->manifest_count);
    printf("MTS: Full Manifest CRC32: 0x%08lx\n", (unsigned long)p_full_manifest_hdr->crc32);
    printf("MTS: Full Manifest Set Size: %ld\n",
           (unsigned long)(p_full_manifest_hdr->manifest_set_size) + sizeof(diag_manifest_set_v2_header_t));
    printf("MTS: ATU Full Manifest Location: 0x%08lx\n", (unsigned long)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR);
    printf("MTS: Physical Full Manifest Location: 0x%08lx%08lx\n",
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR >> 32),
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR & 0xFFFFFFFF));
    printf("------------------------------------------------\n");
}

fpfw_status_t mts_platform_get_manifest_info(p_dcp_msg_get_manifest_t manifest_info)
{
    fpfw_status_t status = FPFW_STATUS_NOT_FOUND;

    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;

    if (set_header->sentinel == DIAG_METADATA_SENTINEL)
    {
        manifest_info->physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
        manifest_info->start_addr_offset = IB_TLM_DDR_GET_FULL_MANIFEST_OFFSET;
        manifest_info->total_size = sizeof(diag_manifest_set_v2_header_t) + set_header->manifest_set_size;

        status = FPFW_STATUS_SUCCESS;

        printf("MTS: Manifest Info Base Address: 0x%08lx%08lx\n",
               (unsigned long)(manifest_info->physical_start_addr >> 32),
               (unsigned long)(manifest_info->physical_start_addr & 0xFFFFFFFF));

        printf("MTS: Manifest Info Base Offset: 0x%08lx%08lx\n",
               (unsigned long)(manifest_info->start_addr_offset >> 32),
               (unsigned long)(manifest_info->start_addr_offset & 0xFFFFFFFF));
    }

    printf("MTS: Full Manifest Size: %ld\n",
           (unsigned long)(set_header->manifest_set_size + sizeof(diag_manifest_set_v2_header_t)));
    printf("MTS: ATU Full Manifest Location: 0x%08lx\n", (unsigned long)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR);
    printf("MTS: Physical Full Manifest Location: 0x%08lx%08lx\n",
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR >> 32),
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR & 0xFFFFFFFF));

    return status;
}
