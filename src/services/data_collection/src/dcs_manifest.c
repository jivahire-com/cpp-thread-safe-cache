
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_manifest.c
 * Handle metadata preparation for manifest to be read by host
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <assert.h>
#include <build_data.h> // for BUILD_ELF_SECTION_BINARY_METADATA
#include <data_collection_service.h>
#include <dcs_events_i.h>
#include <diag_decoder.h>
#include <in_band_telemetry_ddr.h>
#include <stdio.h> // for NULL, printf

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
extern uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
extern uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
extern uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

/*------------- Functions ----------------*/

static_assert(sizeof(((diag_packed_manifest_header_t*)0)->manifest_id) <= sizeof(g_note_gnu_build_id.BuildId),
              "Source ID is too small");

void dcs_create_manifest_from_elf(uintptr_t atu_base_addr, size_t max_size)
{
    // once HSP supports copying manifests from flash, this function will be removed
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2145384

    diag_packed_manifest_header_t pkd_manifest_hdr = {
        .provider_metadata_size = (uintptr_t)&_ProviderMetadata_et_msdata_end - (uintptr_t)&_ProviderMetadata_et_msdata_start,
        .event_metadata_size = (uintptr_t)&_EventMetadata_et_msdata_end - (uintptr_t)&_EventMetadata_et_msdata_start,
    };

    // the gnu build id is unique per core.  Use the first 16 bytes for the manifest id which needs to be
    // unique for the diagnostic decoder tool to decode the data
    memcpy((void*)&pkd_manifest_hdr.manifest_id, (void*)g_note_gnu_build_id.BuildId, sizeof(pkd_manifest_hdr.manifest_id));

    diag_manifest_header_t manifest_hdr = {
        .manifest_parser_type = DIAG_MANIFEST_PARSER_PACKED,
        .manifest_size = sizeof(diag_packed_manifest_header_t) + pkd_manifest_hdr.provider_metadata_size +
                         pkd_manifest_hdr.event_metadata_size,
    };

    size_t sizeNeeded = manifest_hdr.manifest_size + sizeof(manifest_hdr);
    FpFwAssertWithArgs(sizeNeeded < max_size, sizeNeeded, max_size, 0, 0);

    // copy the metadata to the desired destination
    memcpy((void*)atu_base_addr, &manifest_hdr, sizeof(manifest_hdr));
    atu_base_addr += sizeof(manifest_hdr);
    memcpy((void*)atu_base_addr, &pkd_manifest_hdr, sizeof(pkd_manifest_hdr));
    atu_base_addr += sizeof(pkd_manifest_hdr);
    memcpy((void*)atu_base_addr, (void*)&_ProviderMetadata_et_msdata_start, pkd_manifest_hdr.provider_metadata_size);
    atu_base_addr += pkd_manifest_hdr.provider_metadata_size;
    memcpy((void*)atu_base_addr, (void*)&_EventMetadata_et_msdata_start, pkd_manifest_hdr.event_metadata_size);
}

void dcs_build_diag_decoder_full_manifest(void)
{
    // currently only SCP is located in IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR
    //  https://azurecsi.visualstudio.com/Dev/_workitems/edit/2145384
    // HSP will copy a manifest set from flash to staging DDR that includes both SCP and MCP
    // will update this at that time

    // validate SCP
    //
    diag_manifest_header_t* scp_staging_manifest_hdr =
        (diag_manifest_header_t*)IB_TLM_DDR_ATU_AP_MSCP_STAGING_MANIFEST_BASE_ADDR;

    // use printf because event trace needs manifest for decode
    if (scp_staging_manifest_hdr->manifest_parser_type != DIAG_MANIFEST_PARSER_PACKED)
    {
        printf("DCS: SCP Manifest Parser Type is not Packed\n");
        FPFW_ET_LOG(DcsManifestCreateFail, CPU_SCP);
        return;
    }

    size_t remaining_dest_size = IB_TLM_DDR_ATU_AP_FULL_MANIFEST_SIZE;
    uintptr_t dest_write_ptr = IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;

    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)dest_write_ptr;
    set_header->manifest_set_header_version = DIAG_MANIFEST_SET_HEADER_VERSION_V2;
    set_header->manifest_count = 2;
    set_header->sentinel = DIAG_METADATA_SENTINEL;
    set_header->manifest_set_size = 0;
    // update crc last

    // write SCP manifest
    dest_write_ptr += sizeof(diag_manifest_set_v2_header_t);
    remaining_dest_size -= sizeof(diag_manifest_set_v2_header_t);
    // dest_write_ptr is now equal to the first manifest which is SCP

    size_t scp_manifest_size = scp_staging_manifest_hdr->manifest_size + sizeof(diag_manifest_header_t);

    memcpy((void*)dest_write_ptr, scp_staging_manifest_hdr, scp_manifest_size);
    dest_write_ptr += scp_manifest_size;
    set_header->manifest_set_size += scp_manifest_size;
    remaining_dest_size -= scp_manifest_size;

    // write mcp manifest
    diag_manifest_header_t* mcp_dest_manifest_hdr = (diag_manifest_header_t*)dest_write_ptr;
    dcs_create_manifest_from_elf((uintptr_t)mcp_dest_manifest_hdr, remaining_dest_size);

    size_t mcp_manifest_size = mcp_dest_manifest_hdr->manifest_size + sizeof(diag_manifest_header_t);
    set_header->manifest_set_size += mcp_manifest_size;

    set_header->crc32 = fpfw_crc32(0, (void*)set_header->payload, set_header->manifest_set_size);

    printf("DCS: Full Manifest Set Size: %ld\n",
           (unsigned long)(set_header->manifest_set_size + sizeof(diag_manifest_set_v2_header_t)));
    printf("DCS: ATU Full Manifest Location: 0x%08lx\n", (unsigned long)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR);
    printf("DCS: Physical Full Manifest Location: 0x%08lx%08lx\n",
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR >> 32),
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR & 0xFFFFFFFF));
}

fpfw_status_t dcs_get_manifest_info(p_dcp_msg_get_manifest_t manifest_info)
{
    fpfw_status_t status = FPFW_STATUS_NOT_FOUND;

    diag_manifest_set_v2_header_t* set_header = (diag_manifest_set_v2_header_t*)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR;

    if (set_header->sentinel == DIAG_METADATA_SENTINEL)
    {
        manifest_info->physical_start_addr = IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR;
        manifest_info->start_addr_offset = IB_TLM_DDR_GET_FULL_MANIFEST_OFFSET;
        manifest_info->total_size = sizeof(diag_manifest_set_v2_header_t) + set_header->manifest_set_size;

        status = FPFW_STATUS_SUCCESS;

        printf("DCS: Manifest Info Base Address: 0x%08lx%08lx\n",
               (unsigned long)(manifest_info->physical_start_addr >> 32),
               (unsigned long)(manifest_info->physical_start_addr & 0xFFFFFFFF));

        printf("DCS: Manifest Info Base Offset: 0x%08lx%08lx\n",
               (unsigned long)(manifest_info->start_addr_offset >> 32),
               (unsigned long)(manifest_info->start_addr_offset & 0xFFFFFFFF));
    }

    printf("DCS: Full Manifest Size: %ld\n",
           (unsigned long)(set_header->manifest_set_size + sizeof(diag_manifest_set_v2_header_t)));
    printf("DCS: ATU Full Manifest Location: 0x%08lx\n", (unsigned long)IB_TLM_DDR_ATU_AP_FULL_MANIFEST_BASE_ADDR);
    printf("DCS: Physical Full Manifest Location: 0x%08lx%08lx\n",
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR >> 32),
           (unsigned long)(IB_TLM_DDR_PHY_AP_FULL_MANIFEST_BASE_ADDR & 0xFFFFFFFF));

    return status;
}
